#pragma once

#include <algorithm>
#include <cstdint>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	/*
	 * Initial simulation calibration anchors.
	 *
	 * These are not WHO clinical constants. They represent different
	 * characteristic timescales for deterioration and recovery while
	 * richer age-, disease-, and treatment-specific health mechanics
	 * are not yet present.
	 */
	inline constexpr int32_t NUTRITION_HEALTH_DETERIORATION_DAYS = 30;
	inline constexpr int32_t NUTRITION_HEALTH_RECOVERY_DAYS = 60;

	struct NutritionHealthBurdenUpdate {
		fixed_point_t previous_burden = fixed_point_t::_0;
		fixed_point_t health_vulnerability_pressure = fixed_point_t::_0;
		fixed_point_t response_days = fixed_point_t::_0;
		fixed_point_t daily_change = fixed_point_t::_0;
		fixed_point_t burden = fixed_point_t::_0;

		bool operator==(NutritionHealthBurdenUpdate const&) const = default;
	};

	[[nodiscard]] inline NutritionHealthBurdenUpdate update_nutrition_health_burden(
		fixed_point_t previous_burden,
		fixed_point_t health_vulnerability_pressure
	) {
		previous_burden = std::clamp(
			previous_burden,
			fixed_point_t::_0,
			fixed_point_t::_1
		);

		health_vulnerability_pressure = std::clamp(
			health_vulnerability_pressure,
			fixed_point_t::_0,
			fixed_point_t::_1
		);

		if (health_vulnerability_pressure == previous_burden) {
			return {
				previous_burden,
				health_vulnerability_pressure,
				fixed_point_t::_0,
				fixed_point_t::_0,
				previous_burden
			};
		}

		const fixed_point_t response_days =
			health_vulnerability_pressure > previous_burden
				? fixed_point_t { NUTRITION_HEALTH_DETERIORATION_DAYS }
				: fixed_point_t { NUTRITION_HEALTH_RECOVERY_DAYS };

		const fixed_point_t daily_change =
			(health_vulnerability_pressure - previous_burden)
			/ response_days;

		const fixed_point_t burden = std::clamp(
			previous_burden + daily_change,
			fixed_point_t::_0,
			fixed_point_t::_1
		);

		return {
			previous_burden,
			health_vulnerability_pressure,
			response_days,
			daily_change,
			burden
		};
	}
}
