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
#include "openvic-simulation/economy/production/ProductiveSiteMaterialFlowResolver.hpp"
#include "openvic-simulation/economy/production/ProductiveSiteUtilityResolver.hpp"
#include "openvic-simulation/economy/production/ProductiveSiteElectricityGridResolver.hpp"
#include "openvic-simulation/economy/production/ProductiveSiteOperatingEconomics.hpp"
#include "openvic-simulation/economy/production/ProductiveSiteWageFormation.hpp"
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
        std::optional<ProductiveSiteOperatingEconomicsResult> last_economics;
        fixed_point_t labor_offer_used = fixed_point_t::_1;
        fixed_point_t compensation_per_worker = fixed_point_t::_0;
        bool wage_formation_enabled = false;
        ProductiveSiteWageFormationPolicy wage_policy {};
        std::optional<ProductiveSiteWageFormationResult> last_wage_formation;

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

            if (!last_economics.has_value()) {
                return no_history_offer;
            }

            return ProductiveSiteOperatingEconomics::
                labor_offer_from_prior_economics(
                    *last_economics,
                    previous_allocation->allocated,
                    no_history_offer
                );
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
	std::vector<ProductiveSiteUtilityRequirement> upstream_site_utility_requirements;
	std::vector<ProductiveSiteElectricitySource> electricity_sources;
	LogisticsGraph electricity_transmission_graph;
	std::vector<ProductiveSiteElectricityConnection> electricity_connections;
	bool electricity_grid_configured = false;
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
    std::optional<WorkforceAllocationResult> upstream_current_allocation;
    std::optional<WorkforceAllocationResult> upstream_previous_allocation;
    AggregateProductionResult upstream_last_production {};
    AggregateMarketCycleResult upstream_last_market {};
    std::optional<ProductiveSiteOperatingEconomicsResult>
        upstream_last_economics;
    fixed_point_t upstream_labor_offer_used = fixed_point_t::_1;
    fixed_point_t upstream_compensation_per_worker = fixed_point_t::_0;
    bool upstream_wage_formation_enabled = false;
    ProductiveSiteWageFormationPolicy upstream_wage_policy {};
    std::optional<ProductiveSiteWageFormationResult>
        upstream_last_wage_formation;
    std::vector<ProductiveSiteElectricityAllocation>
        latest_electricity_allocations;

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
    [[nodiscard]] fixed_point_t electricity_cost_proxy_for(
        std::string_view employer_id
    ) const {
        auto const it = std::find_if(
            latest_electricity_allocations.begin(),
            latest_electricity_allocations.end(),
            [employer_id](ProductiveSiteElectricityAllocation const& allocation) {
                return allocation.employer_id == employer_id;
            }
        );

        return it != latest_electricity_allocations.end()
            ? it->generation_cost_proxy
            : fixed_point_t::_0;
    }

    [[nodiscard]] fixed_point_t calculate_material_replacement_cost(
        AggregateProducer const& producer,
        AggregateProductionResult const& production
    ) {
        fixed_point_t total = fixed_point_t::_0;

        for (auto const& [good, input_per_output] :
                producer.get_production_type().input_goods) {
            if (good == nullptr || input_per_output <= fixed_point_t::_0) {
                continue;
            }

            GoodInstance& market =
                good_instance_manager.get_good_instance_by_definition(*good);

            fixed_point_t const consumed =
                input_per_output * production.actual_output;

            total += consumed * std::max(
                market.get_price(),
                fixed_point_t::_0
            );
        }

        return total;
    }

    [[nodiscard]] ProductiveSiteOperatingEconomicsResult
    calculate_site_operating_economics(
        std::string_view employer_id,
        AggregateProducer const& producer,
        AggregateProductionResult const& production,
        AggregateMarketCycleResult const& market,
        std::optional<WorkforceAllocationResult> const& allocation,
        fixed_point_t labor_offer_used,
        fixed_point_t compensation_per_worker
    ) {
        // B4B's labor_offer_used remains an employer-priority signal.
        // B16 compensation_per_worker is a distinct gross wage/compensation rate.
        (void)labor_offer_used;
        fixed_point_t const labor_compensation_cost =
            allocation.has_value()
                ? allocation->allocated *
                    std::max(
                        compensation_per_worker,
                        fixed_point_t::_0
                    )
                : fixed_point_t::_0;

        return ProductiveSiteOperatingEconomics::calculate(
            production.actual_output,
            market.money_received,
            market.money_spent,
            calculate_material_replacement_cost(producer, production),
            labor_compensation_cost,
            electricity_cost_proxy_for(employer_id),
            production.actual_output * corridor.calculate_unit_cost()
        );
    }

    [[nodiscard]] static fixed_point_t
    active_compensation_or_zero(
        fixed_point_t compensation,
        std::optional<WorkforceAllocationResult> const& allocation
    ) {
        return
            allocation.has_value() &&
            allocation->allocated >= fixed_point_t::_1
                ? std::max(compensation, fixed_point_t::_0)
                : fixed_point_t::_0;
    }

    [[nodiscard]] static ProductiveSiteWageFormationResult
    calculate_next_site_compensation(
        fixed_point_t current_compensation,
        fixed_point_t requested_workforce,
        std::optional<WorkforceAllocationResult> const& allocation,
        ProductiveSiteOperatingEconomicsResult const& economics,
        fixed_point_t highest_competing_compensation,
        ProductiveSiteWageFormationPolicy const& policy
    ) {
        return ProductiveSiteWageFormation::calculate(
            ProductiveSiteWageFormationInput {
                .prior_compensation = current_compensation,
                .requested_workforce = requested_workforce,
                .allocated_workforce =
                    allocation.has_value()
                        ? allocation->allocated
                        : fixed_point_t::_0,
                .operating_surplus = economics.operating_surplus,
                .highest_competing_compensation =
                    highest_competing_compensation
            },
            policy
        );
    }

    void apply_upstream_site_utility_constraints() {
        std::vector<ProductiveSiteUtilityTarget> targets;
        targets.reserve(1 + additional_upstream_sites.size());

        targets.push_back(ProductiveSiteUtilityTarget {
            .employer_id = upstream_employer_id.empty()
                ? std::string { "site:primary" }
                : upstream_employer_id,
            .producer = &upstream
        });

        for (auto& site : additional_upstream_sites) {
            targets.push_back(ProductiveSiteUtilityTarget {
                .employer_id = site->employer_id,
                .producer = &site->producer
            });
        }

        latest_electricity_allocations.clear();

        if (electricity_grid_configured) {
            latest_electricity_allocations =
                ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
                    electricity_sources,
                    electricity_transmission_graph,
                    electricity_connections,
                    targets,
                    upstream_site_utility_requirements
                );
        }

        ProductiveSiteUtilityResolver::apply(
            targets,
            upstream_site_utility_requirements
        );
    }

    [[nodiscard]] ResourceFlowResult
    fulfill_source_flows_across_upstream_sites() {
        std::vector<ProductiveSiteMaterialInput> inputs;
        inputs.reserve(1 + additional_resource_inputs.size());

        inputs.push_back(ProductiveSiteMaterialInput {
            .good = scenario.source_inflow_good,
            .desired_flow = scenario.source_inflow_per_daily_tick,
            .network = &source_network,
            .source_access = build_resource_source_access(),
            .primary = true
        });

        for (AdditionalResourceInputState& input :
                additional_resource_inputs) {
            inputs.push_back(ProductiveSiteMaterialInput {
                .good = input.good,
                .desired_flow = input.inflow_per_daily_tick,
                .network = &input.network,
                .primary = false
            });
        }

        std::vector<ProductiveSiteMaterialTarget> targets;
        targets.reserve(1 + additional_upstream_sites.size());

        targets.push_back(ProductiveSiteMaterialTarget {
            .employer_id = upstream_employer_id.empty()
                ? std::string { "site:primary" }
                : upstream_employer_id,
            .producer = &upstream,
            .bridge = &upstream_bridge
        });

        for (auto& site : additional_upstream_sites) {
            targets.push_back(ProductiveSiteMaterialTarget {
                .employer_id = site->employer_id,
                .producer = &site->producer,
                .bridge = &site->bridge
            });
        }

        std::vector<MaterialInventoryTarget> inventory_targets;

        for (ProductiveSiteElectricitySource& source : electricity_sources) {
            if (
                source.fuel_good == nullptr ||
                source.fuel_per_output <= fixed_point_t::_0
            ) {
                continue;
            }

            fixed_point_t const desired_inventory =
                std::max(
                    source.available_generation_per_tick,
                    fixed_point_t::_0
                ) *
                std::clamp(
                    source.availability_fraction,
                    fixed_point_t::_0,
                    fixed_point_t::_1
                ) *
                calculate_generator_effective_fuel_per_output(source);

            inventory_targets.push_back(MaterialInventoryTarget {
                .consumer_id = std::string { "generator:" } + source.source_id,
                .good = source.fuel_good,
                .desired_inventory = desired_inventory,
                .inventory = &source.fuel_inventory
            });
        }

        return ProductiveSiteMaterialFlowResolver::resolve(
            std::move(inputs),
            targets,
            inventory_targets,
            upstream_site_resource_routes,
            logistics_graph
        );
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
if (
!upstream_previous_allocation.has_value() ||
!upstream_last_economics.has_value()
) {
return no_history_offer;
}

return ProductiveSiteOperatingEconomics::
labor_offer_from_prior_economics(
*upstream_last_economics,
upstream_previous_allocation->allocated,
no_history_offer
);
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

    [[nodiscard]] std::optional<ProductiveSiteOperatingEconomicsResult>
    get_upstream_operating_economics() const {
            return upstream_last_economics;
    }

    [[nodiscard]] std::optional<ProductiveSiteOperatingEconomicsResult>
    get_additional_upstream_operating_economics(size_t index) const {
            return index < additional_upstream_sites.size()
                    ? additional_upstream_sites[index]->last_economics
                    : std::nullopt;
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

                            upstream_labor_offer_used =
                                    get_upstream_labor_offer();

                            WorkforceEmployerRequest request =
                                    make_producer_workforce_request(
                                            upstream,
                                            upstream_employer_id,
                                            upstream_labor_offer_used,
                                            upstream_compensation_per_worker
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

                    site->labor_offer_used = site->get_labor_offer();

                    WorkforceEmployerRequest request =
                            make_producer_workforce_request(
                                    site->producer,
                                    site->employer_id,
                                    site->labor_offer_used,
                                    site->compensation_per_worker
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
                    upstream_current_allocation =
                            preallocated_upstream_workforce;
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
                            upstream_current_allocation =
                                    preallocated_upstream_workforce;
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

	[[nodiscard]] bool set_upstream_site_compensation_per_worker(
		std::string_view employer_id,
		fixed_point_t compensation_per_worker
	) {
		if (
			employer_id.empty() ||
			compensation_per_worker < fixed_point_t::_0
		) {
			return false;
		}

		std::string const primary_id =
			upstream_employer_id.empty()
				? std::string { "site:primary" }
				: upstream_employer_id;

		if (employer_id == primary_id) {
			upstream_compensation_per_worker =
				compensation_per_worker;
			return true;
		}

		for (auto& site : additional_upstream_sites) {
			if (site->employer_id == employer_id) {
				site->compensation_per_worker =
					compensation_per_worker;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool configure_upstream_site_wage_formation(
		std::string_view employer_id,
		ProductiveSiteWageFormationPolicy const& policy
	) {
		if (employer_id.empty() || !policy.is_valid()) {
			return false;
		}

		std::string const primary_id =
			upstream_employer_id.empty()
				? std::string { "site:primary" }
				: upstream_employer_id;

		if (employer_id == primary_id) {
			upstream_wage_policy = policy;
			upstream_wage_formation_enabled = true;
			return true;
		}

		for (auto& site : additional_upstream_sites) {
			if (site->employer_id == employer_id) {
				site->wage_policy = policy;
				site->wage_formation_enabled = true;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_site_wage_formation_enabled(
		std::string_view employer_id,
		bool enabled
	) {
		if (employer_id.empty()) {
			return false;
		}

		std::string const primary_id =
			upstream_employer_id.empty()
				? std::string { "site:primary" }
				: upstream_employer_id;

		if (employer_id == primary_id) {
			upstream_wage_formation_enabled = enabled;
			return true;
		}

		for (auto& site : additional_upstream_sites) {
			if (site->employer_id == employer_id) {
				site->wage_formation_enabled = enabled;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] std::optional<fixed_point_t>
	get_upstream_site_compensation_per_worker(
		std::string_view employer_id
	) const {
		std::string const primary_id =
			upstream_employer_id.empty()
				? std::string { "site:primary" }
				: upstream_employer_id;

		if (employer_id == primary_id) {
			return upstream_compensation_per_worker;
		}

		for (auto const& site : additional_upstream_sites) {
			if (site->employer_id == employer_id) {
				return site->compensation_per_worker;
			}
		}

		return std::nullopt;
	}

	[[nodiscard]] std::optional<ProductiveSiteWageFormationResult>
	get_upstream_site_last_wage_formation(
		std::string_view employer_id
	) const {
		std::string const primary_id =
			upstream_employer_id.empty()
				? std::string { "site:primary" }
				: upstream_employer_id;

		if (employer_id == primary_id) {
			return upstream_last_wage_formation;
		}

		for (auto const& site : additional_upstream_sites) {
			if (site->employer_id == employer_id) {
				return site->last_wage_formation;
			}
		}

		return std::nullopt;
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



	[[nodiscard]] bool configure_upstream_electricity_sources(
		std::vector<ProductiveSiteElectricitySource> sources,
		std::vector<LogisticsGraphEdge> transmission_edges,
		std::vector<ProductiveSiteElectricityConnection> connections
	) {
		if (sources.empty()) {
			return false;
		}

		for (ProductiveSiteElectricitySource const& source : sources) {
			if (
				source.source_id.empty() ||
				source.available_generation_per_tick < fixed_point_t::_0 ||
				source.availability_fraction < fixed_point_t::_0 ||
				source.availability_fraction > fixed_point_t::_1 ||
				source.minimum_stable_output < fixed_point_t::_0 ||
				source.ramp_up_per_tick < fixed_point_t::_0 ||
				source.ramp_down_per_tick < fixed_point_t::_0 ||
				source.marginal_cost < fixed_point_t::_0 ||
				source.current_dispatch_per_tick < fixed_point_t::_0 ||
				source.fuel_per_output < fixed_point_t::_0 ||
				source.fuel_inventory < fixed_point_t::_0 ||
				source.resource_availability_fraction < fixed_point_t::_0 ||
				source.resource_availability_fraction > fixed_point_t::_1 ||
				source.heat_rate_multiplier <= fixed_point_t::_0 ||
				(source.fuel_good == nullptr &&
					source.fuel_per_output > fixed_point_t::_0)
			) {
				return false;
			}
		}

		std::sort(
			sources.begin(),
			sources.end(),
			[](ProductiveSiteElectricitySource const& lhs,
				ProductiveSiteElectricitySource const& rhs) {
				return lhs.source_id < rhs.source_id;
			}
		);

		for (size_t i = 1; i < sources.size(); ++i) {
			if (sources[i - 1].source_id == sources[i].source_id) {
				return false;
			}
		}

		for (ProductiveSiteElectricityConnection const& connection :
				connections) {
			if (connection.employer_id.empty()) {
				return false;
			}
		}

		std::sort(
			connections.begin(),
			connections.end(),
			[](ProductiveSiteElectricityConnection const& lhs,
				ProductiveSiteElectricityConnection const& rhs) {
				return lhs.employer_id < rhs.employer_id;
			}
		);

		for (size_t i = 1; i < connections.size(); ++i) {
			if (
				connections[i - 1].employer_id ==
					connections[i].employer_id
			) {
				return false;
			}
		}

		LogisticsGraph candidate;
		if (!candidate.configure(std::move(transmission_edges))) {
			return false;
		}

		electricity_sources = std::move(sources);
		electricity_transmission_graph = std::move(candidate);
		electricity_connections = std::move(connections);
		electricity_grid_configured = true;
		return true;
	}

	[[nodiscard]] bool configure_upstream_electricity_grid(
		ProductiveSiteElectricityGridState grid,
		std::vector<LogisticsGraphEdge> transmission_edges,
		std::vector<ProductiveSiteElectricityConnection> connections
	) {
		return configure_upstream_electricity_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "grid_source",
					.source_node = grid.source_node,
					.available_generation_per_tick =
						grid.available_generation_per_tick
				}
			},
			std::move(transmission_edges),
			std::move(connections)
		);
	}

	[[nodiscard]] bool configure_upstream_electricity_source_fuel(
		std::string_view source_id,
		GoodDefinition const* fuel_good,
		fixed_point_t fuel_per_output,
		fixed_point_t initial_inventory
	) {
		if (
			!electricity_grid_configured ||
			fuel_good == nullptr ||
			fuel_per_output <= fixed_point_t::_0 ||
			initial_inventory < fixed_point_t::_0
		) {
			return false;
		}

		for (ProductiveSiteElectricitySource& source : electricity_sources) {
			if (source.source_id == source_id) {
				source.fuel_good = fuel_good;
				source.fuel_per_output = fuel_per_output;
				source.fuel_inventory = initial_inventory;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_electricity_source_resource_availability(
		std::string_view source_id,
		fixed_point_t resource_availability_fraction
	) {
		if (
			!electricity_grid_configured ||
			resource_availability_fraction < fixed_point_t::_0 ||
			resource_availability_fraction > fixed_point_t::_1
		) {
			return false;
		}

		for (ProductiveSiteElectricitySource& source : electricity_sources) {
			if (source.source_id == source_id) {
				source.resource_availability_fraction =
					resource_availability_fraction;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_electricity_source_forced_outage(
		std::string_view source_id,
		bool forced_outage
	) {
		if (!electricity_grid_configured) {
			return false;
		}

		for (ProductiveSiteElectricitySource& source : electricity_sources) {
			if (source.source_id == source_id) {
				source.forced_outage = forced_outage;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_electricity_source_heat_rate_multiplier(
		std::string_view source_id,
		fixed_point_t heat_rate_multiplier
	) {
		if (
			!electricity_grid_configured ||
			heat_rate_multiplier <= fixed_point_t::_0
		) {
			return false;
		}

		for (ProductiveSiteElectricitySource& source : electricity_sources) {
			if (source.source_id == source_id) {
				source.heat_rate_multiplier = heat_rate_multiplier;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_electricity_source_availability(
		std::string_view source_id,
		fixed_point_t availability_fraction
	) {
		if (
			!electricity_grid_configured ||
			availability_fraction < fixed_point_t::_0 ||
			availability_fraction > fixed_point_t::_1
		) {
			return false;
		}

		for (ProductiveSiteElectricitySource& source :
				electricity_sources) {
			if (source.source_id == source_id) {
				source.availability_fraction = availability_fraction;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_electricity_source_generation(
		std::string_view source_id,
		fixed_point_t available_generation_per_tick
	) {
		if (
			!electricity_grid_configured ||
			available_generation_per_tick < fixed_point_t::_0
		) {
			return false;
		}

		for (ProductiveSiteElectricitySource& source :
				electricity_sources) {
			if (source.source_id == source_id) {
				source.available_generation_per_tick =
					available_generation_per_tick;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] bool set_upstream_electricity_generation(
		fixed_point_t available_generation_per_tick
	) {
		if (
			electricity_sources.size() != 1 ||
			available_generation_per_tick < fixed_point_t::_0
		) {
			return false;
		}

		electricity_sources.front().available_generation_per_tick =
			available_generation_per_tick;
		return true;
	}

	[[nodiscard]] bool set_upstream_electricity_edge_open(
		std::string_view edge_id,
		bool open
	) {
		return electricity_grid_configured &&
			electricity_transmission_graph.set_edge_open(
				edge_id,
				open
			);
	}

	[[nodiscard]] bool configure_upstream_site_utility_requirements(
		std::vector<ProductiveSiteUtilityRequirement> requirements
	) {
		for (ProductiveSiteUtilityRequirement const& requirement :
				requirements) {
			if (
				requirement.employer_id.empty() ||
				requirement.required_per_output < fixed_point_t::_0 ||
				requirement.available_per_tick < fixed_point_t::_0
			) {
				return false;
			}
		}

		std::sort(
			requirements.begin(),
			requirements.end(),
			[](ProductiveSiteUtilityRequirement const& lhs,
				ProductiveSiteUtilityRequirement const& rhs) {
				if (lhs.employer_id != rhs.employer_id) {
					return lhs.employer_id < rhs.employer_id;
				}
				return static_cast<int>(lhs.kind) <
					static_cast<int>(rhs.kind);
			}
		);

		for (size_t i = 1; i < requirements.size(); ++i) {
			if (
				requirements[i - 1].employer_id ==
					requirements[i].employer_id &&
				requirements[i - 1].kind ==
					requirements[i].kind
			) {
				return false;
			}
		}

		upstream_site_utility_requirements =
			std::move(requirements);
		return true;
	}

	[[nodiscard]] bool set_upstream_site_utility_availability(
		std::string_view employer_id,
		ProductiveSiteUtilityKind kind,
		fixed_point_t available_per_tick
	) {
		if (available_per_tick < fixed_point_t::_0) {
			return false;
		}

		for (ProductiveSiteUtilityRequirement& requirement :
				upstream_site_utility_requirements) {
			if (
				requirement.employer_id == employer_id &&
				requirement.kind == kind
			) {
				requirement.available_per_tick =
					available_per_tick;
				return true;
			}
		}

		return false;
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
upstream_current_allocation = allocation;
if (pending_provenance) {
pending_provenance->workforce = allocation;
}
preallocated_upstream_workforce.reset();
} else if (preallocated_upstream_workforce.has_value()) {
upstream_current_allocation = *preallocated_upstream_workforce;
if (pending_provenance) {
pending_provenance->workforce =
*preallocated_upstream_workforce;
}
preallocated_upstream_workforce.reset();
}
		apply_upstream_site_utility_constraints();

		ResourceFlowResult const source_flow =
			fulfill_source_flows_across_upstream_sites();

		status.source_buffer_draw = source_flow.buffer_draw;
		status.source_unmet_inflow = source_flow.unmet;

            upstream_last_production = upstream.produce();
            const AggregateProductionResult& upstream_result =
                    upstream_last_production;

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

            upstream_last_market =
                    upstream_bridge.get_cycle_result();

            std::string const primary_employer_id =
                    upstream_employer_id.empty()
                            ? std::string { "site:primary" }
                            : upstream_employer_id;

            upstream_last_economics =
                    calculate_site_operating_economics(
                            primary_employer_id,
                            upstream,
                            upstream_last_production,
                            upstream_last_market,
                            upstream_current_allocation,
                            upstream_labor_offer_used,
                            upstream_compensation_per_worker
                    );

            // Wage formation is intentionally one-cycle delayed:
            // this cycle's compensation is paid during authoritative hiring,
            // then realized labor scarcity/economics set next cycle's rate.
            fixed_point_t const primary_prior_compensation =
                    upstream_compensation_per_worker;

            std::vector<fixed_point_t> additional_prior_compensation;
            additional_prior_compensation.reserve(
                    additional_upstream_sites.size()
            );
            for (auto const& site : additional_upstream_sites) {
                    additional_prior_compensation.push_back(
                            site->compensation_per_worker
                    );
            }

            auto const highest_competing_compensation =
                    [&](std::string_view employer_id) {
                            fixed_point_t highest =
                                    fixed_point_t::_0;

                            std::string const primary_id =
                                    upstream_employer_id.empty()
                                            ? std::string { "site:primary" }
                                            : upstream_employer_id;

                            if (employer_id != primary_id) {
                                    highest = std::max(
                                            highest,
                                            active_compensation_or_zero(
                                                primary_prior_compensation,
                                                upstream_current_allocation
                                            )
                                    );
                            }

                            for (size_t i = 0;
                                 i < additional_upstream_sites.size();
                                 ++i) {
                                    auto const& candidate =
                                            additional_upstream_sites[i];

                                    if (candidate->employer_id == employer_id) {
                                            continue;
                                    }

                                    highest = std::max(
                                        highest,
                                        active_compensation_or_zero(
                                            additional_prior_compensation[i],
                                            candidate->current_allocation
                                        )
                                    );
                            }

                            return highest;
                    };

            std::string const primary_id =
                    upstream_employer_id.empty()
                            ? std::string { "site:primary" }
                            : upstream_employer_id;

            upstream_last_wage_formation.reset();
            if (
                    upstream_wage_formation_enabled &&
                    upstream_last_economics.has_value() &&
                    upstream_current_allocation.has_value()
            ) {
                    upstream_last_wage_formation =
                            calculate_next_site_compensation(
                                    primary_prior_compensation,
                                    upstream_requested_workforce,
                                    upstream_current_allocation,
                                    *upstream_last_economics,
                                    highest_competing_compensation(
                                        primary_id
                                    ),
                                    upstream_wage_policy
                            );

                    upstream_compensation_per_worker =
                            upstream_last_wage_formation->
                                next_compensation;
            }

            for (auto& site : additional_upstream_sites) {
                    site->bridge.clear_completed_orders();

                    site->last_market =
                            site->bridge.get_cycle_result();

                    site->last_economics =
                            calculate_site_operating_economics(
                                    site->employer_id,
                                    site->producer,
                                    site->last_production,
                                    site->last_market,
                                    site->current_allocation,
                                    site->labor_offer_used,
                                    site->compensation_per_worker
                            );
            }

            for (size_t i = 0;
                 i < additional_upstream_sites.size();
                 ++i) {
                    auto& site = additional_upstream_sites[i];
                    site->last_wage_formation.reset();

                    if (
                            site->wage_formation_enabled &&
                            site->last_economics.has_value() &&
                            site->current_allocation.has_value()
                    ) {
                            site->last_wage_formation =
                                    calculate_next_site_compensation(
                                            additional_prior_compensation[i],
                                            site->requested_workforce,
                                            site->current_allocation,
                                            *site->last_economics,
                                            highest_competing_compensation(
                                                site->employer_id
                                            ),
                                            site->wage_policy
                                    );

                            site->compensation_per_worker =
                                    site->last_wage_formation->
                                        next_compensation;
                    }
            }

            upstream_previous_allocation =
                    upstream_current_allocation;
            upstream_current_allocation.reset();

            for (auto& site : additional_upstream_sites) {
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
			pending_provenance->upstream_economics = upstream_last_economics;
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
