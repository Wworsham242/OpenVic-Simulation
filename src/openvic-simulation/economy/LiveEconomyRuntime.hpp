#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/LiveEconomyScenario.hpp"
#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"
#include "openvic-simulation/economy/trading/MarketNodeAccess.hpp"
#include "openvic-simulation/economy/trading/SharedTransportCapacity.hpp"
#include "openvic-simulation/economy/trading/TransportCorridor.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/resources/ResourceSupply.hpp"
#include "openvic-simulation/resources/ResourceSupplyNetwork.hpp"

namespace OpenVic {

struct ResourceSourceRoute final {
	std::string source_id;
	TransportCorridor corridor;
	bool access_allowed = true;
	fixed_point_t accessible_fraction = fixed_point_t::_1;
	std::string shared_capacity_id;

	ResourceSourceRoute(
		std::string new_source_id,
		TransportCorridor new_corridor,
		bool new_access_allowed = true,
		fixed_point_t new_accessible_fraction = fixed_point_t::_1,
		std::string new_shared_capacity_id = {}
	) : source_id { std::move(new_source_id) },
		corridor { std::move(new_corridor) },
		access_allowed { new_access_allowed },
		accessible_fraction { new_accessible_fraction },
		shared_capacity_id { std::move(new_shared_capacity_id) } {}
};

struct LiveEconomyStatus final {
	bool configured = false;
	uint64_t completed_daily_ticks = 0;

	fixed_point_t source_nominal_inflow = 0;
	fixed_point_t source_availability_fraction = fixed_point_t::_1;
	fixed_point_t source_accessible_inflow = 0;
	fixed_point_t source_buffer_inventory = 0;
	fixed_point_t source_buffer_draw = 0;
	fixed_point_t source_unmet_inflow = 0;
	size_t source_count = 0;

	fixed_point_t upstream_output = 0;
	fixed_point_t downstream_desired_output = 0;
	fixed_point_t downstream_output = 0;
	bool downstream_input_limited = false;

	fixed_point_t intermediate_upstream_inventory = 0;
	fixed_point_t intermediate_downstream_inventory = 0;
	fixed_point_t final_inventory = 0;

	fixed_point_t corridor_capacity = 0;
	fixed_point_t deliverable_intermediate = 0;

	fixed_point_t intermediate_price = 0;
	fixed_point_t intermediate_supply_yesterday = 0;
	fixed_point_t intermediate_demand_yesterday = 0;
	fixed_point_t intermediate_quantity_traded_yesterday = 0;
};

class LiveEconomyRuntime final {
private:
	GameRulesManager const& game_rules_manager;
	GoodInstanceManager& good_instance_manager;
	LiveEconomyScenarioDefinition const& scenario;

	ResourceSupplyNetwork source_network;
	std::vector<ResourceSourceRoute> resource_routes;
	std::vector<SharedTransportCapacity> shared_transport_capacities;

	GoodDefinition const& intermediate_good;
	GoodDefinition const& final_good;

	AggregateProducer upstream;
	AggregateProducer downstream;

	AggregateProducerMarketBridge upstream_bridge;
	AggregateProducerMarketBridge downstream_bridge;

	MarketNodeAccessTable access_table;
	TransportCorridor corridor;

	LiveEconomyStatus status {};

	[[nodiscard]] std::vector<ResourceSourceAccess> build_resource_source_access() const {
		std::vector<ResourceSourceAccess> access;
		access.reserve(resource_routes.size());

		for (ResourceSourceRoute const& route : resource_routes) {
			access.push_back(ResourceSourceAccess {
				.source_id = route.source_id,
				.delivery_capacity = route.corridor.calculate_bottleneck_capacity(),
				.accessible_fraction = route.accessible_fraction,
				.access_allowed = route.access_allowed
			});
		}

		for (SharedTransportCapacity const& shared : shared_transport_capacities) {
			std::vector<SharedTransportRequest> requests;
			std::vector<size_t> matching_indices;

			for (size_t i = 0; i < resource_routes.size(); ++i) {
				ResourceSourceRoute const& route = resource_routes[i];
				if (route.shared_capacity_id == shared.get_capacity_id()) {
					requests.push_back(SharedTransportRequest {
						.flow_id = route.source_id,
						.requested = access[i].delivery_capacity
					});
					matching_indices.push_back(i);
				}
			}

			auto const allocations = shared.allocate(requests);

			for (size_t i = 0; i < allocations.size(); ++i) {
				size_t const route_index = matching_indices[i];
				access[route_index].delivery_capacity = std::min(
					access[route_index].delivery_capacity,
					allocations[i].allocated
				);
			}
		}

		return access;
	}

	void refresh_status_from_market() {
		status.source_nominal_inflow = source_network.nominal_supply_per_tick();
		status.source_accessible_inflow = source_network.accessible_supply_per_tick();
		status.source_availability_fraction =
			status.source_nominal_inflow > fixed_point_t::_0
				? status.source_accessible_inflow / status.source_nominal_inflow
				: fixed_point_t::_1;
		status.source_buffer_inventory = source_network.buffer_inventory();
		status.source_count = source_network.source_count();

		GoodInstance& market =
			good_instance_manager.get_good_instance_by_definition(intermediate_good);

		status.intermediate_upstream_inventory =
			upstream.get_inventory(intermediate_good);
		status.intermediate_downstream_inventory =
			downstream.get_inventory(intermediate_good);
		status.final_inventory =
			downstream.get_inventory(final_good);

		status.corridor_capacity =
			corridor.calculate_bottleneck_capacity();
		status.deliverable_intermediate =
			access_table.calculate_deliverable_quantity(
				corridor.get_source_node(),
				corridor.get_destination_node()
			);

		status.intermediate_price = market.get_price();
		status.intermediate_supply_yesterday = market.get_total_supply_yesterday();
		status.intermediate_demand_yesterday = market.get_total_demand_yesterday();
		status.intermediate_quantity_traded_yesterday =
			market.get_quantity_traded_yesterday();
	}

public:
	LiveEconomyRuntime(
		GameRulesManager const& new_game_rules_manager,
		GoodInstanceManager& new_good_instance_manager,
		LiveEconomyScenarioDefinition const& new_scenario
	) : game_rules_manager { new_game_rules_manager },
		good_instance_manager { new_good_instance_manager },
		scenario { new_scenario },
		source_network {
			{
				ResourceSourceState {
					.source_id = "scenario_source",
					.node = new_scenario.source_node,
					.supply = ResourceSupplyState {
						.nominal_per_tick = new_scenario.source_inflow_per_daily_tick,
						.availability_fraction = fixed_point_t::_1
					}
				}
			}
		},
		intermediate_good { new_scenario.upstream_process->output_good },
		final_good { new_scenario.downstream_process->output_good },
		upstream {
			"live_scenario_upstream",
			*new_scenario.upstream_process,
			new_scenario.upstream_capacity,
			new_scenario.upstream_utilization
		},
		downstream {
			"live_scenario_downstream",
			*new_scenario.downstream_process,
			new_scenario.downstream_capacity,
			new_scenario.downstream_utilization
		},
		upstream_bridge { upstream },
		downstream_bridge { downstream },
		corridor {
			new_scenario.source_node,
			new_scenario.destination_node
		} {

		for (TransportLeg const& leg : new_scenario.corridor_legs) {
			corridor.add_leg(leg);
		}

		status.configured = new_scenario.is_valid();
		refresh_status_from_market();
	}

	[[nodiscard]] bool set_source_resource_availability(fixed_point_t availability_fraction) {
		return set_resource_source_availability("scenario_source", availability_fraction);
	}

	[[nodiscard]] bool configure_resource_supply_network(
		std::vector<ResourceSourceState> sources,
		ResourceBufferState buffer
	) {
		ResourceSupplyNetwork candidate { std::move(sources), buffer };
		if (!candidate.is_valid()) {
			return false;
		}
		source_network = std::move(candidate);
		refresh_status_from_market();
		return true;
	}

	[[nodiscard]] bool set_resource_source_availability(
		std::string_view source_id,
		fixed_point_t availability_fraction
	) {
		if (!source_network.set_source_availability(source_id, availability_fraction)) {
			return false;
		}
		refresh_status_from_market();
		return true;
	}

	[[nodiscard]] bool configure_resource_source_routes(
		std::vector<ResourceSourceRoute> routes
	) {
		for (ResourceSourceRoute const& route : routes) {
			if (route.source_id.empty()) {
				return false;
			}
		}
		resource_routes = std::move(routes);
		return true;
	}

	[[nodiscard]] bool set_resource_route_access(
		std::string_view source_id,
		bool access_allowed
	) {
		for (ResourceSourceRoute& route : resource_routes) {
			if (route.source_id == source_id) {
				route.access_allowed = access_allowed;
				return true;
			}
		}
		return false;
	}

	[[nodiscard]] bool configure_shared_transport_capacities(
		std::vector<SharedTransportCapacity> capacities
	) {
		for (SharedTransportCapacity const& capacity : capacities) {
			if (!capacity.is_valid()) {
				return false;
			}
		}

		shared_transport_capacities = std::move(capacities);
		return true;
	}

	void pre_market_daily_tick() {
		ResourceFlowResult const source_flow =
			source_network.fulfill(
				scenario.source_inflow_per_daily_tick,
				build_resource_source_access()
			);

		status.source_buffer_draw = source_flow.buffer_draw;
		status.source_unmet_inflow = source_flow.unmet;

		upstream.add_inventory(
			*scenario.source_inflow_good,
			source_flow.delivered
		);

		const AggregateProductionResult upstream_result = upstream.produce();
		status.upstream_output = upstream_result.actual_output;

		const fixed_point_t physical_intermediate =
			upstream.get_inventory(intermediate_good);

		corridor.publish_access(
			access_table,
			physical_intermediate
		);

		const fixed_point_t deliverable =
			access_table.calculate_deliverable_quantity(
				corridor.get_source_node(),
				corridor.get_destination_node()
			);

		GoodInstance& market =
			good_instance_manager.get_good_instance_by_definition(intermediate_good);

		if (auto sell_order = upstream_bridge.make_output_sell_order();
			sell_order.has_value()) {
			market.add_market_sell_order(std::move(*sell_order));
		}

		const fixed_point_t shortfall =
			downstream_bridge.calculate_input_shortfall(intermediate_good);

		if (auto buy_order = downstream_bridge.make_input_buy_order(
				intermediate_good,
				shortfall * market.get_max_next_price(),
				std::nullopt,
				deliverable
			); buy_order.has_value()) {
			market.add_buy_up_to_order(std::move(*buy_order));
		}

		refresh_status_from_market();
	}

	void post_market_daily_tick() {
		upstream_bridge.clear_completed_orders();
		downstream_bridge.clear_completed_orders();

		const AggregateProductionResult downstream_result = downstream.produce();

		status.downstream_desired_output =
			downstream_result.desired_output;
		status.downstream_output =
			downstream_result.actual_output;
		status.downstream_input_limited =
			downstream_result.input_limited;

		++status.completed_daily_ticks;
		refresh_status_from_market();
	}

	[[nodiscard]] LiveEconomyStatus get_status() const {
		return status;
	}
};

}
