#pragma once

#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenVic {

struct CampaignRngStreamState {
	std::string stream_id;
	uint64_t state_lo = 0;
	uint64_t state_hi = 0;
	uint64_t draw_count = 0;

	bool operator==(CampaignRngStreamState const&) const = default;
};

struct CampaignReplayState {
	uint64_t accepted_command_count = 0;
	uint64_t replay_cursor = 0;

	bool operator==(CampaignReplayState const&) const = default;
};

struct CampaignCommandRecord {
	uint64_t sequence = 0;
	SimTime submitted_at = SimTime::from_ticks(0);
	std::string actor_id;
	std::string command_type;
	std::vector<uint8_t> payload;

	bool operator==(CampaignCommandRecord const&) const = default;
};

struct CampaignStateSnapshot {
	static constexpr uint32_t CURRENT_SCHEMA_VERSION = 1;

	uint32_t schema_version = CURRENT_SCHEMA_VERSION;
	SimulationTimelineSnapshot timeline;
	std::vector<CampaignRngStreamState> rng_streams;
	CampaignReplayState replay;
	std::vector<CampaignCommandRecord> command_log;
	ecs::WorldIdentitySnapshot ecs_identity;

	bool operator==(CampaignStateSnapshot const&) const = default;

	[[nodiscard]] bool is_canonical() const {
		if (schema_version != CURRENT_SCHEMA_VERSION) {
			return false;
		}
		if (timeline.schema_version != SimulationTimelineSnapshot::CURRENT_SCHEMA_VERSION) {
			return false;
		}
		if (replay.accepted_command_count != command_log.size()
			|| replay.replay_cursor > replay.accepted_command_count) {
			return false;
		}
		for (std::size_t i = 0; i < command_log.size(); ++i) {
			CampaignCommandRecord const& command = command_log[i];
			if (command.sequence != i || command.actor_id.empty() || command.command_type.empty()) {
				return false;
			}
		}
		for (std::size_t i = 1; i < rng_streams.size(); ++i) {
			if (rng_streams[i - 1].stream_id >= rng_streams[i].stream_id) {
				return false;
			}
		}
		for (CampaignRngStreamState const& stream : rng_streams) {
			if (stream.stream_id.empty()) {
				return false;
			}
		}
		std::vector<uint32_t> free_indices = ecs_identity.free_list;
		std::sort(free_indices.begin(), free_indices.end());
		if (std::adjacent_find(free_indices.begin(), free_indices.end()) != free_indices.end()) {
			return false;
		}
		for (uint32_t index : free_indices) {
			if (index >= ecs_identity.slots.size()) {
				return false;
			}
		}
		for (auto const& slot : ecs_identity.slots) {
			if (slot.generation == 0) {
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] uint64_t checksum() const {
		static constexpr uint64_t FNV_OFFSET = 14695981039346656037ull;
		static constexpr uint64_t FNV_PRIME = 1099511628211ull;

		auto fold_byte = [](uint64_t hash, uint8_t byte) {
			hash ^= static_cast<uint64_t>(byte);
			hash *= FNV_PRIME;
			return hash;
		};
		auto fold_u64 = [&](uint64_t hash, uint64_t value) {
			for (unsigned shift = 0; shift < 64; shift += 8) {
				hash = fold_byte(hash, static_cast<uint8_t>((value >> shift) & 0xffu));
			}
			return hash;
		};
		auto fold_string = [&](uint64_t hash, std::string const& value) {
			hash = fold_u64(hash, static_cast<uint64_t>(value.size()));
			for (unsigned char byte : value) {
				hash = fold_byte(hash, static_cast<uint8_t>(byte));
			}
			return hash;
		};

		uint64_t hash = FNV_OFFSET;
		hash = fold_u64(hash, schema_version);
		hash = fold_u64(hash, timeline.checksum());

		hash = fold_u64(hash, static_cast<uint64_t>(rng_streams.size()));
		for (CampaignRngStreamState const& stream : rng_streams) {
			hash = fold_string(hash, stream.stream_id);
			hash = fold_u64(hash, stream.state_lo);
			hash = fold_u64(hash, stream.state_hi);
			hash = fold_u64(hash, stream.draw_count);
		}

		hash = fold_u64(hash, replay.accepted_command_count);
		hash = fold_u64(hash, replay.replay_cursor);

		hash = fold_u64(hash, static_cast<uint64_t>(command_log.size()));
		for (CampaignCommandRecord const& command : command_log) {
			hash = fold_u64(hash, command.sequence);
			hash = fold_u64(hash, static_cast<uint64_t>(command.submitted_at.ticks()));
			hash = fold_string(hash, command.actor_id);
			hash = fold_string(hash, command.command_type);
			hash = fold_u64(hash, static_cast<uint64_t>(command.payload.size()));
			for (uint8_t byte : command.payload) {
				hash = fold_byte(hash, byte);
			}
		}

		hash = fold_u64(hash, static_cast<uint64_t>(ecs_identity.slots.size()));
		for (auto const& slot : ecs_identity.slots) {
			hash = fold_u64(hash, slot.generation);
			hash = fold_byte(hash, slot.immutable ? uint8_t { 1 } : uint8_t { 0 });
		}

		hash = fold_u64(hash, static_cast<uint64_t>(ecs_identity.free_list.size()));
		for (uint32_t index : ecs_identity.free_list) {
			hash = fold_u64(hash, index);
		}
		return hash;
	}
};

}