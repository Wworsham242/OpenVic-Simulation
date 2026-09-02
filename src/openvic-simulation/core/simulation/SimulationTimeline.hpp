#pragma once

#include "openvic-simulation/core/simulation/SimTime.hpp"
#include "openvic-simulation/core/simulation/SimulationEventScheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace OpenVic {

struct SimulationTimelineSnapshot {
	static constexpr uint32_t CURRENT_SCHEMA_VERSION = 1;

	uint32_t schema_version = CURRENT_SCHEMA_VERSION;
	SimTime current_time = SimTime::from_ticks(0);
	uint64_t next_event_sequence = 0;
	std::vector<ScheduledSimulationEvent> events;

	bool operator==(SimulationTimelineSnapshot const&) const = default;

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
		auto fold_i64 = [&](uint64_t hash, int64_t value) {
			return fold_u64(hash, static_cast<uint64_t>(value));
		};
		auto fold_string = [&](uint64_t hash, std::string const& value) {
			hash = fold_u64(hash, static_cast<uint64_t>(value.size()));
			for (unsigned char byte : value) {
				hash = fold_byte(hash, static_cast<uint8_t>(byte));
			}
			return hash;
		};
		auto fold_bytes = [&](uint64_t hash, std::vector<uint8_t> const& value) {
			hash = fold_u64(hash, static_cast<uint64_t>(value.size()));
			for (uint8_t byte : value) {
				hash = fold_byte(hash, byte);
			}
			return hash;
		};

		uint64_t hash = FNV_OFFSET;
		hash = fold_u64(hash, static_cast<uint64_t>(schema_version));
		hash = fold_i64(hash, current_time.ticks());
		hash = fold_u64(hash, next_event_sequence);
		hash = fold_u64(hash, static_cast<uint64_t>(events.size()));

		for (ScheduledSimulationEvent const& event : events) {
			hash = fold_i64(hash, event.at.ticks());
			hash = fold_u64(hash, event.sequence);
			hash = fold_string(hash, event.payload.type_id);
			hash = fold_bytes(hash, event.payload.data);
		}
		return hash;
	}
};

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

	[[nodiscard]] SimulationTimelineSnapshot capture_snapshot() const {
		return SimulationTimelineSnapshot {
			.schema_version = SimulationTimelineSnapshot::CURRENT_SCHEMA_VERSION,
			.current_time = current_time_value,
			.next_event_sequence = event_scheduler.next_sequence(),
			.events = event_scheduler.snapshot_events()
		};
	}

	[[nodiscard]] bool restore_snapshot(SimulationTimelineSnapshot const& snapshot) {
		if (snapshot.schema_version != SimulationTimelineSnapshot::CURRENT_SCHEMA_VERSION) {
			return false;
		}

		auto restored = SimulationEventScheduler::restore(
			snapshot.next_event_sequence,
			snapshot.events
		);
		if (!restored.has_value()) {
			return false;
		}

		current_time_value = snapshot.current_time;
		event_scheduler = std::move(*restored);
		return true;
	}

	[[nodiscard]] bool restore(
		SimTime time,
		uint64_t next_sequence,
		std::vector<ScheduledSimulationEvent> const& events
	) {
		return restore_snapshot(SimulationTimelineSnapshot {
			.schema_version = SimulationTimelineSnapshot::CURRENT_SCHEMA_VERSION,
			.current_time = time,
			.next_event_sequence = next_sequence,
			.events = events
		});
	}
};

}