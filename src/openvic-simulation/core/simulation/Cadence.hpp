#pragma once

#include "openvic-simulation/core/simulation/SimTime.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace OpenVic {

/// Deterministic periodic timing without owning execution or event state.
///
/// Cadence answers when normal periodic work is due. It is deliberately
/// separate from both the ECS scheduler (which orders work) and the event
/// scheduler (which owns exceptional/scheduled wake-ups).
class Cadence final {
private:
	int64_t period_ticks_value = 1;
	int64_t phase_ticks_value = 0;

	constexpr Cadence(int64_t period_ticks, int64_t phase_ticks)
		: period_ticks_value { period_ticks }, phase_ticks_value { phase_ticks } {}

	static constexpr uint64_t fnv1a(std::string_view text) {
		uint64_t hash = 0xcbf29ce484222325ULL;
		for (char value : text) {
			hash ^= static_cast<uint8_t>(value);
			hash *= 0x100000001b3ULL;
		}
		return hash;
	}

	static constexpr uint64_t mix64(uint64_t value) {
		value ^= value >> 30;
		value *= 0xbf58476d1ce4e5b9ULL;
		value ^= value >> 27;
		value *= 0x94d049bb133111ebULL;
		return value ^ (value >> 31);
	}

	static constexpr int64_t floor_mod(int64_t value, int64_t divisor) {
		const int64_t remainder = value % divisor;
		return remainder < 0 ? remainder + divisor : remainder;
	}

public:
	[[nodiscard]] static constexpr std::optional<Cadence> create(int64_t period_ticks, int64_t phase_ticks = 0) {
		if (period_ticks <= 0 || phase_ticks < 0 || phase_ticks >= period_ticks) {
			return std::nullopt;
		}
		return Cadence { period_ticks, phase_ticks };
	}

	/// Produces a stable phase for one semantic key and domain.
	///
	/// This prevents large populations of systems/entities from all becoming
	/// due on the same global boundary while remaining deterministic.
	[[nodiscard]] static constexpr std::optional<Cadence> staggered(
		int64_t period_ticks,
		uint64_t stable_key,
		std::string_view domain
	) {
		if (period_ticks <= 0 || domain.empty()) {
			return std::nullopt;
		}
		const uint64_t period = static_cast<uint64_t>(period_ticks);
		const uint64_t phase = mix64(stable_key ^ fnv1a(domain)) % period;
		return Cadence { period_ticks, static_cast<int64_t>(phase) };
	}

	[[nodiscard]] constexpr int64_t period_ticks() const {
		return period_ticks_value;
	}

	[[nodiscard]] constexpr int64_t phase_ticks() const {
		return phase_ticks_value;
	}

	[[nodiscard]] constexpr bool is_due(SimTime time) const {
		return floor_mod(time.ticks(), period_ticks_value) == phase_ticks_value;
	}

	[[nodiscard]] std::optional<SimTime> next_at_or_after(SimTime time) const {
		const int64_t current_phase = floor_mod(time.ticks(), period_ticks_value);
		const int64_t delta = floor_mod(phase_ticks_value - current_phase, period_ticks_value);
		return time.checked_advance(delta);
	}

	[[nodiscard]] std::optional<SimTime> next_after(SimTime time) const {
		const std::optional<SimTime> next_tick = time.checked_advance(1);
		if (!next_tick.has_value()) {
			return std::nullopt;
		}
		return next_at_or_after(*next_tick);
	}
};

}