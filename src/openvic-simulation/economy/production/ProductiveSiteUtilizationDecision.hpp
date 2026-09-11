#pragma once

#include <algorithm>
#include <array>
#include <optional>

#include "openvic-simulation/scripts/RestrictedCalculation.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

struct ProductiveSiteUtilizationDecisionPolicy final {
    fixed_point_t minimum_utilization = fixed_point_t::_0;
    fixed_point_t maximum_utilization = fixed_point_t::_1;
    fixed_point_t adjustment_rate = fixed_point_t::_0_25;
    fixed_point_t surplus_for_full_response = fixed_point_t { 100 };
    fixed_point_t profitability_weight = fixed_point_t::_0_25;

    std::optional<RestrictedCalculationDefinition>
        profitability_pressure_formula;

    [[nodiscard]] bool is_valid() const {
        return
            minimum_utilization >= fixed_point_t::_0 &&
            maximum_utilization <= fixed_point_t::_1 &&
            maximum_utilization >= minimum_utilization &&
            adjustment_rate >= fixed_point_t::_0 &&
            adjustment_rate <= fixed_point_t::_1 &&
            surplus_for_full_response > fixed_point_t::_0 &&
            profitability_weight >= fixed_point_t::_0 &&
            profitability_weight <= fixed_point_t::_1 &&
            (
                !profitability_pressure_formula.has_value() ||
                (
                    profitability_pressure_formula->is_valid() &&
                    profitability_pressure_formula->get_input_count() == 2
                )
            );
    }

    bool operator==(ProductiveSiteUtilizationDecisionPolicy const&) const = default;
};

struct ProductiveSiteUtilizationDecisionInput final {
    fixed_point_t prior_utilization = fixed_point_t::_0;
    fixed_point_t operating_surplus = fixed_point_t::_0;
};

struct ProductiveSiteUtilizationDecisionResult final {
    fixed_point_t normalized_surplus_signal = fixed_point_t::_0;
    fixed_point_t profitability_pressure = fixed_point_t::_0;
    fixed_point_t unconstrained_target = fixed_point_t::_0;
    fixed_point_t constrained_target = fixed_point_t::_0;
    fixed_point_t next_utilization = fixed_point_t::_0;

    bool operator==(ProductiveSiteUtilizationDecisionResult const&) const = default;
};

class ProductiveSiteUtilizationDecision final {
public:
    [[nodiscard]] static ProductiveSiteUtilizationDecisionResult calculate(
        ProductiveSiteUtilizationDecisionInput const& input,
        ProductiveSiteUtilizationDecisionPolicy const& policy
    ) {
        fixed_point_t const prior = std::clamp(
            input.prior_utilization,
            fixed_point_t::_0,
            fixed_point_t::_1
        );

        fixed_point_t const normalized_signal = std::clamp(
            input.operating_surplus / policy.surplus_for_full_response,
            -fixed_point_t::_1,
            fixed_point_t::_1
        );

        fixed_point_t profitability_pressure =
            normalized_signal * policy.profitability_weight;

        if (policy.profitability_pressure_formula.has_value()) {
            std::array<fixed_point_t, 2> const formula_inputs {
                normalized_signal,
                policy.profitability_weight
            };

            auto const formula_result =
                policy.profitability_pressure_formula->evaluate(
                    formula_inputs
                );

            if (!formula_result.has_value()) {
                return {};
            }

            profitability_pressure = *formula_result;
        }

        fixed_point_t const unconstrained_target =
            prior + profitability_pressure;

        fixed_point_t const constrained_target = std::clamp(
            unconstrained_target,
            policy.minimum_utilization,
            policy.maximum_utilization
        );

        fixed_point_t const next = std::clamp(
            prior + (constrained_target - prior) * policy.adjustment_rate,
            policy.minimum_utilization,
            policy.maximum_utilization
        );

        return ProductiveSiteUtilizationDecisionResult {
            .normalized_surplus_signal = normalized_signal,
            .profitability_pressure = profitability_pressure,
            .unconstrained_target = unconstrained_target,
            .constrained_target = constrained_target,
            .next_utilization = next
        };
    }
};

}
