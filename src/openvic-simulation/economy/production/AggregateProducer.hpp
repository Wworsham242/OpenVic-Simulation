#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/types/fixed_point/FixedPointMap.hpp"

namespace OpenVic {

struct AggregateProductionResult final {
	fixed_point_t desired_output = 0;
	fixed_point_t actual_output = 0;
	bool input_limited = false;
	// Capacity-stage facts, before inputs are consumed. Potential includes
	// utilization but excludes labor/input limits; it is not external demand.
	fixed_point_t installed_capacity = 0;
	fixed_point_t utilization = 0;
	fixed_point_t potential_output = 0;
	bool workforce_enabled = false;
	fixed_point_t workforce_required = 0;
	fixed_point_t available_workforce = 0;
	fixed_point_t labor_supported_capacity = 0;
	std::optional<fixed_point_t> input_supported_output;
	std::optional<fixed_point_t> external_supported_output;
	bool external_limited = false;
	bool labor_limited = false;
	// The installed ceiling participates in min(installed, labor-supported).
	// This does not claim unmet expansion demand, which this producer lacks.
	bool installed_ceiling_active = false;
	bool utilization_limited = false;
	bool input_below_potential = false;

	bool operator==(AggregateProductionResult const&) const = default;
};

class AggregateProducer final {
private:
	std::string identifier;
	ProductionType const& production_type;
	fixed_point_t capacity = 0;
	fixed_point_t utilization = 0;
	bool workforce_constraint_enabled = false;
	fixed_point_t available_workforce = 0;
	std::optional<fixed_point_t> external_output_ceiling;
	fixed_point_map_t<GoodDefinition const*> inventory;

	[[nodiscard]] static fixed_point_t clamp_nonnegative(fixed_point_t value) {
		return std::max(value, fixed_point_t::_0);
	}

	[[nodiscard]] static fixed_point_t clamp_unit(fixed_point_t value) {
		return std::clamp(value, fixed_point_t::_0, fixed_point_t::_1);
	}

public:
	AggregateProducer(
		std::string_view new_identifier,
		ProductionType const& new_production_type,
		fixed_point_t new_capacity,
		fixed_point_t new_utilization
	) : identifier { new_identifier },
		production_type { new_production_type },
		capacity { clamp_nonnegative(new_capacity) },
		utilization { clamp_unit(new_utilization) } {}

	[[nodiscard]] std::string_view get_identifier() const { return identifier; }
	[[nodiscard]] ProductionType const& get_production_type() const { return production_type; }
	[[nodiscard]] fixed_point_t get_capacity() const { return capacity; }
	void set_capacity(fixed_point_t value) { capacity = clamp_nonnegative(value); }
	[[nodiscard]] fixed_point_t get_utilization() const { return utilization; }
	void set_utilization(fixed_point_t value) { utilization = clamp_unit(value); }

	[[nodiscard]] fixed_point_t get_available_workforce() const {
		return available_workforce;
	}

	void set_available_workforce(fixed_point_t value) {
		available_workforce = clamp_nonnegative(value);
		workforce_constraint_enabled = true;
	}

	void clear_available_workforce_constraint() {
		available_workforce = fixed_point_t::_0;
		workforce_constraint_enabled = false;
	}

	[[nodiscard]] bool is_workforce_constrained() const {
		return workforce_constraint_enabled;
	}

	void set_external_output_ceiling(fixed_point_t value) {
		external_output_ceiling = clamp_nonnegative(value);
	}

	void clear_external_output_ceiling() {
		external_output_ceiling.reset();
	}

	[[nodiscard]] std::optional<fixed_point_t> get_external_output_ceiling() const {
		return external_output_ceiling;
	}

	[[nodiscard]] fixed_point_t calculate_labor_supported_capacity() const {
		if (!workforce_constraint_enabled) {
			return capacity;
		}

		const fixed_point_t workforce_per_capacity =
			fixed_point_t { type_safe::get(production_type.base_workforce_size) };

		if (workforce_per_capacity <= fixed_point_t::_0) {
			return fixed_point_t::_0;
		}

		return available_workforce / workforce_per_capacity;
	}

	[[nodiscard]] fixed_point_t get_inventory(GoodDefinition const& good) const {
		auto const it = inventory.find(&good);
		return it != inventory.end() ? it->second : fixed_point_t::_0;
	}

	void set_inventory(GoodDefinition const& good, fixed_point_t quantity) {
		inventory[&good] = clamp_nonnegative(quantity);
	}

	void add_inventory(GoodDefinition const& good, fixed_point_t quantity) {
		fixed_point_t& current = inventory[&good];
		current = clamp_nonnegative(current + quantity);
	}

	[[nodiscard]] fixed_point_t calculate_desired_output() const {
		const fixed_point_t effective_capacity = std::min(
			capacity,
			calculate_labor_supported_capacity()
		);

		fixed_point_t desired =
			production_type.base_output_quantity *
			effective_capacity *
			utilization;

		if (external_output_ceiling.has_value()) {
			desired = std::min(desired, *external_output_ceiling);
		}

		return desired;
	}

	[[nodiscard]] AggregateProductionResult produce() {
		const fixed_point_t desired_output = calculate_desired_output();
		AggregateProductionResult result {
			.desired_output = desired_output,
			.actual_output = desired_output,
			.input_limited = false
		};
		result.installed_capacity = capacity;
		result.utilization = utilization;
		result.potential_output = production_type.base_output_quantity * capacity * utilization;
		result.workforce_enabled = workforce_constraint_enabled;
		result.workforce_required = capacity * fixed_point_t { type_safe::get(production_type.base_workforce_size) };
		result.available_workforce = available_workforce;
		result.labor_supported_capacity = calculate_labor_supported_capacity();
		result.external_supported_output = external_output_ceiling;
		result.external_limited =
			external_output_ceiling.has_value() &&
			*external_output_ceiling <
				production_type.base_output_quantity *
				std::min(capacity, result.labor_supported_capacity) *
				utilization;
		result.labor_limited =
			production_type.base_output_quantity *
				std::min(capacity, result.labor_supported_capacity) *
				utilization <
			result.potential_output;
		result.installed_ceiling_active = capacity <= result.labor_supported_capacity;
		result.utilization_limited = utilization < fixed_point_t::_1
			&& production_type.base_output_quantity * capacity > fixed_point_t::_0;
		// Retain the independent input ceiling even when labor is tighter or
		// output is zero. This read-only pass does not affect production choices.
		for (auto const& [good, input_per_output] : production_type.input_goods) {
			if (input_per_output <= 0) { continue; }
			fixed_point_t const feasible = get_inventory(*good) / input_per_output;
			if (!result.input_supported_output || feasible < *result.input_supported_output) {
				result.input_supported_output = feasible;
			}
		}
		result.input_below_potential = result.input_supported_output.has_value()
			&& *result.input_supported_output < result.potential_output;

		if (result.actual_output <= 0) {
			result.actual_output = 0;
			return result;
		}

		for (auto const& [good, input_per_output] : production_type.input_goods) {
			if (input_per_output <= 0) { continue; }
			const fixed_point_t feasible = get_inventory(*good) / input_per_output;
			if (feasible < result.actual_output) {
				result.actual_output = feasible;
				result.input_limited = true;
			}
		}

		result.actual_output = clamp_nonnegative(result.actual_output);

		for (auto const& [good, input_per_output] : production_type.input_goods) {
			if (input_per_output <= 0) { continue; }
			fixed_point_t& available = inventory[good];
			available = clamp_nonnegative(available - input_per_output * result.actual_output);
		}

		inventory[&production_type.output_good] += result.actual_output;
		return result;
	}
};

}
