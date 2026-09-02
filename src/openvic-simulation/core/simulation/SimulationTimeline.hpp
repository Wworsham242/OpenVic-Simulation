#pragma once

#include "openvic-simulation/core/simulation/SimTime.hpp"
#include "openvic-simulation/core/simulation/SimulationEventScheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace OpenVic {

/// Authoritative owner for generalized elapsed simulation time and scheduled wake-ups.
///
/// FOUNDATION-003 introduces this as a runtime seam only. Existing Victoria gameplay
/// remains driven by InstanceManager's Date-based daily loop until later migration
/// proves parity. No domain system reads this timeline yet.
class SimulationTimeline final {
private:
	SimTime current_time_value = SimTime::from_ticks(0);
	SimulationEventScheduler event_scheduler;

public:
	[[nodiscard]] SimTime current_time() const {
		return current_time_value;
	}

	[[nodiscard]] bool advance(int64_t ticks) {
		if (ticks < 0) {
			return false;
		}

		const std::optional<SimTime> advanced = current_time_value.checked_advance(ticks);
		if (!advanced.has_value()) {
			return false;
		}

		current_time_value = *advanced;
		return true;
	}

	[[nodiscard]] std::optional<uint64_t> schedule_event(
		SimTime at,
		SimulationEventPayload payload
	) {
		return event_scheduler.schedule(at, std::move(payload));
	}

	[[nodiscard]] std::optional<ScheduledSimulationEvent> pop_due_event() {
		return event_scheduler.pop_due(current_time_value);
	}

	[[nodiscard]] size_t scheduled_event_count() const {
		return event_scheduler.size();
	}

	[[nodiscard]] uint64_t next_event_sequence() const {
		return event_scheduler.next_sequence();
	}

	[[nodiscard]] std::vector<ScheduledSimulationEvent> snapshot_events() const {
		return event_scheduler.snapshot_events();
	}

	[[nodiscard]] bool restore(
		SimTime time,
		uint64_t next_sequence,
		std::vector<ScheduledSimulationEvent> const& events
	) {
		auto restored = SimulationEventScheduler::restore(next_sequence, events);
		if (!restored.has_value()) {
			return false;
		}

		current_time_value = time;
		event_scheduler = std::move(*restored);
		return true;
	}
};

}