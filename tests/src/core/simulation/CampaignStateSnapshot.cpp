#include "openvic-simulation/core/simulation/CampaignStateSnapshot.hpp"

#include "openvic-simulation/ecs/World.hpp"

#include <cstdint>
#include <utility>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;
using namespace OpenVic::ecs;

namespace {
	struct CampaignTestComponent {
		int64_t value = 0;
	};
}
ECS_COMPONENT(CampaignTestComponent, "foundation_005::CampaignTestComponent")

namespace {
	CampaignStateSnapshot make_campaign_fixture() {
		SimulationTimeline timeline;
		REQUIRE(timeline.advance(48));
		REQUIRE(timeline.schedule_event(
			SimTime::from_ticks(72),
			SimulationEventPayload { .type_id = "campaign.test", .data = { 1, 2 } }
		).has_value());

		World world;
		EntityID const a = world.create_entity(CampaignTestComponent { 10 });
		EntityID const b = world.create_entity(CampaignTestComponent { 20 });
		world.destroy_entity(a);
		CHECK(world.is_alive(b));

		WorldIdentitySnapshot identity;
		REQUIRE(world.snapshot_identity(identity));

		return CampaignStateSnapshot {
			.schema_version = CampaignStateSnapshot::CURRENT_SCHEMA_VERSION,
			.timeline = timeline.capture_snapshot(),
			.rng_streams = {
				CampaignRngStreamState {
					.stream_id = "ai.strategic",
					.state_lo = 11,
					.state_hi = 22,
					.draw_count = 7
				},
				CampaignRngStreamState {
					.stream_id = "combat.operational",
					.state_lo = 33,
					.state_hi = 44,
					.draw_count = 9
				}
			},
			.replay = CampaignReplayState {
				.accepted_command_count = 3,
				.replay_cursor = 2
			},
			.command_log = {
				CampaignCommandRecord {
					.sequence = 0,
					.submitted_at = SimTime::from_ticks(24),
					.actor_id = "actor:fixture:a",
					.command_type = "fixture.command.a",
					.payload = { 1 }
				},
				CampaignCommandRecord {
					.sequence = 1,
					.submitted_at = SimTime::from_ticks(24),
					.actor_id = "actor:fixture:b",
					.command_type = "fixture.command.b",
					.payload = { 2 }
				},
				CampaignCommandRecord {
					.sequence = 2,
					.submitted_at = SimTime::from_ticks(48),
					.actor_id = "actor:fixture:c",
					.command_type = "fixture.command.c",
					.payload = { 3 }
				}
			},
			.ecs_identity = std::move(identity)
		};
	}
}

TEST_CASE("Campaign snapshot fixture is canonical", "[foundation][campaign][persistence]") {
	CampaignStateSnapshot snapshot = make_campaign_fixture();
	CHECK(snapshot.is_canonical());
}

TEST_CASE("Campaign checksum covers all composed durable dimensions", "[foundation][campaign][checksum]") {
	const CampaignStateSnapshot baseline = make_campaign_fixture();
	REQUIRE(baseline.is_canonical());
	const uint64_t baseline_checksum = baseline.checksum();

	auto changed = baseline;
	changed.timeline.current_time = SimTime::from_ticks(49);
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.rng_streams[0].draw_count += 1;
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.replay.replay_cursor += 1;
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.command_log[0].payload.push_back(9);
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.ecs_identity.slots[1].immutable = true;
	CHECK(changed.checksum() != baseline_checksum);

	changed = baseline;
	changed.ecs_identity.free_list.clear();
	CHECK(changed.checksum() != baseline_checksum);
}

TEST_CASE("Campaign canonical validation rejects ambiguous durable state", "[foundation][campaign][validation]") {
	const CampaignStateSnapshot baseline = make_campaign_fixture();

	auto bad = baseline;
	bad.schema_version += 1;
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.replay.replay_cursor = bad.replay.accepted_command_count + 1;
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.replay.accepted_command_count += 1;
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.command_log[1].sequence = 99;
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.command_log[0].actor_id.clear();
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	std::swap(bad.rng_streams[0], bad.rng_streams[1]);
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.rng_streams[1].stream_id = bad.rng_streams[0].stream_id;
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.rng_streams[0].stream_id.clear();
	CHECK_FALSE(bad.is_canonical());

	bad = baseline;
	bad.ecs_identity.free_list.push_back(bad.ecs_identity.free_list.front());
	CHECK_FALSE(bad.is_canonical());
}

TEST_CASE("Campaign envelope ECS identity restores allocator continuity", "[foundation][campaign][ecs][replay]") {
	World original;

	EntityID const e0 = original.create_entity(CampaignTestComponent { 0 });
	EntityID const e1 = original.create_entity(CampaignTestComponent { 1 });
	EntityID const e2 = original.create_entity(CampaignTestComponent { 2 });
	original.destroy_entity(e1);

	WorldIdentitySnapshot identity;
	REQUIRE(original.snapshot_identity(identity));

	CampaignStateSnapshot snapshot;
	snapshot.timeline = SimulationTimeline {}.capture_snapshot();
	snapshot.rng_streams = {
		CampaignRngStreamState { .stream_id = "core", .state_lo = 1, .state_hi = 2, .draw_count = 0 }
	};
	snapshot.ecs_identity = identity;
	REQUIRE(snapshot.is_canonical());

	World restored;
	REQUIRE(restored.restore_identity(snapshot.ecs_identity));
	REQUIRE(restored.restore_entity(e0, CampaignTestComponent { 0 }));
	REQUIRE(restored.restore_entity(e2, CampaignTestComponent { 2 }));

	EntityID const continued = original.create_entity(CampaignTestComponent { 99 });
	EntityID const replayed = restored.create_entity(CampaignTestComponent { 99 });

	CHECK(replayed == continued);
}

TEST_CASE("Identical campaign snapshots produce identical checksum", "[foundation][campaign][replay]") {
	const CampaignStateSnapshot a = make_campaign_fixture();
	const CampaignStateSnapshot b = a;

	REQUIRE(a.is_canonical());
	REQUIRE(b.is_canonical());
	CHECK(a == b);
	CHECK(a.checksum() == b.checksum());
}