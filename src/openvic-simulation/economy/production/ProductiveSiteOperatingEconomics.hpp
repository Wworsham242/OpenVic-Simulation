#pragma once

#include <algorithm>
#include <optional>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/// One bounded operating-economics view for a productive site.
///
/// market_revenue and market_cash_spend are settled cash facts from the
/// existing market bridge. Material replacement, electricity, and logistics remain imputed operating-cost
/// signals. labor_compensation_cost is actual gross compensation once B16 wage
/// settlement is configured through the authoritative workforce allocator.
struct ProductiveSiteOperatingEconomicsResult final {
	fixed_point_t actual_output = fixed_point_t::_0;

	fixed_point_t market_revenue = fixed_point_t::_0;
	fixed_point_t market_cash_spend = fixed_point_t::_0;

	fixed_point_t material_replacement_cost = fixed_point_t::_0;
	fixed_point_t labor_compensation_cost = fixed_point_t::_0;
	fixed_point_t electricity_cost_proxy = fixed_point_t::_0;
	fixed_point_t logistics_cost_proxy = fixed_point_t::_0;

	fixed_point_t total_operating_cost = fixed_point_t::_0;
	fixed_point_t operating_surplus = fixed_point_t::_0;
	std::optional<fixed_point_t> operating_cost_per_output;

	bool operator==(ProductiveSiteOperatingEconomicsResult const&) const = default;
};

class ProductiveSiteOperatingEconomics final {
public:
	[[nodiscard]] static ProductiveSiteOperatingEconomicsResult calculate(
		fixed_point_t actual_output,
		fixed_point_t market_revenue,
		fixed_point_t market_cash_spend,
		fixed_point_t material_replacement_cost,
		fixed_point_t labor_compensation_cost,
		fixed_point_t electricity_cost_proxy,
		fixed_point_t logistics_cost_proxy
	) {
		ProductiveSiteOperatingEconomicsResult result {
			.actual_output = std::max(actual_output, fixed_point_t::_0),
			.market_revenue = std::max(market_revenue, fixed_point_t::_0),
			.market_cash_spend = std::max(market_cash_spend, fixed_point_t::_0),
			.material_replacement_cost =
				std::max(material_replacement_cost, fixed_point_t::_0),
			.labor_compensation_cost =
				std::max(labor_compensation_cost, fixed_point_t::_0),
			.electricity_cost_proxy =
				std::max(electricity_cost_proxy, fixed_point_t::_0),
			.logistics_cost_proxy =
				std::max(logistics_cost_proxy, fixed_point_t::_0)
		};

		result.total_operating_cost =
			result.market_cash_spend +
			result.material_replacement_cost +
			result.labor_compensation_cost +
			result.electricity_cost_proxy +
			result.logistics_cost_proxy;

		result.operating_surplus =
			result.market_revenue - result.total_operating_cost;

		if (result.actual_output > fixed_point_t::_0) {
			result.operating_cost_per_output =
				result.total_operating_cost / result.actual_output;
		}

		return result;
	}

	[[nodiscard]] static fixed_point_t labor_offer_from_prior_economics(
		ProductiveSiteOperatingEconomicsResult const& economics,
		fixed_point_t prior_allocated_workforce,
		fixed_point_t no_history_offer = fixed_point_t::_1
	) {
		if (prior_allocated_workforce < fixed_point_t::_1) {
			return no_history_offer;
		}

		return std::max(
			economics.operating_surplus,
			fixed_point_t::_0
		) / prior_allocated_workforce;
	}
};

}