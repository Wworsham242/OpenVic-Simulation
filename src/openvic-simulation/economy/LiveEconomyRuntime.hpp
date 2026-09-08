#pragma once

#include <memory>

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
#include "openvic-simulation/map/MapInstance.hpp"
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

struct ProductiveSiteResourceRoute final {
	std::string employer_id;
	market_node_index_t source_node {};
	market_node_index_t destination_node {};
	GoodDefinition const* good = nullptr;
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

    struct BoundUpstreamSiteState final {
        ProductiveSiteBinding binding;
        std::string employer_id;
        AggregateProducer producer;
        AggregateProducerMarketBridge bridge;

        fixed_point_t requested_workforce = fixed_point_t::_0;
        std::optional<WorkforceAllocationResult> current_allocation;
        std::optional<WorkforceAllocationResult> previous_allocation;
        AggregateProductionResult last_production {};
        AggregateMarketCycleResult last_market {};

        static std::string make_employer_id(
            ProductiveSiteBinding const& binding
        ) {
            std::string result = "site:";
            result += binding.province_id;
            result += ":";
            result += binding.building_id;
            return result;
        }

        BoundUpstreamSiteState(
            ProductiveSiteBinding new_binding,
            ProductionType const& production_type,
            fixed_point_t utilization
        ) :
            binding { std::move(new_binding) },
            employer_id { make_employer_id(binding) },
            producer {
                employer_id,
                production_type,
                fixed_point_t::_0,
                utilization
            },
            bridge { producer } {}

        [[nodiscard]] fixed_point_t get_labor_offer(
            fixed_point_t no_history_offer = fixed_point_t::_1
        ) const {
            if (
                !previous_allocation.has_value() ||
                previous_allocation->allocated < fixed_point_t::_1
            ) {
                return no_history_offer;
            }

            fixed_point_t const operating_surplus = std::max(
                last_market.money_received - last_market.money_spent,
                fixed_point_t::_0
            );

            return operating_surplus / previous_allocation->allocated;
        }
    };

    struct AdditionalResourceInputState final {
        GoodDefinition const* good = nullptr;
        fixed_point_t inflow_per_daily_tick = fixed_point_t::_0;
        std::string source_id;
        ResourceSupplyNetwork network;
    };

    ResourceSupplyNetwork source_network;
    std::vector<AdditionalResourceInputState> additional_resource_inputs;
	std::vector<ResourceSourceRoute> resource_routes;
	std::vector<ResourceAlternativeRoute> alternative_resource_routes;
	LogisticsGraph logistics_graph;
	std::vector<ResourceGraphRoute> resource_graph_routes;
	std::vector<ProductiveSiteResourceRoute> upstream_site_resource_routes;
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
    std::string upstream_employer_id;
    fixed_point_t upstream_requested_workforce = fixed_point_t::_0;
    std::optional<WorkforceAllocationResult> preallocated_upstream_workforce;

    // Stable-address ownership is required because each market bridge
    // retains an AggregateProducer&.
    std::vector<std::unique_ptr<BoundUpstreamSiteState>>
        additional_upstream_sites;

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
    [[nodiscard]] ProductiveSiteResourceRoute const*
    find_upstream_site_resource_route(
            std::string_view employer_id,
            GoodDefinition const& good
    ) const {
            auto const it = std::find_if(
                    upstream_site_resource_routes.begin(),
                    upstream_site_resource_routes.end(),
                    [employer_id, &good](
                            ProductiveSiteResourceRoute const& route
                    ) {
                            return route.employer_id == employer_id
                                    && route.good == &good;
                    }
            );

            return it != upstream_site_resource_routes.end()
                    ? &*it
                    : nullptr;
    }

    [[nodiscard]] bool has_upstream_site_resource_routes_for(
            GoodDefinition const& good
    ) const {
            return std::any_of(
                    upstream_site_resource_routes.begin(),
                    upstream_site_resource_routes.end(),
                    [&good](ProductiveSiteResourceRoute const& route) {
                            return route.good == &good;
                    }
            );
    }

    [[nodiscard]] ResourceFlowResult
    fulfill_source_flows_across_upstream_sites() {
            struct source_target_t final {
                    std::string employer_id;
                    AggregateProducer* producer = nullptr;
                    fixed_point_t shortfall = fixed_point_t::_0;
                    fixed_point_t accessible_request = fixed_point_t::_0;
                    fixed_point_t allocated = fixed_point_t::_0;
                    std::string flow_id;
            };

            struct input_flow_t final {
                    GoodDefinition const* good = nullptr;
                    fixed_point_t desired_flow = fixed_point_t::_0;
                    ResourceSupplyNetwork* network = nullptr;
                    std::vector<ResourceSourceAccess> source_access;
                    std::vector<source_target_t> targets;
                    ResourceFlowResult result {};
                    bool primary = false;
            };

            std::vector<input_flow_t> inputs;
            inputs.reserve(1 + additional_resource_inputs.size());

            inputs.push_back(input_flow_t {
                    .good = scenario.source_inflow_good,
                    .desired_flow = scenario.source_inflow_per_daily_tick,
                    .network = &source_network,
                    .source_access = build_resource_source_access(),
                    .primary = true
            });

            for (AdditionalResourceInputState& input :
                    additional_resource_inputs) {
                    inputs.push_back(input_flow_t {
                            .good = input.good,
                            .desired_flow = input.inflow_per_daily_tick,
                            .network = &input.network,
                            .primary = false
                    });
            }

            std::sort(
                    inputs.begin(),
                    inputs.end(),
                    [](input_flow_t const& lhs, input_flow_t const& rhs) {
                            if (lhs.primary != rhs.primary) {
                                    return lhs.primary;
                            }
                            return lhs.good->get_identifier()
                                    < rhs.good->get_identifier();
                    }
            );

            std::vector<LogisticsGraphFlowRequest> graph_requests;

            for (input_flow_t& input : inputs) {
                    GoodDefinition const& good = *input.good;

                    input.targets.push_back(source_target_t {
                            .employer_id = upstream_employer_id.empty()
                                    ? std::string { "site:primary" }
                                    : upstream_employer_id,
                            .producer = &upstream,
                            .shortfall =
                                    upstream_bridge
                                            .calculate_input_shortfall(good)
                    });

                    for (auto const& site : additional_upstream_sites) {
                            input.targets.push_back(source_target_t {
                                    .employer_id = site->employer_id,
                                    .producer = &site->producer,
                                    .shortfall =
                                            site->bridge
                                                .calculate_input_shortfall(
                                                        good
                                                )
                            });
                    }

                    std::sort(
                            input.targets.begin(),
                            input.targets.end(),
                            [](source_target_t const& lhs,
                                    source_target_t const& rhs) {
                                    return lhs.employer_id
                                            < rhs.employer_id;
                            }
                    );

                    bool const has_explicit_routes =
                            has_upstream_site_resource_routes_for(good);

                    for (source_target_t& target : input.targets) {
                            if (
                                    !has_explicit_routes ||
                                    target.shortfall <= fixed_point_t::_0
                            ) {
                                    target.accessible_request =
                                            target.shortfall;
                                    continue;
                            }

                            ProductiveSiteResourceRoute const* const route =
                                    find_upstream_site_resource_route(
                                            target.employer_id,
                                            good
                                    );

                            if (route == nullptr) {
                                    target.accessible_request =
                                            target.shortfall;
                                    continue;
                            }

                            target.flow_id =
                                    std::string { good.get_identifier() };
                            target.flow_id += "|";
                            target.flow_id += target.employer_id;

                            graph_requests.push_back(
                                    LogisticsGraphFlowRequest {
                                            .flow_id = target.flow_id,
                                            .source = route->source_node,
                                            .destination =
                                                    route->destination_node,
                                            .requested = target.shortfall
                                    }
                            );
                    }
            }

            // One graph allocation across every site AND every commodity.
            // Shared transport edges therefore cannot be double-spent by
            // giving each input good its own copy of edge capacity.
            auto const graph_allocations =
                    logistics_graph.allocate_flows(graph_requests);

            for (LogisticsGraphFlowAllocation const& allocation :
                    graph_allocations) {
                    for (input_flow_t& input : inputs) {
                            auto const target_it = std::find_if(
                                    input.targets.begin(),
                                    input.targets.end(),
                                    [&allocation](
                                            source_target_t const& target
                                    ) {
                                            return !target.flow_id.empty()
                                                    && target.flow_id ==
                                                        allocation.flow_id;
                                    }
                            );

                            if (target_it != input.targets.end()) {
                                    target_it->accessible_request =
                                            std::min(
                                                    target_it->shortfall,
                                                    allocation.allocated
                                            );
                                    break;
                            }
                    }
            }

            ResourceFlowResult primary_result {};

            for (input_flow_t& input : inputs) {
                    fixed_point_t total_accessible_request =
                            fixed_point_t::_0;

                    for (source_target_t const& target : input.targets) {
                            total_accessible_request +=
                                    target.accessible_request;
                    }

                    fixed_point_t const desired_source_flow =
                            std::max(
                                    input.desired_flow,
                                    fixed_point_t::_0
                            );

                    fixed_point_t const reachable_demand =
                            std::min(
                                    desired_source_flow,
                                    total_accessible_request
                            );

                    input.result = input.network->fulfill(
                            reachable_demand,
                            input.source_access
                    );

                    fixed_point_t const route_unmet =
                            desired_source_flow - reachable_demand;

                    if (
                            route_unmet > fixed_point_t::_0 &&
                            total_accessible_request <
                                    desired_source_flow
                    ) {
                            input.result.requested =
                                    desired_source_flow;
                            input.result.unmet += route_unmet;
                            input.result.source_access_limited = true;
                    }

                    if (
                            input.result.delivered > fixed_point_t::_0 &&
                            total_accessible_request >
                                    fixed_point_t::_0
                    ) {
                            fixed_point_t remaining =
                                    input.result.delivered;

                            for (source_target_t& target :
                                    input.targets) {
                                    if (
                                            target.accessible_request <=
                                            fixed_point_t::_0
                                    ) {
                                            continue;
                                    }

                                    fixed_point_t const share =
                                            std::min(
                                                    target.accessible_request,
                                                    input.result.delivered *
                                                        target
                                                            .accessible_request /
                                                        total_accessible_request
                                            );

                                    target.producer->add_inventory(
                                            *input.good,
                                            share
                                    );

                                    target.allocated += share;
                                    remaining -= share;
                            }

                            for (source_target_t& target :
                                    input.targets) {
                                    if (
                                            remaining <=
                                            fixed_point_t::_0
                                    ) {
                                            break;
                                    }

                                    fixed_point_t const unmet =
                                            std::max(
                                                    target
                                                        .accessible_request -
                                                        target.allocated,
                                                    fixed_point_t::_0
                                            );

                                    fixed_point_t const extra =
                                            std::min(
                                                    unmet,
                                                    remaining
                                            );

                                    if (extra > fixed_point_t::_0) {
                                            target.producer
                                                ->add_inventory(
                                                        *input.good,
                                                        extra
                                                );

                                            target.allocated += extra;
                                            remaining -= extra;
                                    }
                            }
                    }

                    if (input.primary) {
                            primary_result = input.result;
                    }
            }

            return primary_result;
    }

public:// Migration mapping only: SimTime itself remains unitless.
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

[[nodiscard]] fixed_point_t get_upstream_labor_offer(
fixed_point_t no_history_offer = fixed_point_t::_1
) const {
if (!completed_provenance.has_value()) {
return no_history_offer;
}

LiveEconomyCycleProvenance const& previous = *completed_provenance;

if (
!previous.workforce.has_value() ||
previous.workforce->allocated < fixed_point_t::_1
) {
return no_history_offer;
}

fixed_point_t const operating_surplus = std::max(
previous.upstream_market.money_received -
previous.upstream_market.money_spent,
fixed_point_t::_0
);

return operating_surplus / previous.workforce->allocated;
}

[[nodiscard]] ProductiveSiteBinding const* get_upstream_site_binding() const {
return upstream_site ? &*upstream_site : nullptr;
}

void set_preallocated_upstream_workforce(
WorkforceAllocationResult allocation
) {
preallocated_upstream_workforce = allocation;
}
	[[nodiscard]] bool bind_upstream_site(
            ProductiveSiteBinding binding,
            MapInstance& map
    ) {
            if (!binding.resolve(map, upstream.get_production_type())) {
                    return false;
            }

            upstream_employer_id =
                    BoundUpstreamSiteState::make_employer_id(binding);

            for (auto const& site : additional_upstream_sites) {
                    if (site->employer_id == upstream_employer_id) {
                            return false;
                    }
            }

            upstream_site = std::move(binding);
            return true;
    }

    [[nodiscard]] bool bind_additional_upstream_site(
            ProductiveSiteBinding binding,
            MapInstance& map
    ) {
            if (!binding.resolve(map, upstream.get_production_type())) {
                    return false;
            }

            std::string const employer_id =
                    BoundUpstreamSiteState::make_employer_id(binding);

            if (
                    !upstream_employer_id.empty() &&
                    employer_id == upstream_employer_id
            ) {
                    return false;
            }

            for (auto const& site : additional_upstream_sites) {
                    if (site->employer_id == employer_id) {
                            return false;
                    }
            }

            additional_upstream_sites.push_back(
                    std::make_unique<BoundUpstreamSiteState>(
                            std::move(binding),
                            upstream.get_production_type(),
                            scenario.upstream_utilization
                    )
            );

            // The vector moves unique_ptrs, not the site objects themselves.
            std::sort(
                    additional_upstream_sites.begin(),
                    additional_upstream_sites.end(),
                    [](auto const& lhs, auto const& rhs) {
                            return lhs->employer_id < rhs->employer_id;
                    }
            );

            return true;
    }

    [[nodiscard]] size_t get_additional_upstream_site_count() const {
            return additional_upstream_sites.size();
    }

    [[nodiscard]] AggregateProductionResult const*
    get_additional_upstream_last_production(size_t index) const {
            return index < additional_upstream_sites.size()
                    ? &additional_upstream_sites[index]->last_production
                    : nullptr;
    }

    [[nodiscard]] AggregateMarketCycleResult const*
    get_additional_upstream_last_market(size_t index) const {
            return index < additional_upstream_sites.size()
                    ? &additional_upstream_sites[index]->last_market
                    : nullptr;
    }

    [[nodiscard]] std::optional<WorkforceAllocationResult>
    get_additional_upstream_previous_workforce(size_t index) const {
            return index < additional_upstream_sites.size()
                    ? additional_upstream_sites[index]->previous_allocation
                    : std::nullopt;
    }

    [[nodiscard]] std::vector<ProvinceWorkforceEmployerRequests>
    prepare_upstream_employer_requests(MapInstance& map) {
            std::vector<ProvinceWorkforceEmployerRequests> groups;

            if (upstream_site.has_value()) {
                    auto resolved = upstream_site->resolve(
                            map,
                            upstream.get_production_type()
                    );

                    if (!resolved.has_value()) {
                            upstream.set_capacity(fixed_point_t::_0);
                            upstream.set_available_workforce(fixed_point_t::_0);
                            upstream_requested_workforce = fixed_point_t::_0;
                    } else {
                            upstream.set_capacity(resolved->installed_capacity);
                            upstream.set_available_workforce(fixed_point_t::_0);

                            WorkforceEmployerRequest request =
                                    make_producer_workforce_request(
                                            upstream,
                                            upstream_employer_id,
                                            get_upstream_labor_offer()
                                    );

                            upstream_requested_workforce = request.requested;

                            groups.push_back(
                                    ProvinceWorkforceEmployerRequests {
                                            .province_id =
                                                    upstream_site->province_id,
                                            .employers = { request }
                                    }
                            );
                    }
            }

            for (auto& site : additional_upstream_sites) {
                    auto resolved = site->binding.resolve(
                            map,
                            site->producer.get_production_type()
                    );

                    if (!resolved.has_value()) {
                            site->producer.set_capacity(fixed_point_t::_0);
                            site->producer.set_available_workforce(
                                    fixed_point_t::_0
                            );
                            site->requested_workforce = fixed_point_t::_0;
                            site->current_allocation =
                                    WorkforceAllocationResult {};
                            continue;
                    }

                    site->producer.set_capacity(
                            resolved->installed_capacity
                    );
                    site->producer.set_available_workforce(
                            fixed_point_t::_0
                    );

                    WorkforceEmployerRequest request =
                            make_producer_workforce_request(
                                    site->producer,
                                    site->employer_id,
                                    site->get_labor_offer()
                            );

                    site->requested_workforce = request.requested;

                    groups.push_back(
                            ProvinceWorkforceEmployerRequests {
                                    .province_id =
                                            site->binding.province_id,
                                    .employers = { request }
                            }
                    );
            }

            return groups;
    }

    void apply_upstream_employer_allocations(
            std::vector<WorkforceEmployerAllocation> const& allocations
    ) {
            if (upstream_site.has_value()) {
                    preallocated_upstream_workforce =
                            WorkforceAllocationResult {
                                    .requested =
                                            upstream_requested_workforce,
                                    .allocated = fixed_point_t::_0
                            };
            }

            for (auto& site : additional_upstream_sites) {
                    site->current_allocation =
                            WorkforceAllocationResult {
                                    .requested =
                                            site->requested_workforce,
                                    .allocated = fixed_point_t::_0
                            };
            }

            for (
                    WorkforceEmployerAllocation const& allocation :
                    allocations
            ) {
                    if (
                            upstream_site.has_value() &&
                            allocation.employer_id ==
                                    upstream_employer_id
                    ) {
                            preallocated_upstream_workforce =
                                    WorkforceAllocationResult {
                                            .requested =
                                                    upstream_requested_workforce,
                                            .allocated =
                                                    allocation.allocated
                                    };
                            continue;
                    }

                    for (auto& site : additional_upstream_sites) {
                            if (
                                    allocation.employer_id ==
                                    site->employer_id
                            ) {
                                    site->current_allocation =
                                            WorkforceAllocationResult {
                                                    .requested =
                                                            site->requested_workforce,
                                                    .allocated =
                                                            allocation.allocated
                                            };
                                    break;
                            }
                    }
            }
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
	[[nodiscard]] bool configure_additional_upstream_resource_input(
		GoodDefinition const& good,
		fixed_point_t inflow_per_daily_tick,
		market_node_index_t source_node
	) {
		if (
			&good == scenario.source_inflow_good ||
			!upstream.get_production_type().input_goods.contains(&good) ||
			inflow_per_daily_tick < fixed_point_t::_0
		) {
			return false;
		}

		for (AdditionalResourceInputState const& input :
				additional_resource_inputs) {
			if (input.good == &good) {
				return false;
			}
		}

		std::string source_id = "input:";
		source_id += good.get_identifier();

		ResourceSupplyNetwork network {
			{
				ResourceSourceState {
					.source_id = source_id,
					.node = source_node,
					.supply = ResourceSupplyState {
						.nominal_per_tick =
								inflow_per_daily_tick,
						.availability_fraction =
								fixed_point_t::_1
					}
				}
			}
		};

		if (!network.is_valid()) {
			return false;
		}

		additional_resource_inputs.push_back(
			AdditionalResourceInputState {
				.good = &good,
				.inflow_per_daily_tick = inflow_per_daily_tick,
				.source_id = std::move(source_id),
				.network = std::move(network)
			}
		);

		std::sort(
			additional_resource_inputs.begin(),
			additional_resource_inputs.end(),
			[](AdditionalResourceInputState const& lhs,
				AdditionalResourceInputState const& rhs) {
				return lhs.good->get_identifier()
						< rhs.good->get_identifier();
			}
		);

		return true;
	}

	[[nodiscard]] bool set_additional_upstream_resource_availability(
		GoodDefinition const& good,
		fixed_point_t availability_fraction
	) {
		for (AdditionalResourceInputState& input :
				additional_resource_inputs) {
			if (input.good == &good) {
				return input.network.set_source_availability(
					input.source_id,
					availability_fraction
				);
			}
		}

		return false;
	}

	[[nodiscard]] size_t get_additional_upstream_resource_input_count()
			const {
		return additional_resource_inputs.size();
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
	}	[[nodiscard]] bool configure_upstream_site_resource_routes(
		std::vector<ProductiveSiteResourceRoute> routes
	) {
		for (ProductiveSiteResourceRoute& route : routes) {
			if (route.employer_id.empty()) {
				return false;
			}

			if (route.good == nullptr) {
				route.good = scenario.source_inflow_good;
			}

			if (
				route.good == nullptr ||
				!upstream.get_production_type()
					.input_goods.contains(route.good)
			) {
				return false;
			}
		}

		std::sort(
			routes.begin(),
			routes.end(),
			[](ProductiveSiteResourceRoute const& lhs,
				ProductiveSiteResourceRoute const& rhs) {
				if (lhs.good->get_identifier()
						!= rhs.good->get_identifier()) {
					return lhs.good->get_identifier()
							< rhs.good->get_identifier();
				}
				return lhs.employer_id < rhs.employer_id;
			}
		);

		for (size_t i = 1; i < routes.size(); ++i) {
			if (
				routes[i - 1].good == routes[i].good &&
				routes[i - 1].employer_id ==
						routes[i].employer_id
			) {
				return false;
			}
		}

		upstream_site_resource_routes = std::move(routes);
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

            for (auto& site : additional_upstream_sites) {
                    site->bridge.reset_cycle_result();
            }
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
			fulfill_source_flows_across_upstream_sites();

		status.source_buffer_draw = source_flow.buffer_draw;
		status.source_unmet_inflow = source_flow.unmet;

            const AggregateProductionResult upstream_result =
                    upstream.produce();

            fixed_point_t total_upstream_output =
                    upstream_result.actual_output;

            for (auto& site : additional_upstream_sites) {
                    site->last_production =
                            site->producer.produce();

                    total_upstream_output +=
                            site->last_production.actual_output;
            }

            status.upstream_output = total_upstream_output;
		if (pending_provenance) {
			pending_provenance->resource = source_flow;
			pending_provenance->upstream = upstream_result;
		}

		fixed_point_t physical_intermediate =
                    upstream.get_inventory(intermediate_good);

            for (auto const& site : additional_upstream_sites) {
                    physical_intermediate +=
                            site->producer.get_inventory(
                                    intermediate_good
                            );
            }

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

            for (auto& site : additional_upstream_sites) {
                    if (
                            auto sell_order =
                                    site->bridge.make_output_sell_order();
                            sell_order.has_value()
                    ) {
                            market.add_market_sell_order(
                                    std::move(*sell_order)
                            );
                    }
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

            for (auto& site : additional_upstream_sites) {
                    site->bridge.clear_completed_orders();

                    site->last_market =
                            site->bridge.get_cycle_result();

                    site->previous_allocation =
                            site->current_allocation;

                    site->current_allocation.reset();
            }

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
