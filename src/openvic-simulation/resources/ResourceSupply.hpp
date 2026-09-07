#pragma once

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/// Domain-owned resource supply state exposed through a narrow typed coupling.
///
/// Nominal supply is the physical/source capability before external constraints.
/// Availability is a dimensionless [0, 1] fraction which can represent causes
/// such as depletion, weather, sanctions, outage, extraction disruption, or
/// another resource/energy-domain mechanism.
///
/// This object does not decide *why* availability changed. The owning domain
/// computes that. Consumers receive only the resulting accessible flow.
struct ResourceSupplyState final {
	fixed_point_t nominal_per_tick = 0;
	fixed_point_t availability_fraction = fixed_point_t::_1;

	[[nodiscard]] bool is_valid() const {
		return nominal_per_tick >= fixed_point_t::_0
			&& availability_fraction >= fixed_point_t::_0
			&& availability_fraction <= fixed_point_t::_1;
	}

	[[nodiscard]] bool set_availability_fraction(fixed_point_t value) {
		if (value < fixed_point_t::_0 || value > fixed_point_t::_1) {
			return false;
		}
		availability_fraction = value;
		return true;
	}

	[[nodiscard]] fixed_point_t accessible_per_tick() const {
		return nominal_per_tick * availability_fraction;
	}
};

}