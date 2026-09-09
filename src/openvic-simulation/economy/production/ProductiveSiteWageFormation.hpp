#pragma once

#include <algorithm>

#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

struct ProductiveSiteWageFormationPolicy final {
	fixed_point_t minimum_compensation = fixed_point_t::_0;
	fixed_point_t maximum_compensation = fixed_point_t::usable_max;

	// Fraction of the gap between prior compensation and the newly implied
	// target applied each completed cycle. 1 = immediate, 0 = frozen.
	fixed_point_t adjustment_rate = fixed_point_t::_0_25;

	// Unfilled labor demand raises compensation relative to the current wage
	// scale. A value of 1 means a 100% shortage adds one current-wage unit
	// to the unconstrained target.
	fixed_point_t scarcity_weight = fixed_point_t::_1;

	// Share of realized operating surplus/loss per allocated worker passed
	// into the wage target. This is monetary, not the labor-offer priority.
	fixed_point_t operating_surplus_share = fixed_point_t::_0_10;

	// Pull toward the highest compensation paid by another active local
	// productive site.
	fixed_point_t competition_weight = fixed_point_t::_0_25;

	[[nodiscard]] bool is_valid() const {
		return
			minimum_compensation >= fixed_point_t::_0 &&
			maximum_compensation >= minimum_compensation &&
			adjustment_rate >= fixed_point_t::_0 &&
			adjustment_rate <= fixed_point_t::_1 &&
			scarcity_weight >= fixed_point_t::_0 &&
			operating_surplus_share >= fixed_point_t::_0 &&
			competition_weight >= fixed_point_t::_0 &&
			competition_weight <= fixed_point_t::_1;
	}

	bool operator==(ProductiveSiteWageFormationPolicy const&) const = default;
};

struct ProductiveSiteWageFormationInput final {
	fixed_point_t prior_compensation = fixed_point_t::_0;
	fixed_point_t requested_workforce = fixed_point_t::_0;
	fixed_point_t allocated_workforce = fixed_point_t::_0;
	fixed_point_t operating_surplus = fixed_point_t::_0;
	fixed_point_t highest_competing_compensation = fixed_point_t::_0;
};

struct ProductiveSiteWageFormationResult final {
	fixed_point_t scarcity_ratio = fixed_point_t::_0;
	fixed_point_t scarcity_pressure = fixed_point_t::_0;
	fixed_point_t operating_surplus_per_worker = fixed_point_t::_0;
	fixed_point_t profitability_pressure = fixed_point_t::_0;
	fixed_point_t competition_pressure = fixed_point_t::_0;
	fixed_point_t unconstrained_target = fixed_point_t::_0;
	fixed_point_t constrained_target = fixed_point_t::_0;
	fixed_point_t next_compensation = fixed_point_t::_0;

	bool operator==(ProductiveSiteWageFormationResult const&) const = default;
};

class ProductiveSiteWageFormation final {
public:
	[[nodiscard]] static ProductiveSiteWageFormationResult calculate(
		ProductiveSiteWageFormationInput const& input,
		ProductiveSiteWageFormationPolicy const& policy
	) {
		fixed_point_t const prior =
			std::max(input.prior_compensation, fixed_point_t::_0);
		fixed_point_t const requested =
			std::max(input.requested_workforce, fixed_point_t::_0);
		fixed_point_t const allocated =
			std::max(input.allocated_workforce, fixed_point_t::_0);

		fixed_point_t scarcity_ratio = fixed_point_t::_0;
		if (requested > fixed_point_t::_0) {
			scarcity_ratio = std::clamp(
				(requested - std::min(allocated, requested)) / requested,
				fixed_point_t::_0,
				fixed_point_t::_1
			);
		}

		// If a site starts at zero compensation, scarcity still needs a
		// monetary scale capable of moving the wage off zero.
		fixed_point_t const wage_scale =
			std::max(prior, fixed_point_t::_1);

		fixed_point_t const scarcity_pressure =
			wage_scale * scarcity_ratio * policy.scarcity_weight;

		fixed_point_t surplus_per_worker = fixed_point_t::_0;
		if (allocated >= fixed_point_t::_1) {
			surplus_per_worker =
				input.operating_surplus / allocated;
		}

		fixed_point_t const profitability_pressure =
			surplus_per_worker * policy.operating_surplus_share;

		fixed_point_t const competition_pressure =
			std::max(
				input.highest_competing_compensation - prior,
				fixed_point_t::_0
			) * policy.competition_weight;

		fixed_point_t const unconstrained =
			prior +
			scarcity_pressure +
			profitability_pressure +
			competition_pressure;

		fixed_point_t const constrained = std::clamp(
			unconstrained,
			policy.minimum_compensation,
			policy.maximum_compensation
		);

		fixed_point_t const next =
			prior +
			policy.adjustment_rate * (constrained - prior);

		return ProductiveSiteWageFormationResult {
			.scarcity_ratio = scarcity_ratio,
			.scarcity_pressure = scarcity_pressure,
			.operating_surplus_per_worker = surplus_per_worker,
			.profitability_pressure = profitability_pressure,
			.competition_pressure = competition_pressure,
			.unconstrained_target = unconstrained,
			.constrained_target = constrained,
			.next_compensation = std::clamp(
				next,
				policy.minimum_compensation,
				policy.maximum_compensation
			)
		};
	}
};

}