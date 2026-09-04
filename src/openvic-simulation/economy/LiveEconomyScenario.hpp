#pragma once

#include <vector>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/trading/TransportCorridor.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {

struct LiveEconomyScenarioDefinition final {
	ProductionType const* upstream_process = nullptr;
	ProductionType const* downstream_process = nullptr;

	fixed_point_t upstream_capacity = 0;
	fixed_point_t upstream_utilization = fixed_point_t::_1;
	fixed_point_t downstream_capacity = 0;
	fixed_point_t downstream_utilization = fixed_point_t::_1;

	GoodDefinition const* source_inflow_good = nullptr;
	fixed_point_t source_inflow_per_daily_tick = 0;

	market_node_index_t source_node {};
	market_node_index_t destination_node {};
	std::vector<TransportLeg> corridor_legs;

	[[nodiscard]] bool is_valid() const {
		if (
			upstream_process == nullptr ||
			downstream_process == nullptr ||
			source_inflow_good == nullptr ||
			upstream_capacity < 0 ||
			downstream_capacity < 0 ||
			source_inflow_per_daily_tick < 0 ||
			corridor_legs.empty()
		) {
			return false;
		}

		if (
			!upstream_process->input_goods.contains(source_inflow_good) ||
			!downstream_process->input_goods.contains(&upstream_process->output_good)
		) {
			return false;
		}

		return true;
	}
};

}