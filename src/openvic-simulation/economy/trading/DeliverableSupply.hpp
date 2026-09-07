#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/// Quantity-only interface between physical supply and the market-clearing kernel.
///
/// This is deliberately not a logistics model. Transport networks, sanctions,
/// tariffs, border rules, insurance, infrastructure, and geography remain
/// separate causal mechanisms. They may contribute to the values in this
/// envelope, but the envelope only answers:
///
/// "Of the physical supply that exists, how much is deliverable to this buyer?"
struct DeliverableSupply final {
	fixed_point_t physical_supply = 0;
	fixed_point_t accessible_fraction = fixed_point_t::_1;
	fixed_point_t delivery_capacity = fixed_point_t::usable_max;
	bool access_allowed = true;
	bool operator==(DeliverableSupply const&) const = default;

	[[nodiscard]] bool is_delivery_limited() const {
		return calculate_deliverable_quantity() < physical_supply;
	}

	[[nodiscard]] fixed_point_t calculate_deliverable_quantity() const {
		if (!access_allowed || physical_supply <= 0 || accessible_fraction <= 0 || delivery_capacity <= 0) {
			return 0;
		}

		const fixed_point_t bounded_fraction = std::clamp(
			accessible_fraction,
			fixed_point_t::_0,
			fixed_point_t::_1
		);
		const fixed_point_t fraction_accessible = physical_supply * bounded_fraction;

		return std::max(
			std::min(fraction_accessible, delivery_capacity),
			fixed_point_t::_0
		);
	}
};

}
