#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/production/ProductiveSiteUtilityResolver.hpp"
#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"

namespace OpenVic {

struct ProductiveSiteElectricityConnection final {
	std::string employer_id;
	market_node_index_t destination_node {};
};

struct ProductiveSiteElectricityGridState final {
	market_node_index_t source_node {};
	fixed_point_t available_generation_per_tick = fixed_point_t::_0;
};

struct ProductiveSiteElectricitySource final {
	std::string source_id;
	market_node_index_t source_node {};

	// Nominal/nameplate generation supplied by scenario/runtime state.
	fixed_point_t available_generation_per_tick = fixed_point_t::_0;

	// B12 operating characteristics. Defaults preserve B11 behavior.
	fixed_point_t availability_fraction = fixed_point_t::_1;
	fixed_point_t minimum_stable_output = fixed_point_t::_0;
	fixed_point_t ramp_up_per_tick = fixed_point_t::usable_max;
	fixed_point_t ramp_down_per_tick = fixed_point_t::usable_max;
	fixed_point_t marginal_cost = fixed_point_t::_0;
	int32_t dispatch_priority = 0;

	// Stateful runtime dispatch history used by ramp constraints.
	fixed_point_t current_dispatch_per_tick = fixed_point_t::_0;
};

struct ProductiveSiteElectricityAllocation final {
	std::string employer_id;
	fixed_point_t requested = fixed_point_t::_0;
	fixed_point_t transmission_allocated = fixed_point_t::_0;
	fixed_point_t delivered = fixed_point_t::_0;
};

/// Deterministic coarse electricity allocation for productive sites.
///
/// Multiple finite generation sources can inject at different grid nodes.
/// B11 adds one deterministic redispatch pass: after the first shared
/// transmission solve, generation that failed to reach loads may be reassigned
/// from any source with spare physical generation to loads that remain unmet
/// and are still reachable.
///
/// The final redispatch state is solved as one complete source->load flow set,
/// so line capacities are never double-spent between passes.
///
/// This is still not economic dispatch, unit commitment, reserve scheduling,
/// storage dispatch, AC power flow, frequency control, or a power market.
class ProductiveSiteElectricityGridResolver final {
private:
	struct Load final {
		std::string employer_id;
		market_node_index_t destination_node {};
		fixed_point_t requested = fixed_point_t::_0;
		fixed_point_t delivered = fixed_point_t::_0;
	};

	struct SourceBudget final {
		std::string source_id;
		market_node_index_t source_node {};
		fixed_point_t available = fixed_point_t::_0;
		fixed_point_t minimum_stable_output = fixed_point_t::_0;
		fixed_point_t mandatory_floor = fixed_point_t::_0;
		fixed_point_t dispatch_budget = fixed_point_t::_0;
		fixed_point_t marginal_cost = fixed_point_t::_0;
		int32_t dispatch_priority = 0;
	};

	struct SourceLoadDelivery final {
		std::string source_id;
		std::string employer_id;
		fixed_point_t delivered = fixed_point_t::_0;
	};

	[[nodiscard]] static ProductiveSiteUtilityTarget const* find_target(
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::string_view employer_id
	) {
		auto const it = std::find_if(
			targets.begin(),
			targets.end(),
			[employer_id](ProductiveSiteUtilityTarget const& target) {
				return target.employer_id == employer_id;
			}
		);

		return it != targets.end() ? &*it : nullptr;
	}

	[[nodiscard]] static ProductiveSiteElectricityConnection const*
	find_connection(
		std::vector<ProductiveSiteElectricityConnection> const& connections,
		std::string_view employer_id
	) {
		auto const it = std::find_if(
			connections.begin(),
			connections.end(),
			[employer_id](ProductiveSiteElectricityConnection const& connection) {
				return connection.employer_id == employer_id;
			}
		);

		return it != connections.end() ? &*it : nullptr;
	}

	static void assign_source_dispatch_budgets(
		std::vector<SourceBudget>& sources,
		fixed_point_t total_requested
	) {
		if (total_requested <= fixed_point_t::_0) {
			return;
		}

		// First honor committed/ramp-constrained floors. If those floors exceed
		// load, proportionally curtail them as an emergency balancing action.
		fixed_point_t total_floor = fixed_point_t::_0;
		for (SourceBudget const& source : sources) {
			total_floor += source.mandatory_floor;
		}

		if (total_floor >= total_requested && total_floor > fixed_point_t::_0) {
			fixed_point_t remaining = total_requested;

			for (SourceBudget& source : sources) {
				source.dispatch_budget = std::min(
					source.available,
					total_requested *
						source.mandatory_floor /
						total_floor
				);
				remaining -= source.dispatch_budget;
			}

			for (SourceBudget& source : sources) {
				if (remaining <= fixed_point_t::_0) {
					break;
				}

				fixed_point_t const headroom = std::max(
					source.available - source.dispatch_budget,
					fixed_point_t::_0
				);

				fixed_point_t const extra = std::min(headroom, remaining);
				source.dispatch_budget += extra;
				remaining -= extra;
			}
			return;
		}

		for (SourceBudget& source : sources) {
			source.dispatch_budget = source.mandatory_floor;
		}

		fixed_point_t remaining =
			std::max(total_requested - total_floor, fixed_point_t::_0);

		// Merit order: explicit priority, then marginal cost, then source id.
		std::vector<size_t> order;
		order.reserve(sources.size());
		for (size_t i = 0; i < sources.size(); ++i) {
			order.push_back(i);
		}

		std::sort(
			order.begin(),
			order.end(),
			[&sources](size_t lhs, size_t rhs) {
				SourceBudget const& a = sources[lhs];
				SourceBudget const& b = sources[rhs];

				if (a.dispatch_priority != b.dispatch_priority) {
					return a.dispatch_priority < b.dispatch_priority;
				}
				if (a.marginal_cost != b.marginal_cost) {
					return a.marginal_cost < b.marginal_cost;
				}
				return a.source_id < b.source_id;
			}
		);

		for (size_t const index : order) {
			if (remaining <= fixed_point_t::_0) {
				break;
			}

			SourceBudget& source = sources[index];
			fixed_point_t const headroom = std::max(
				source.available - source.dispatch_budget,
				fixed_point_t::_0
			);

			if (headroom <= fixed_point_t::_0) {
				continue;
			}

			// A stopped unit only starts when enough demand exists to operate
			// at or above its minimum stable output.
			if (
				source.dispatch_budget <= fixed_point_t::_0 &&
				source.minimum_stable_output > fixed_point_t::_0 &&
				remaining < source.minimum_stable_output
			) {
				continue;
			}

			fixed_point_t const extra = std::min(headroom, remaining);
			source.dispatch_budget += extra;
			remaining -= extra;
		}
	}

	[[nodiscard]] static std::vector<fixed_point_t>
	distribute_source_budget_to_loads(
		SourceBudget const& source,
		std::vector<Load> const& loads,
		fixed_point_t total_requested
	) {
		std::vector<fixed_point_t> allocations(
			loads.size(),
			fixed_point_t::_0
		);

		if (
			source.dispatch_budget <= fixed_point_t::_0 ||
			total_requested <= fixed_point_t::_0
		) {
			return allocations;
		}

		fixed_point_t remaining = source.dispatch_budget;

		for (size_t i = 0; i < loads.size(); ++i) {
			allocations[i] = std::min(
				loads[i].requested,
				source.dispatch_budget *
					loads[i].requested /
					total_requested
			);
			remaining -= allocations[i];
		}

		for (size_t i = 0; i < loads.size(); ++i) {
			if (remaining <= fixed_point_t::_0) {
				break;
			}

			fixed_point_t const unmet = std::max(
				loads[i].requested - allocations[i],
				fixed_point_t::_0
			);

			fixed_point_t const extra = std::min(unmet, remaining);
			allocations[i] += extra;
			remaining -= extra;
		}

		return allocations;
	}

	[[nodiscard]] static std::vector<LogisticsGraphFlowRequest>
	build_initial_requests(
		std::vector<SourceBudget> const& sources,
		std::vector<Load> const& loads,
		fixed_point_t total_requested
	) {
		std::vector<LogisticsGraphFlowRequest> requests;

		for (SourceBudget const& source : sources) {
			auto const load_allocations =
				distribute_source_budget_to_loads(
					source,
					loads,
					total_requested
				);

			for (size_t i = 0; i < loads.size(); ++i) {
				if (load_allocations[i] <= fixed_point_t::_0) {
					continue;
				}

				std::string flow_id = "electricity|";
				flow_id += source.source_id;
				flow_id += "|";
				flow_id += loads[i].employer_id;

				requests.push_back(LogisticsGraphFlowRequest {
					.flow_id = std::move(flow_id),
					.source = source.source_node,
					.destination = loads[i].destination_node,
					.requested = load_allocations[i]
				});
			}
		}

		return requests;
	}

	static void accumulate_allocations(
		std::vector<LogisticsGraphFlowAllocation> const& allocations,
		std::vector<Load>& loads,
		std::vector<SourceLoadDelivery>& deliveries
	) {
		for (Load& load : loads) {
			load.delivered = fixed_point_t::_0;
		}
		deliveries.clear();

		for (LogisticsGraphFlowAllocation const& allocation : allocations) {
			std::string_view const flow = allocation.flow_id;
			std::string_view const prefix = "electricity|";

			if (!flow.starts_with(prefix)) {
				continue;
			}

			size_t const separator = flow.find('|', prefix.size());
			if (separator == std::string_view::npos) {
				continue;
			}

			std::string_view const source_id =
				flow.substr(prefix.size(), separator - prefix.size());
			std::string_view const employer_id =
				flow.substr(separator + 1);

			auto const load_it = std::find_if(
				loads.begin(),
				loads.end(),
				[employer_id](Load const& load) {
					return load.employer_id == employer_id;
				}
			);

			if (load_it != loads.end()) {
				load_it->delivered += allocation.allocated;
			}

			deliveries.push_back(SourceLoadDelivery {
				.source_id = std::string { source_id },
				.employer_id = std::string { employer_id },
				.delivered = allocation.allocated
			});
		}
	}

	[[nodiscard]] static fixed_point_t delivered_by_source(
		std::vector<SourceLoadDelivery> const& deliveries,
		std::string_view source_id
	) {
		fixed_point_t total = fixed_point_t::_0;

		for (SourceLoadDelivery const& delivery : deliveries) {
			if (delivery.source_id == source_id) {
				total += delivery.delivered;
			}
		}

		return total;
	}

	[[nodiscard]] static fixed_point_t delivered_to_load(
		std::vector<SourceLoadDelivery> const& deliveries,
		std::string_view source_id,
		std::string_view employer_id
	) {
		fixed_point_t total = fixed_point_t::_0;

		for (SourceLoadDelivery const& delivery : deliveries) {
			if (
				delivery.source_id == source_id &&
				delivery.employer_id == employer_id
			) {
				total += delivery.delivered;
			}
		}

		return total;
	}

	[[nodiscard]] static std::vector<LogisticsGraphFlowRequest>
	build_redispatched_requests(
		std::vector<SourceBudget> const& sources,
		std::vector<Load> const& loads,
		std::vector<SourceLoadDelivery> const& first_pass_deliveries,
		LogisticsGraph const& transmission_graph
	) {
		std::vector<fixed_point_t> remaining_unmet;
		remaining_unmet.reserve(loads.size());

		for (Load const& load : loads) {
			remaining_unmet.push_back(
				std::max(
					load.requested - load.delivered,
					fixed_point_t::_0
				)
			);
		}

		// extras[source][load]
		std::vector<std::vector<fixed_point_t>> extras(
			sources.size(),
			std::vector<fixed_point_t>(
				loads.size(),
				fixed_point_t::_0
			)
		);

		for (size_t source_index = 0;
				source_index < sources.size();
				++source_index) {
			SourceBudget const& source = sources[source_index];

			fixed_point_t spare = std::max(
				source.available -
					delivered_by_source(
						first_pass_deliveries,
						source.source_id
					),
				fixed_point_t::_0
			);

			if (spare <= fixed_point_t::_0) {
				continue;
			}

			for (size_t load_index = 0;
					load_index < loads.size();
					++load_index) {
				if (
					spare <= fixed_point_t::_0 ||
					remaining_unmet[load_index] <= fixed_point_t::_0
				) {
					continue;
				}

				LogisticsGraphPath const reachable =
					transmission_graph.find_route(
						source.source_node,
						loads[load_index].destination_node
					);

				if (!reachable.found) {
					continue;
				}

				fixed_point_t const extra = std::min(
					spare,
					remaining_unmet[load_index]
				);

				extras[source_index][load_index] = extra;
				spare -= extra;
				remaining_unmet[load_index] -= extra;
			}
		}

		std::vector<LogisticsGraphFlowRequest> requests;

		for (size_t source_index = 0;
				source_index < sources.size();
				++source_index) {
			SourceBudget const& source = sources[source_index];

			for (size_t load_index = 0;
					load_index < loads.size();
					++load_index) {
				Load const& load = loads[load_index];

				fixed_point_t const baseline =
					delivered_to_load(
						first_pass_deliveries,
						source.source_id,
						load.employer_id
					);

				fixed_point_t const requested =
					baseline + extras[source_index][load_index];

				if (requested <= fixed_point_t::_0) {
					continue;
				}

				std::string flow_id = "electricity|";
				flow_id += source.source_id;
				flow_id += "|";
				flow_id += load.employer_id;

				requests.push_back(LogisticsGraphFlowRequest {
					.flow_id = std::move(flow_id),
					.source = source.source_node,
					.destination = load.destination_node,
					.requested = requested
				});
			}
		}

		return requests;
	}

public:
	[[nodiscard]] static std::vector<ProductiveSiteElectricityAllocation>
	resolve_sources_stateful(
		std::vector<ProductiveSiteElectricitySource>& sources,
		LogisticsGraph const& transmission_graph,
		std::vector<ProductiveSiteElectricityConnection> connections,
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::vector<ProductiveSiteUtilityRequirement>& requirements
	) {
		std::sort(
			sources.begin(),
			sources.end(),
			[](ProductiveSiteElectricitySource const& lhs,
				ProductiveSiteElectricitySource const& rhs) {
				return lhs.source_id < rhs.source_id;
			}
		);

		std::sort(
			connections.begin(),
			connections.end(),
			[](ProductiveSiteElectricityConnection const& lhs,
				ProductiveSiteElectricityConnection const& rhs) {
				return lhs.employer_id < rhs.employer_id;
			}
		);

		std::vector<Load> loads;

		for (ProductiveSiteUtilityRequirement& requirement : requirements) {
			if (
				requirement.kind != ProductiveSiteUtilityKind::Electricity ||
				requirement.required_per_output <= fixed_point_t::_0
			) {
				continue;
			}

			ProductiveSiteUtilityTarget const* const target =
				find_target(targets, requirement.employer_id);
			ProductiveSiteElectricityConnection const* const connection =
				find_connection(connections, requirement.employer_id);

			if (
				target == nullptr ||
				target->producer == nullptr ||
				connection == nullptr
			) {
				requirement.available_per_tick = fixed_point_t::_0;
				continue;
			}

			loads.push_back(Load {
				.employer_id = requirement.employer_id,
				.destination_node = connection->destination_node,
				.requested =
					target->producer->calculate_pre_external_desired_output() *
					requirement.required_per_output
			});
		}

		std::sort(
			loads.begin(),
			loads.end(),
			[](Load const& lhs, Load const& rhs) {
				return lhs.employer_id < rhs.employer_id;
			}
		);

		std::vector<SourceBudget> source_budgets;
		source_budgets.reserve(sources.size());

		for (ProductiveSiteElectricitySource const& source : sources) {
			fixed_point_t const nameplate = std::max(
				source.available_generation_per_tick,
				fixed_point_t::_0
			);

			fixed_point_t const availability = std::clamp(
				source.availability_fraction,
				fixed_point_t::_0,
				fixed_point_t::_1
			);

			fixed_point_t const availability_limited =
				nameplate * availability;

			fixed_point_t const ramp_up_limited = std::min(
				availability_limited,
				source.current_dispatch_per_tick +
					std::max(
						source.ramp_up_per_tick,
						fixed_point_t::_0
					)
			);

			fixed_point_t const ramp_down_floor = std::max(
				source.current_dispatch_per_tick -
					std::max(
						source.ramp_down_per_tick,
						fixed_point_t::_0
					),
				fixed_point_t::_0
			);

			fixed_point_t mandatory_floor = std::min(
				ramp_up_limited,
				ramp_down_floor
			);

			if (
				source.current_dispatch_per_tick > fixed_point_t::_0 &&
				ramp_up_limited >= source.minimum_stable_output
			) {
				mandatory_floor = std::max(
					mandatory_floor,
					std::max(
						source.minimum_stable_output,
						fixed_point_t::_0
					)
				);
			}

			source_budgets.push_back(SourceBudget {
				.source_id = source.source_id,
				.source_node = source.source_node,
				.available = ramp_up_limited,
				.minimum_stable_output = std::max(
					source.minimum_stable_output,
					fixed_point_t::_0
				),
				.mandatory_floor = std::min(
					mandatory_floor,
					ramp_up_limited
				),
				.marginal_cost = source.marginal_cost,
				.dispatch_priority = source.dispatch_priority
			});
		}

		fixed_point_t total_requested = fixed_point_t::_0;
		for (Load const& load : loads) {
			total_requested += load.requested;
		}

		assign_source_dispatch_budgets(
			source_budgets,
			total_requested
		);

		auto initial_requests = build_initial_requests(
			source_budgets,
			loads,
			total_requested
		);

		auto allocations =
			transmission_graph.allocate_flows(initial_requests);

		std::vector<SourceLoadDelivery> first_pass_deliveries;
		accumulate_allocations(
			allocations,
			loads,
			first_pass_deliveries
		);

		fixed_point_t total_unmet = fixed_point_t::_0;
		for (Load const& load : loads) {
			total_unmet += std::max(
				load.requested - load.delivered,
				fixed_point_t::_0
			);
		}

		if (total_unmet > fixed_point_t::_0) {
			auto final_requests = build_redispatched_requests(
				source_budgets,
				loads,
				first_pass_deliveries,
				transmission_graph
			);

			if (!final_requests.empty()) {
				allocations =
					transmission_graph.allocate_flows(final_requests);

				std::vector<SourceLoadDelivery> final_deliveries;
				accumulate_allocations(
					allocations,
					loads,
					final_deliveries
				);
			}
		}

		std::vector<ProductiveSiteElectricityAllocation> results;
		results.reserve(loads.size());

		for (Load const& load : loads) {
			results.push_back(ProductiveSiteElectricityAllocation {
				.employer_id = load.employer_id,
				.requested = load.requested,
				.transmission_allocated = load.delivered,
				.delivered = load.delivered
			});

			for (ProductiveSiteUtilityRequirement& requirement :
					requirements) {
				if (
					requirement.kind ==
						ProductiveSiteUtilityKind::Electricity &&
					requirement.employer_id == load.employer_id
				) {
					requirement.available_per_tick = load.delivered;
					break;
				}
			}
		}

		for (ProductiveSiteElectricitySource& source : sources) {
			source.current_dispatch_per_tick =
				delivered_by_source(
					allocations.empty()
						? std::vector<SourceLoadDelivery> {}
						: [&]() {
							std::vector<SourceLoadDelivery> delivered;
							std::vector<Load> load_copy = loads;
							accumulate_allocations(
								allocations,
								load_copy,
								delivered
							);
							return delivered;
						}(),
					source.source_id
				);
		}

		return results;
	}

	// Stateless compatibility wrapper for B10/B11 direct callers.
	[[nodiscard]] static std::vector<ProductiveSiteElectricityAllocation>
	resolve_sources(
		std::vector<ProductiveSiteElectricitySource> sources,
		LogisticsGraph const& transmission_graph,
		std::vector<ProductiveSiteElectricityConnection> connections,
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::vector<ProductiveSiteUtilityRequirement>& requirements
	) {
		return resolve_sources_stateful(
			sources,
			transmission_graph,
			std::move(connections),
			targets,
			requirements
		);
	}

	// B9 compatibility wrapper. Preserve the original B9 meaning of
	// transmission_allocated: raw shared-line capacity before generation
	// scarcity is applied. delivered remains the actual electrical service.
	[[nodiscard]] static std::vector<ProductiveSiteElectricityAllocation> resolve(
		ProductiveSiteElectricityGridState const& grid,
		LogisticsGraph const& transmission_graph,
		std::vector<ProductiveSiteElectricityConnection> const& connections,
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::vector<ProductiveSiteUtilityRequirement>& requirements
	) {
		std::vector<LogisticsGraphFlowRequest> raw_requests;

		for (ProductiveSiteUtilityRequirement const& requirement : requirements) {
			if (
				requirement.kind != ProductiveSiteUtilityKind::Electricity ||
				requirement.required_per_output <= fixed_point_t::_0
			) {
				continue;
			}

			ProductiveSiteUtilityTarget const* const target =
				find_target(targets, requirement.employer_id);
			ProductiveSiteElectricityConnection const* const connection =
				find_connection(connections, requirement.employer_id);

			if (
				target == nullptr ||
				target->producer == nullptr ||
				connection == nullptr
			) {
				continue;
			}

			std::string flow_id = "electricity|";
			flow_id += requirement.employer_id;

			raw_requests.push_back(LogisticsGraphFlowRequest {
				.flow_id = std::move(flow_id),
				.source = grid.source_node,
				.destination = connection->destination_node,
				.requested =
					target->producer->calculate_pre_external_desired_output() *
					requirement.required_per_output
			});
		}

		auto const raw_allocations =
			transmission_graph.allocate_flows(raw_requests);

		auto results = resolve_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "grid_source",
					.source_node = grid.source_node,
					.available_generation_per_tick =
						grid.available_generation_per_tick
				}
			},
			transmission_graph,
			connections,
			targets,
			requirements
		);

		for (ProductiveSiteElectricityAllocation& result : results) {
			std::string flow_id = "electricity|";
			flow_id += result.employer_id;

			auto const allocation_it = std::find_if(
				raw_allocations.begin(),
				raw_allocations.end(),
				[&flow_id](LogisticsGraphFlowAllocation const& allocation) {
					return allocation.flow_id == flow_id;
				}
			);

			result.transmission_allocated =
				allocation_it != raw_allocations.end()
					? allocation_it->allocated
					: fixed_point_t::_0;
		}

		return results;
	}
};

}