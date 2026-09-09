#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

	// Physical truth owned by ProvinceInstance, independent of ownership and modifiers.
	// Water available to vegetation as a fraction of sufficient water: 0 = none,
	// 1 = sufficient. This aggregate input does not model rainfall or irrigation.
	struct ProvinceEnvironmentalState {
	private:
		fixed_point_t water_availability = fixed_point_t::_1;

	public:
		ProvinceEnvironmentalState() = default;
		explicit ProvinceEnvironmentalState(fixed_point_t available_water)
			: water_availability { std::clamp(available_water, fixed_point_t::_0, fixed_point_t::_1) } {}

		[[nodiscard]] fixed_point_t get_water_availability() const { return water_availability; }
		bool operator==(ProvinceEnvironmentalState const&) const = default;
	};
}
