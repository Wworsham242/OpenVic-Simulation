#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {
	/*
	 * Derived downstream pressures from sustained failure to acquire
	 * basic survival needs.
	 *
	 * These are inputs to later health, migration, and political/conflict
	 * models. They are not mortality probabilities, migration decisions,
	 * rebellion probabilities, or war probabilities.
	 */
	struct BasicResourceStressResponse {
		fixed_point_t survival_stress = fixed_point_t::_0;

		/* Nutrition-side exposure for a future health model. */
		fixed_point_t health_vulnerability_pressure = fixed_point_t::_0;

		/*
		 * Incentive or aspiration to leave the current location.
		 * Actual migration also requires destination utility and the
		 * physical, financial, and legal ability to move.
		 */
		fixed_point_t mobility_push_pressure = fixed_point_t::_0;

		/*
		 * Existing social/political susceptibility used to moderate
		 * resource-related instability.
		 */
		fixed_point_t instability_susceptibility = fixed_point_t::_0;

		/*
		 * Contribution of survival stress to instability.
		 * This is not conflict-onset probability.
		 */
		fixed_point_t instability_pressure = fixed_point_t::_0;

		bool operator==(BasicResourceStressResponse const&) const = default;
	};

	[[nodiscard]] inline BasicResourceStressResponse calculate_basic_resource_stress_response(
		fixed_point_t survival_stress,
		fixed_point_t instability_susceptibility
	) {
		survival_stress = std::clamp(
			survival_stress,
			fixed_point_t::_0,
			fixed_point_t::_1
		);

		instability_susceptibility = std::clamp(
			instability_susceptibility,
			fixed_point_t::_0,
			fixed_point_t::_1
		);

		return {
			survival_stress,
			survival_stress,
			survival_stress,
			instability_susceptibility,
			survival_stress * instability_susceptibility
		};
	}
}
