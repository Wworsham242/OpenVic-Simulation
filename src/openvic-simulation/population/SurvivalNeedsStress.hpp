#pragma once

#include <algorithm>
#include <cstdint>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
/*
 * Coarse population-level exposure to sustained failure to acquire
 * life needs.
 *
 * This is deliberately not a mortality or famine model.
 *
 * The input is yesterday's authoritative native POP life-needs
 * fulfillment. A first-order distributed lag prevents a single bad
 * market day from being treated like prolonged deprivation.
 *
 * The 30-day response period is an initial simulation calibration
 * timescale, not a claim of a universal biological threshold.
 */
struct SurvivalNeedsStressUpdate {
fixed_point_t previous_exposure = fixed_point_t::_0;
fixed_point_t life_needs_fulfilled = fixed_point_t::_1;
fixed_point_t daily_deficit = fixed_point_t::_0;
fixed_point_t exposure = fixed_point_t::_0;

bool operator==(SurvivalNeedsStressUpdate const&) const = default;
};

[[nodiscard]] inline SurvivalNeedsStressUpdate update_survival_needs_stress(
fixed_point_t previous_exposure,
fixed_point_t life_needs_fulfilled
) {
constexpr int32_t RESPONSE_DAYS = 30;

previous_exposure = std::clamp(
previous_exposure,
fixed_point_t::_0,
fixed_point_t::_1
);

life_needs_fulfilled = std::clamp(
life_needs_fulfilled,
fixed_point_t::_0,
fixed_point_t::_1
);

const fixed_point_t daily_deficit =
fixed_point_t::_1 - life_needs_fulfilled;

fixed_point_t exposure =
previous_exposure
+ (daily_deficit - previous_exposure) / fixed_point_t { RESPONSE_DAYS };

exposure = std::clamp(
exposure,
fixed_point_t::_0,
fixed_point_t::_1
);

return {
previous_exposure,
life_needs_fulfilled,
daily_deficit,
exposure
};
}
}
