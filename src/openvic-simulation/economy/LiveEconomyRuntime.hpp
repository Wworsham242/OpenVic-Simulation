#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "openvic-simulation/core/simulation/Cadence.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/LiveEconomyScenario.hpp"
#include "openvic-simulation/economy/LiveEconomyProvenance.hpp"
#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"
#include "openvic-simulation/economy/production/WorkforceAllocation.hpp"
#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"
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

struct ResourceAlternativeRoute final {
	std::string source_id;
	std::string route_id;
	TransportCorridor corridor;
	bool access_allowed = true;
	fixed_point_t accessible_fraction = fixed_point_t::_1;

	ResourceAlternativeRoute(
		std::string new_source_id,
		std::string new_route_id,
		TransportCorridor new_corridor,
		bool new_access_allowed = true,
		fixed_point_t new_accessible_fraction = fixed_point_t::_1
	) : source_id { std::move(new_source_id) },
		route_id { std::move(new_route_id) },
		corridor { std::move(new_corridor) },
		access_allowed { new_access_allowed },
		accessible_fraction { new_accessible_fraction } {}

	[[nodiscard]] fixed_point_t effective_capacity() const {
		if (!access_allowed) {
			return fixed_point_t::_0;
		}
		return corridor.calculate_bottleneck_capacity() * accessible_fraction;
	}
};

struct ResourceGraphRoute final {
	std::string source_id;
	market_node_index_t source_node {};
	market_node_index_t destination_node {};
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
	bool operator==(LiveEconomyStatus const&) const = default;
};

class LiveEconomyRuntime final {
private:
	GameRulesManager const& game_rules_manager;
	GoodInstanceManager& good_instance_manager;
	LiveEconomyScenarioDefinition const& scenario;

	ResourceSupplyNetwork source_network;
	std::vector<ResourceSourceRoute> resource_routes;
	std::vector<ResourceAlternativeRoute> alternative_resource_routes;
	LogisticsGraph logistics_graph;
	std::vector<ResourceGraphRoute> resource_graph_routes;
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
	bool record_provenance;
	std::optional<LiveEconomyCycleProvenance> pending_provenance;
	std::optional<LiveEconomyCycleProvenance> completed_provenance;
	std::optional<ProductiveSiteBinding> upstream_site;
	std::optional<WorkforceAllocationResult> preallocated_upstream_workforce;

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

		// Add usable alternate-route capacity after primary/shared constraints.
		for (ResourceAlternativeRoute const& alternate : alternative_resource_routes) {
			if (!alternate.access_allowed) {
				continue;
			}

			for (ResourceSourceAccess& source_access : access) {
				if (source_access.source_id == alternate.source_id) {
					source_access.delivery_capacity += alternate.effective_capacity();
					source_access.access_allowed = true;
					break;
				}
			}
		}

		// Batch graph-routed flows so independently selected routes compete for
		// every shared graph edge they actually use.
		std::vector<LogisticsGraphFlowRequest> graph_requests;
		graph_requests.reserve(resource_graph_routes.size());

		for (ResourceGraphRoute const& graph_route : resource_graph_routes) {
			graph_requests.push_back(LogisticsGraphFlowRequest {
				.flow_id = graph_route.source_id,
				.source = graph_route.source_node,
				.destination = graph_route.destination_node,
				.requested =
					source_network.source_accessible_supply_per_tick(
						graph_route.source_id
					)
			});
		}

		auto const graph_allocations =
			logistics_graph.allocate_flows(graph_requests);

		for (LogisticsGraphFlowAllocation const& allocation : graph_allocations) {
			for (ResourceSourceAccess& source_access : access) {
				if (source_access.source_id == allocation.flow_id) {
					source_access.delivery_capacity = allocation.allocated;
					source_access.access_allowed = allocation.path.found;
					break;
				}
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
	// Migration mapping only: SimTime itself remains unitless.
	static constexpr Cadence DAILY_CADENCE = *Cadence::create(24);

	LiveEconomyRuntime(
		GameRulesManager const& new_game_rules_manager,
		GoodInstanceManager& new_good_instance_manager,
		LiveEconomyScenarioDefinition const& new_scenario,
		bool new_record_provenance = true
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
		}, record_provenance { new_record_provenance } {

		for (TransportLeg const& leg : new_scenario.corridor_legs) {
			corridor.add_leg(leg);
		}

		status.configured = new_scenario.is_valid();
		refresh_status_from_market();
	}

	[[nodiscard]] bool set_source_resource_availability(fixed_point_t availability_fraction) {
		return set_resource_source_availability("scenario_source", availability_fraction);
	}

	/// Bind the upstream producer's authoritative capacity to an inherited
	/// OpenVic facility/capacity asset at a specific installed level.
	[[nodiscard]] bool set_upstream_available_workforce(
		fixed_point_t available_workforce
	) {
		if (available_workforce < fixed_point_t::_0) {
			return false;
		}
		upstream.set_available_workforce(available_workforce);
		return true;
	}

[[nodiscard]] AggregateProducer& get_upstream_producer_for_workforce_allocation() {
return upstream;
}

[[nodiscard]] ProductiveSiteBinding const* get_upstream_site_binding() const {
return upstream_site ? &*upstream_site : nullptr;
}

void set_preallocated_upstream_workforce(
WorkforceAllocationResult allocation
) {
preallocated_upstream_workforce = allocation;
}
	[[nodiscard]] bool bind_upstream_site(ProductiveSiteBinding binding, MapInstance& map) {
		if (!binding.resolve(map, upstream.get_production_type())) { return false; }
		upstream_site = std::move(binding);
		return true;
	}

	// Called by the A6 prepare callback after map_tick: province-local labor
	// is post-RGO residual unemployment during this migration, not a permanent
	// priority policy. Re-resolve both capacity and workforce every due cycle.
	[[nodiscard]] std::optional<WorkforcePool> prepare_upstream_site(MapInstance& map) {
		if (!upstream_site) { return std::nullopt; }
		auto site = upstream_site->resolve(map, upstream.get_production_type());
		if (!site) {
			// A missing/mismatched world facility must not reuse stale capacity.
			upstream.set_capacity(0);
			return WorkforcePool { std::span<Pop> {} };
		}
		upstream.set_capacity(site->installed_capacity);
		return site->workforce;
	}

	[[nodiscard]] bool set_upstream_capacity_from_facility(
		BuildingType const& facility,
		building_level_t installed_level
	) {
		if (
			!facility.is_setting_general_capacity_asset() ||
			facility.production_type == nullptr ||
			facility.production_type != &upstream.get_production_type() ||
			installed_level < building_level_t { 0 } ||
			installed_level > facility.max_level
		) {
			return false;
		}

		upstream.set_capacity(
			facility.calculate_installed_capacity(installed_level)
		);
		return true;
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

	[[nodiscard]] bool configure_resource_alternative_routes(
		std::vector<ResourceAlternativeRoute> routes
	) {
		for (ResourceAlternativeRoute const& route : routes) {
			if (route.source_id.empty() || route.route_id.empty()) {
				return false;
			}
		}

		alternative_resource_routes = std::move(routes);
		return true;
	}

	[[nodiscard]] bool set_resource_alternative_route_access(
		std::string_view route_id,
		bool access_allowed
	) {
		for (ResourceAlternativeRoute& route : alternative_resource_routes) {
			if (route.route_id == route_id) {
				route.access_allowed = access_allowed;
				return true;
			}
		}
		return false;
	}
	[[nodiscard]] bool configure_logistics_graph(
		std::vector<LogisticsGraphEdge> edges
	) {
		return logistics_graph.configure(std::move(edges));
	}

	[[nodiscard]] bool configure_resource_graph_routes(
		std::vector<ResourceGraphRoute> routes
	) {
		for (ResourceGraphRoute const& route : routes) {
			if (route.source_id.empty()) {
				return false;
			}
		}

		resource_graph_routes = std::move(routes);
		return true;
	}

	[[nodiscard]] bool set_logistics_graph_edge_open(
		std::string_view edge_id,
		bool open
	) {
		return logistics_graph.set_edge_open(edge_id, open);
	}

	/// Consume the cadence boundaries in (previous, current] for one successful
	/// timeline advance. Call once per advance, using consecutive intervals.
	/// Cadence owns periodic timing; no event queue or timing cursor is duplicated.
	/// prepare_workforce runs once per due boundary and returns A5's optional POP
	/// pool after employment reset/availability preparation. clear_market must use
	/// the caller's authoritative market, including all other producers' orders.
	template<typename PrepareWorkforce, typename ClearMarket>
	void run_due_daily_cycles(
		SimTime previous, SimTime current,
		PrepareWorkforce&& prepare_workforce, ClearMarket&& clear_market
	) {
		for (auto due = DAILY_CADENCE.next_after(previous);
			due.has_value() && *due <= current;
			due = DAILY_CADENCE.next_after(*due)) {
			pre_market_daily_tick(prepare_workforce(*due));
			if (pending_provenance) {
				pending_provenance->due_time = *due;
			}
			clear_market();
			post_market_daily_tick();
		}
	}

	// Supply the current local POP pool after the day's employment reset.
	// An omitted pool preserves the existing externally configured workforce;
	// an explicitly empty pool means no workers. No POP references are retained.
	void pre_market_daily_tick(std::optional<WorkforcePool> upstream_pops = std::nullopt) {
		pending_provenance.reset();
		if (record_provenance) {
			pending_provenance.emplace();
			pending_provenance->productive_site = upstream_site;
			pending_provenance->upstream_process_id = upstream.get_production_type().get_identifier();
			pending_provenance->downstream_process_id = downstream.get_production_type().get_identifier();
			pending_provenance->intermediate_good_id = intermediate_good.get_identifier();
		}
		upstream_bridge.reset_cycle_result();
		downstream_bridge.reset_cycle_result();
		if (upstream_pops.has_value()) {
WorkforceAllocationResult allocation;
(void)allocate_producer_workforce_from_pool(
upstream, *upstream_pops, &allocation
);
if (pending_provenance) {
pending_provenance->workforce = allocation;
}
preallocated_upstream_workforce.reset();
} else if (preallocated_upstream_workforce.has_value()) {
if (pending_provenance) {
pending_provenance->workforce =
*preallocated_upstream_workforce;
}
preallocated_upstream_workforce.reset();
}
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
		if (pending_provenance) {
			pending_provenance->resource = source_flow;
			pending_provenance->upstream = upstream_result;
		}

		const fixed_point_t physical_intermediate =
			upstream.get_inventory(intermediate_good);

		corridor.publish_access(
			access_table,
			physical_intermediate
		);
		if (pending_provenance) {
			pending_provenance->logistics = *access_table.get_access(
				corridor.get_source_node(), corridor.get_destination_node()
			);
		}

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
		if (pending_provenance) {
			pending_provenance->cycle = status.completed_daily_ticks;
			pending_provenance->upstream_market = upstream_bridge.get_cycle_result();
			pending_provenance->downstream_market = downstream_bridge.get_cycle_result();
			pending_provenance->market_price = status.intermediate_price;
			pending_provenance->downstream = downstream_result;
			completed_provenance = std::move(pending_provenance);
			pending_provenance.reset();
		}
	}

	[[nodiscard]] std::optional<LiveEconomyCycleProvenance> const& get_latest_provenance() const {
		return completed_provenance;
	}

	[[nodiscard]] LiveEconomyStatus get_status() const {
		return status;
	}
};

}
