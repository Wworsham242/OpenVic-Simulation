#pragma once

#include "openvic-simulation/core/simulation/OrderedCommandRuntime.hpp"
#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"

#include <vector>

namespace OpenVic {

/// Live continuity package for the generalized runtime pieces currently owned by InstanceManager.
///
/// This is intentionally narrower than CampaignStateSnapshot. It does not claim to contain
/// deterministic RNG, ECS identity, or domain-store state that InstanceManager does not yet own
/// through the generalized persistence seam.
struct LiveCommandTimelineSnapshot {
	SimulationTimelineSnapshot timeline;
	CampaignReplayState replay;
	std::vector<CampaignCommandRecord> command_log;

	bool operator==(LiveCommandTimelineSnapshot const&) const = default;

	[[nodiscard]] bool is_canonical() const {
		SimulationTimeline timeline_probe;
		if (!timeline_probe.restore_snapshot(timeline)) {
			return false;
		}

		OrderedCommandRuntime command_probe;
		return command_probe.restore(replay, command_log);
	}
};

class LiveCommandTimelineState final {
public:
	[[nodiscard]] static LiveCommandTimelineSnapshot capture(
		SimulationTimeline const& timeline,
		OrderedCommandRuntime const& commands
	) {
		return LiveCommandTimelineSnapshot {
			.timeline = timeline.capture_snapshot(),
			.replay = commands.capture_replay_state(),
			.command_log = commands.capture_command_log()
		};
	}

	/// Transactional two-owner restore.
	///
	/// Both candidate objects are validated and restored first. Live owners are mutated only
	/// after both candidates succeed.
	[[nodiscard]] static bool restore(
		LiveCommandTimelineSnapshot const& snapshot,
		SimulationTimeline& timeline,
		OrderedCommandRuntime& commands
	) {
		SimulationTimeline timeline_candidate = timeline;
		OrderedCommandRuntime command_candidate = commands;

		if (!timeline_candidate.restore_snapshot(snapshot.timeline)) {
			return false;
		}
		if (!command_candidate.restore(snapshot.replay, snapshot.command_log)) {
			return false;
		}

		timeline = std::move(timeline_candidate);
		commands = std::move(command_candidate);
		return true;
	}
};

}