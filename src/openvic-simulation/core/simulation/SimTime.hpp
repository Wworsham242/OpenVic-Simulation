#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <optional>

namespace OpenVic {

/// Scenario-agnostic monotonically ordered simulation time.
///
/// A tick is intentionally unitless at the engine layer. A ruleset may choose
/// to interpret one tick as an hour or another fixed quantum without making
/// calendar days an execution primitive.
class SimTime final {
private:
	int64_t ticks_value = 0;

public:
	constexpr SimTime() = default;
	explicit constexpr SimTime(int64_t ticks) : ticks_value { ticks } {}

	[[nodiscard]] static constexpr SimTime from_ticks(int64_t ticks) {
		return SimTime { ticks };
	}

	[[nodiscard]] constexpr int64_t ticks() const {
		return ticks_value;
	}

	[[nodiscard]] std::optional<SimTime> checked_advance(int64_t delta) const {
		if (delta > 0 && ticks_value > std::numeric_limits<int64_t>::max() - delta) {
			return std::nullopt;
		}
		if (delta < 0 && ticks_value < std::numeric_limits<int64_t>::min() - delta) {
			return std::nullopt;
		}
		return SimTime { ticks_value + delta };
	}

	constexpr auto operator<=>(SimTime const&) const = default;
};

}