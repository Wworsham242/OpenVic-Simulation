#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/core/simulation/CampaignStateSnapshot.hpp"
#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"
#include "openvic-simulation/ecs/World.hpp"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

using namespace OpenVic;
using namespace OpenVic::ecs;

namespace {

struct PersistenceBenchComponent {
	std::int64_t value = 0;
};

}

ECS_COMPONENT(PersistenceBenchComponent, "project_convergence_006a2_5::PersistenceBenchComponent")

namespace {

struct Tier {
	std::string_view name;
	std::size_t entity_count;
	std::size_t command_count;
};

Tier parse_tier(std::string_view name) {
	if (name == "smoke") {
		return { "smoke", 10'000, 100 };
	}
	if (name == "regional") {
		return { "regional", 100'000, 1'000 };
	}
	if (name == "large") {
		return { "large", 500'000, 5'000 };
	}
	if (name == "world") {
		return { "world", 1'000'000, 10'000 };
	}

	std::cerr << "Unknown tier: " << name << "\n";
	std::exit(2);
}

std::uint64_t working_set_bytes() {
#ifdef _WIN32
	PROCESS_MEMORY_COUNTERS_EX counters {};
	counters.cb = sizeof(counters);

	if (GetProcessMemoryInfo(
			GetCurrentProcess(),
			reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
			sizeof(counters)
		)) {
		return static_cast<std::uint64_t>(counters.WorkingSetSize);
	}
#endif
	return 0;
}

template<typename Fn>
double timed_ms(Fn&& fn) {
	auto const start = std::chrono::steady_clock::now();
	fn();
	auto const end = std::chrono::steady_clock::now();
	return std::chrono::duration<double, std::milli>(end - start).count();
}

std::vector<std::uint8_t> build_free_slot_mask(
	WorldIdentitySnapshot const& snapshot
) {
	std::vector<std::uint8_t> mask(snapshot.slots.size(), std::uint8_t { 0 });

	for (std::uint32_t index : snapshot.free_list) {
		if (index < mask.size()) {
			mask[index] = std::uint8_t { 1 };
		}
	}

	return mask;
}

CampaignStateSnapshot build_campaign_snapshot(
	WorldIdentitySnapshot identity,
	std::size_t command_count
) {
	SimulationTimeline timeline;
	if (!timeline.advance(48)) {
		std::cerr << "Timeline advance failed\n";
		std::exit(3);
	}

	for (std::uint64_t i = 0; i < 64; ++i) {
		auto const scheduled = timeline.schedule_event(
			SimTime::from_ticks(72 + static_cast<std::int64_t>(i)),
			SimulationEventPayload {
				.type_id = "006a2.5.synthetic-event",
				.data = {
					static_cast<std::uint8_t>(i & 0xffu),
					static_cast<std::uint8_t>((i >> 8u) & 0xffu)
				}
			}
		);
		if (!scheduled.has_value()) {
			std::cerr << "Timeline event scheduling failed\n";
			std::exit(3);
		}
	}

	CampaignStateSnapshot snapshot;
	snapshot.timeline = timeline.capture_snapshot();
	snapshot.rng_streams = {
		CampaignRngStreamState {
			.stream_id = "ai.strategic",
			.state_lo = 0x1111111111111111ull,
			.state_hi = 0x2222222222222222ull,
			.draw_count = 1'024
		},
		CampaignRngStreamState {
			.stream_id = "combat.operational",
			.state_lo = 0x3333333333333333ull,
			.state_hi = 0x4444444444444444ull,
			.draw_count = 2'048
		},
		CampaignRngStreamState {
			.stream_id = "economy.market",
			.state_lo = 0x5555555555555555ull,
			.state_hi = 0x6666666666666666ull,
			.draw_count = 4'096
		},
		CampaignRngStreamState {
			.stream_id = "population.demography",
			.state_lo = 0x7777777777777777ull,
			.state_hi = 0x8888888888888888ull,
			.draw_count = 8'192
		}
	};

	snapshot.command_log.reserve(command_count);
	for (std::size_t i = 0; i < command_count; ++i) {
		snapshot.command_log.push_back(CampaignCommandRecord {
			.sequence = static_cast<std::uint64_t>(i),
			.submitted_at = SimTime::from_ticks(
				static_cast<std::int64_t>(i % 48)
			),
			.actor_id = "actor:" + std::to_string(i % 200),
			.command_type = "006a2.5.synthetic-command",
			.jurisdiction_id = "jurisdiction:" + std::to_string(i % 200),
			.payload = {
				static_cast<std::uint8_t>(i & 0xffu),
				static_cast<std::uint8_t>((i >> 8u) & 0xffu),
				static_cast<std::uint8_t>((i >> 16u) & 0xffu),
				static_cast<std::uint8_t>((i >> 24u) & 0xffu)
			}
		});
	}

	snapshot.replay = CampaignReplayState {
		.accepted_command_count = static_cast<std::uint64_t>(snapshot.command_log.size()),
		.replay_cursor = static_cast<std::uint64_t>(snapshot.command_log.size())
	};
	snapshot.ecs_identity = std::move(identity);
	return snapshot;
}

} // namespace

int main(int argc, char** argv) {
	std::string_view tier_name = "smoke";

	for (int i = 1; i < argc; ++i) {
		std::string_view arg = argv[i];
		if (arg == "--tier" && i + 1 < argc) {
			tier_name = argv[++i];
		}
	}

	Tier const tier = parse_tier(tier_name);

	World original;
	std::vector<EntityID> ids;
	ids.reserve(tier.entity_count);

	std::uint64_t const memory_before = working_set_bytes();

	double const create_ms = timed_ms([&] {
		for (std::size_t i = 0; i < tier.entity_count; ++i) {
			ids.push_back(original.create_entity(
				PersistenceBenchComponent {
					static_cast<std::int64_t>(i * 17 + 3)
				}
			));
		}
	});

	// Deterministic 5% free-list pressure. Destruction order is deterministic
	// because free-list order itself is part of the persistence contract.
	std::size_t destroyed_count = 0;
	double const destroy_ms = timed_ms([&] {
		for (std::size_t i = 0; i < ids.size(); i += 20) {
			original.destroy_entity(ids[i]);
			++destroyed_count;
		}
	});

	std::uint64_t const memory_after_world = working_set_bytes();

	WorldIdentitySnapshot identity;
	bool snapshot_identity_ok = false;
	double const identity_snapshot_ms = timed_ms([&] {
		snapshot_identity_ok = original.snapshot_identity(identity);
	});

	CampaignStateSnapshot campaign;
	double const campaign_compose_ms = timed_ms([&] {
		campaign = build_campaign_snapshot(identity, tier.command_count);
	});

	bool const campaign_canonical = campaign.is_canonical();
	std::vector<std::uint8_t> const free_slot_mask =
		build_free_slot_mask(campaign.ecs_identity);

	std::uint64_t campaign_checksum = 0;
	double const campaign_checksum_ms = timed_ms([&] {
		campaign_checksum = campaign.checksum();
	});

	SimulationTimeline restored_timeline;
	bool timeline_restore_ok = false;
	double const timeline_restore_ms = timed_ms([&] {
		timeline_restore_ok = restored_timeline.restore_snapshot(campaign.timeline);
	});
	bool const timeline_roundtrip_equal =
		timeline_restore_ok &&
		restored_timeline.capture_snapshot() == campaign.timeline;

	World restored;
	bool identity_restore_ok = false;
	double const identity_restore_ms = timed_ms([&] {
		identity_restore_ok = restored.restore_identity(campaign.ecs_identity);
	});

	std::size_t live_restored = 0;
	bool restore_entities_ok = identity_restore_ok;
	double const entity_restore_ms = timed_ms([&] {
		if (!identity_restore_ok) {
			return;
		}

		for (std::uint32_t index = 0;
			index < campaign.ecs_identity.slots.size();
			++index
		) {
			if (free_slot_mask[index] != std::uint8_t { 0 }) {
				continue;
			}

			auto const& slot = campaign.ecs_identity.slots[index];
			EntityID const eid { index, slot.generation };

			if (!restored.restore_entity(
				eid,
				PersistenceBenchComponent {
					static_cast<std::int64_t>(
						static_cast<std::uint64_t>(index) * 17u + 3u
					)
				}
			)) {
				restore_entities_ok = false;
				break;
			}

			++live_restored;
		}
	});

	bool component_values_ok = restore_entities_ok;
	double const component_validate_ms = timed_ms([&] {
		if (!restore_entities_ok) {
			return;
		}

		for (std::uint32_t index = 0;
			index < campaign.ecs_identity.slots.size();
			++index
		) {
			if (free_slot_mask[index] != std::uint8_t { 0 }) {
				continue;
			}

			auto const& slot = campaign.ecs_identity.slots[index];
			EntityID const eid { index, slot.generation };
			auto const* component =
				restored.get_component<PersistenceBenchComponent>(eid);

			if (
				component == nullptr ||
				component->value != static_cast<std::int64_t>(
					static_cast<std::uint64_t>(index) * 17u + 3u
				)
			) {
				component_values_ok = false;
				break;
			}
		}
	});

	WorldIdentitySnapshot restored_identity;
	bool restored_snapshot_ok = false;
	double const restored_snapshot_ms = timed_ms([&] {
		restored_snapshot_ok = restored.snapshot_identity(restored_identity);
	});

	bool const identity_roundtrip_equal =
		restored_snapshot_ok &&
		restored_identity == campaign.ecs_identity;

	EntityID original_next {};
	EntityID restored_next {};
	double const continuation_ms = timed_ms([&] {
		original_next = original.create_entity(
			PersistenceBenchComponent { 9'999'999 }
		);
		restored_next = restored.create_entity(
			PersistenceBenchComponent { 9'999'999 }
		);
	});
	bool const allocator_continuity = original_next == restored_next;

	CampaignStateSnapshot roundtrip_campaign = campaign;
	roundtrip_campaign.timeline = restored_timeline.capture_snapshot();
	roundtrip_campaign.ecs_identity = restored_identity;

	bool const campaign_roundtrip_equal =
		roundtrip_campaign == campaign;
	bool const campaign_checksum_equal =
		roundtrip_campaign.checksum() == campaign_checksum;

	std::uint64_t const memory_after_restore = working_set_bytes();

	std::cout
		<< "{\n"
		<< "  \"increment\": \"PROJECT-CONVERGENCE-006A2.5\",\n"
		<< "  \"scope\": \"authoritative-persistence-replay-scale\",\n"
		<< "  \"tier\": \"" << tier.name << "\",\n"
		<< "  \"counts\": {\n"
		<< "    \"entities_created\": " << tier.entity_count << ",\n"
		<< "    \"entities_destroyed\": " << destroyed_count << ",\n"
		<< "    \"entities_live\": " << (tier.entity_count - destroyed_count) << ",\n"
		<< "    \"entities_restored\": " << live_restored << ",\n"
		<< "    \"identity_slots\": " << campaign.ecs_identity.slots.size() << ",\n"
		<< "    \"free_slots\": " << campaign.ecs_identity.free_list.size() << ",\n"
		<< "    \"timeline_events\": " << campaign.timeline.events.size() << ",\n"
		<< "    \"rng_streams\": " << campaign.rng_streams.size() << ",\n"
		<< "    \"command_records\": " << campaign.command_log.size() << "\n"
		<< "  },\n"
		<< "  \"validation\": {\n"
		<< "    \"identity_snapshot_ok\": " << (snapshot_identity_ok ? "true" : "false") << ",\n"
		<< "    \"campaign_canonical\": " << (campaign_canonical ? "true" : "false") << ",\n"
		<< "    \"timeline_restore_ok\": " << (timeline_restore_ok ? "true" : "false") << ",\n"
		<< "    \"timeline_roundtrip_equal\": " << (timeline_roundtrip_equal ? "true" : "false") << ",\n"
		<< "    \"identity_restore_ok\": " << (identity_restore_ok ? "true" : "false") << ",\n"
		<< "    \"restore_entities_ok\": " << (restore_entities_ok ? "true" : "false") << ",\n"
		<< "    \"component_values_reconstructed_ok\": " << (component_values_ok ? "true" : "false") << ",\n"
		<< "    \"identity_roundtrip_equal\": " << (identity_roundtrip_equal ? "true" : "false") << ",\n"
		<< "    \"allocator_continuity\": " << (allocator_continuity ? "true" : "false") << ",\n"
		<< "    \"campaign_roundtrip_equal\": " << (campaign_roundtrip_equal ? "true" : "false") << ",\n"
		<< "    \"campaign_checksum_equal\": " << (campaign_checksum_equal ? "true" : "false") << "\n"
		<< "  },\n"
		<< "  \"coverage\": {\n"
		<< "    \"timeline_in_campaign_snapshot\": true,\n"
		<< "    \"rng_state_in_campaign_snapshot\": true,\n"
		<< "    \"command_log_in_campaign_snapshot\": true,\n"
		<< "    \"ecs_identity_in_campaign_snapshot\": true,\n"
		<< "    \"authoritative_component_payloads_in_campaign_snapshot\": false,\n"
		<< "    \"domain_singletons_in_campaign_snapshot\": false\n"
		<< "  },\n"
		<< "  \"timing_ms\": {\n"
		<< "    \"entity_create\": " << create_ms << ",\n"
		<< "    \"entity_destroy\": " << destroy_ms << ",\n"
		<< "    \"identity_snapshot\": " << identity_snapshot_ms << ",\n"
		<< "    \"campaign_compose\": " << campaign_compose_ms << ",\n"
		<< "    \"campaign_checksum\": " << campaign_checksum_ms << ",\n"
		<< "    \"timeline_restore\": " << timeline_restore_ms << ",\n"
		<< "    \"identity_restore\": " << identity_restore_ms << ",\n"
		<< "    \"entity_restore\": " << entity_restore_ms << ",\n"
		<< "    \"component_validate\": " << component_validate_ms << ",\n"
		<< "    \"restored_identity_snapshot\": " << restored_snapshot_ms << ",\n"
		<< "    \"allocator_continuation\": " << continuation_ms << "\n"
		<< "  },\n"
		<< "  \"memory_bytes\": {\n"
		<< "    \"before\": " << memory_before << ",\n"
		<< "    \"after_original_world\": " << memory_after_world << ",\n"
		<< "    \"after_restored_world\": " << memory_after_restore << "\n"
		<< "  },\n"
		<< "  \"campaign_checksum\": " << campaign_checksum << ",\n"
		<< "  \"limitations\": [\n"
		<< "    \"measures the existing campaign envelope and ECS identity persistence contract\",\n"
		<< "    \"component values are reconstructed by the benchmark because CampaignStateSnapshot does not yet serialize authoritative component payloads\",\n"
		<< "    \"does not claim full authoritative-world save/load certification\",\n"
		<< "    \"does not add a second persistence ledger or file format\",\n"
		<< "    \"successful allocator continuity proves saved ECS identity preserves subsequent EntityID allocation behavior\"\n"
		<< "  ]\n"
		<< "}\n";

	bool const pass =
		snapshot_identity_ok &&
		campaign_canonical &&
		timeline_restore_ok &&
		timeline_roundtrip_equal &&
		identity_restore_ok &&
		restore_entities_ok &&
		component_values_ok &&
		identity_roundtrip_equal &&
		allocator_continuity &&
		campaign_roundtrip_equal &&
		campaign_checksum_equal;

	return pass ? 0 : 1;
}
