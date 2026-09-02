#pragma once

#include "openvic-simulation/core/simulation/SimTime.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace OpenVic {

struct SimulationEventPayload {
	std::string type_id;
	std::vector<uint8_t> data;

	bool operator==(SimulationEventPayload const&) const = default;
};

struct ScheduledSimulationEvent {
	SimTime at;
	uint64_t sequence = 0;
	SimulationEventPayload payload;

	bool operator==(ScheduledSimulationEvent const&) const = default;
};

/// Deterministic owner of exceptional/scheduled simulation wake-ups.
///
/// Equal-time events are ordered by stable monotonic sequence number. This
/// class does not execute events and does not replace the ECS system DAG.
class SimulationEventScheduler final {
private:
	using key_t = std::pair<SimTime, uint64_t>;

	uint64_t next_sequence_value = 0;
	std::map<key_t, SimulationEventPayload> events_by_key;

public:
	[[nodiscard]] std::optional<uint64_t> schedule(SimTime at, SimulationEventPayload payload) {
		if (next_sequence_value == std::numeric_limits<uint64_t>::max()) {
			return std::nullopt;
		}
		const uint64_t sequence = next_sequence_value++;
		events_by_key.emplace(key_t { at, sequence }, std::move(payload));
		return sequence;
	}

	[[nodiscard]] bool contains(SimTime at, SimulationEventPayload const& payload) const {
		auto iterator = events_by_key.lower_bound(key_t { at, 0 });
		while (iterator != events_by_key.end() && iterator->first.first == at) {
			if (iterator->second == payload) {
				return true;
			}
			++iterator;
		}
		return false;
	}

	[[nodiscard]] bool has_due_type(SimTime now, std::string const& type_id) const {
		for (auto iterator = events_by_key.begin();
			iterator != events_by_key.end() && iterator->first.first <= now;
			++iterator) {
			if (iterator->second.type_id == type_id) {
				return true;
			}
		}
		return false;
	}

	[[nodiscard]] std::optional<ScheduledSimulationEvent> pop_due(SimTime now) {
		if (events_by_key.empty()) {
			return std::nullopt;
		}
		auto iterator = events_by_key.begin();
		if (iterator->first.first > now) {
			return std::nullopt;
		}

		ScheduledSimulationEvent event {
			.at = iterator->first.first,
			.sequence = iterator->first.second,
			.payload = std::move(iterator->second)
		};
		events_by_key.erase(iterator);
		return event;
	}

	[[nodiscard]] std::optional<ScheduledSimulationEvent> pop_due_type(SimTime now, std::string const& type_id) {
		for (auto iterator = events_by_key.begin();
			iterator != events_by_key.end() && iterator->first.first <= now;
			++iterator) {
			if (iterator->second.type_id != type_id) {
				continue;
			}

			ScheduledSimulationEvent event {
				.at = iterator->first.first,
				.sequence = iterator->first.second,
				.payload = std::move(iterator->second)
			};
			events_by_key.erase(iterator);
			return event;
		}
		return std::nullopt;
	}

	[[nodiscard]] uint64_t next_sequence() const {
		return next_sequence_value;
	}

	[[nodiscard]] size_t size() const {
		return events_by_key.size();
	}

	[[nodiscard]] bool empty() const {
		return events_by_key.empty();
	}

	[[nodiscard]] std::vector<ScheduledSimulationEvent> snapshot_events() const {
		std::vector<ScheduledSimulationEvent> result;
		result.reserve(events_by_key.size());
		for (auto const& [key, payload] : events_by_key) {
			result.push_back(ScheduledSimulationEvent {
				.at = key.first,
				.sequence = key.second,
				.payload = payload
			});
		}
		return result;
	}

	/// Restores only canonical event state.
	///
	/// Input must be strictly increasing by (time, sequence), and every stored
	/// sequence must be below the next sequence cursor.
	[[nodiscard]] static std::optional<SimulationEventScheduler> restore(
		uint64_t next_sequence,
		std::vector<ScheduledSimulationEvent> const& events
	) {
		SimulationEventScheduler scheduler;
		scheduler.next_sequence_value = next_sequence;

		std::optional<key_t> previous;
		for (ScheduledSimulationEvent const& event : events) {
			const key_t key { event.at, event.sequence };
			if (event.sequence >= next_sequence || (previous.has_value() && key <= *previous)) {
				return std::nullopt;
			}
			if (!scheduler.events_by_key.emplace(key, event.payload).second) {
				return std::nullopt;
			}
			previous = key;
		}
		return scheduler;
	}
};

}