#pragma once

#include <algorithm>
#include <string>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/// Logistics-domain result for one resource source.
///
/// The resource domain owns physical source capability. Logistics owns whether
/// and how much of that source can reach the consuming node. This structure is
/// the typed coupling between those domains.
struct ResourceSourceAccess final {
	std::string source_id;
	fixed_point_t delivery_capacity = 0;
	fixed_point_t accessible_fraction = fixed_point_t::_1;
	bool access_allowed = true;

	[[nodiscard]] bool is_valid() const {
		return !source_id.empty()
			&& delivery_capacity >= fixed_point_t::_0
			&& accessible_fraction >= fixed_point_t::_0
			&& accessible_fraction <= fixed_point_t::_1;
	}

	[[nodiscard]] fixed_point_t constrain(fixed_point_t physical_supply) const {
		if (
			!access_allowed ||
			physical_supply <= fixed_point_t::_0 ||
			delivery_capacity <= fixed_point_t::_0 ||
			accessible_fraction <= fixed_point_t::_0
		) {
			return fixed_point_t::_0;
		}

		fixed_point_t const accessible =
			physical_supply * accessible_fraction;

		return std::min(accessible, delivery_capacity);
	}
};

}