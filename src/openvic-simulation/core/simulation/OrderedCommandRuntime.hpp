#pragma once

#include "openvic-simulation/core/simulation/CampaignStateSnapshot.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace OpenVic {

class OrderedCommandRuntime final {
private:
	std::vector<CampaignCommandRecord> accepted_commands;
	uint64_t replay_cursor_value = 0;

public:
	[[nodiscard]] uint64_t accepted_command_count() const {
		return static_cast<uint64_t>(accepted_commands.size());
	}

	[[nodiscard]] uint64_t replay_cursor() const {
		return replay_cursor_value;
	}

	[[nodiscard]] std::optional<uint64_t> accept(
		SimTime submitted_at,
		std::string actor_id,
		std::string command_type,
		std::string jurisdiction_id,
		std::vector<uint8_t> payload
	) {
		if (actor_id.empty() || command_type.empty() || jurisdiction_id.empty()) {
			return std::nullopt;
		}

		const uint64_t sequence = static_cast<uint64_t>(accepted_commands.size());
		accepted_commands.push_back(CampaignCommandRecord {
			.sequence = sequence,
			.submitted_at = submitted_at,
			.actor_id = std::move(actor_id),
			.command_type = std::move(command_type),
			.jurisdiction_id = std::move(jurisdiction_id),
			.payload = std::move(payload)
		});
		return sequence;
	}

	[[nodiscard]] std::optional<CampaignCommandRecord> peek_next_replay() const {
		if (replay_cursor_value >= accepted_commands.size()) {
			return std::nullopt;
		}
		return accepted_commands[static_cast<std::size_t>(replay_cursor_value)];
	}

	[[nodiscard]] std::optional<CampaignCommandRecord> consume_next_replay() {
		if (replay_cursor_value >= accepted_commands.size()) {
			return std::nullopt;
		}
		return accepted_commands[static_cast<std::size_t>(replay_cursor_value++)];
	}

	void reset_replay_cursor() {
		replay_cursor_value = 0;
	}

	[[nodiscard]] CampaignReplayState capture_replay_state() const {
		return CampaignReplayState {
			.accepted_command_count = static_cast<uint64_t>(accepted_commands.size()),
			.replay_cursor = replay_cursor_value
		};
	}

	[[nodiscard]] std::vector<CampaignCommandRecord> capture_command_log() const {
		return accepted_commands;
	}

	[[nodiscard]] bool restore(
		CampaignReplayState replay,
		std::vector<CampaignCommandRecord> const& command_log
	) {
		if (replay.accepted_command_count != command_log.size()
			|| replay.replay_cursor > replay.accepted_command_count) {
			return false;
		}

		for (std::size_t i = 0; i < command_log.size(); ++i) {
			CampaignCommandRecord const& command = command_log[i];
			if (command.sequence != i || command.actor_id.empty()
				|| command.command_type.empty() || command.jurisdiction_id.empty()) {
				return false;
			}
		}

		accepted_commands = command_log;
		replay_cursor_value = replay.replay_cursor;
		return true;
	}
};

}