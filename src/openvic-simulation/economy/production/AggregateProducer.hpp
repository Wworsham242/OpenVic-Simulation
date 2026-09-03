#pragma once

#include <algorithm>
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
};

class AggregateProducer final {
private:
	std::string identifier;
	ProductionType const& production_type;
	fixed_point_t capacity = 0;
	fixed_point_t utilization = 0;
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
		return production_type.base_output_quantity * capacity * utilization;
	}

	[[nodiscard]] AggregateProductionResult produce() {
		const fixed_point_t desired_output = calculate_desired_output();
		AggregateProductionResult result {
			.desired_output = desired_output,
			.actual_output = desired_output,
			.input_limited = false
		};

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