#pragma once

#include "openvic-simulation/environment/ProvinceEnvironmentalState.hpp"

namespace OpenVic {

	// Value facts for one native production cycle, not another production ledger.
	struct AgriculturalProductionConstraintResult {
		ProvinceEnvironmentalState environment;
		fixed_point_t yield_factor = fixed_point_t::_1;
		fixed_point_t unconstrained_output = 0;
		fixed_point_t constrained_output = 0;

		bool operator==(AgriculturalProductionConstraintResult const&) const = default;
	};

	// First-stage aggregate yield constraint, not a crop model. Future physical
	// inputs may include soil moisture, temperature stress, crop sensitivity,
	// weather timing, irrigation and disease/pests; none are simulated here.
	[[nodiscard]] inline AgriculturalProductionConstraintResult constrain_agricultural_production(
		ProvinceEnvironmentalState const& environment, fixed_point_t unconstrained_output
	) {
		const fixed_point_t factor = environment.get_water_availability();
		// Decompose the nonnegative raw quantity before multiplication so even
		// fixed_point_t::max cannot overflow. Equivalent to floor(raw * factor).
		const auto raw = std::max(unconstrained_output, fixed_point_t::_0).get_raw_value();
		const auto scale = fixed_point_t::_1.get_raw_value();
		const auto factor_raw = factor.get_raw_value();
		const fixed_point_t constrained = fixed_point_t::parse_raw(
			(raw / scale) * factor_raw + (raw % scale) * factor_raw / scale
		);
		return { environment, factor, unconstrained_output, constrained };
	}
}
