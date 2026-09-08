#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"
#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"
#include "openvic-simulation/resources/ResourceSupplyNetwork.hpp"

namespace OpenVic {

struct ProductiveSiteResourceRoute final {
	std::string employer_id;
	market_node_index_t source_node {};
	market_node_index_t destination_node {};
	GoodDefinition const* good = nullptr;
};

struct ProductiveSiteMaterialInput final {
	GoodDefinition const* good = nullptr;
	fixed_point_t desired_flow = fixed_point_t::_0;
	ResourceSupplyNetwork* network = nullptr;
	std::vector<ResourceSourceAccess> source_access;
	bool primary = false;
};

struct ProductiveSiteMaterialTarget final {
	std::string employer_id;
	AggregateProducer* producer = nullptr;
	AggregateProducerMarketBridge* bridge = nullptr;
};

class ProductiveSiteMaterialFlowResolver final {
private:
	struct SourceTarget final {
		std::string employer_id;
		AggregateProducer* producer = nullptr;
		fixed_point_t shortfall = fixed_point_t::_0;
		fixed_point_t accessible_request = fixed_point_t::_0;
		fixed_point_t allocated = fixed_point_t::_0;
		std::string flow_id;
	};

	struct InputFlow final {
		GoodDefinition const* good = nullptr;
		fixed_point_t desired_flow = fixed_point_t::_0;
		ResourceSupplyNetwork* network = nullptr;
		std::vector<ResourceSourceAccess> source_access;
		std::vector<SourceTarget> targets;
		ResourceFlowResult result {};
		bool primary = false;
	};

	[[nodiscard]] static ProductiveSiteResourceRoute const* find_route(
		std::vector<ProductiveSiteResourceRoute> const& routes,
		std::string_view employer_id,
		GoodDefinition const& good
	) {
		auto const it = std::find_if(
			routes.begin(), routes.end(),
			[employer_id, &good](ProductiveSiteResourceRoute const& route) {
				return route.employer_id == employer_id && route.good == &good;
			}
		);
		return it != routes.end() ? &*it : nullptr;
	}

	[[nodiscard]] static bool has_routes_for(
		std::vector<ProductiveSiteResourceRoute> const& routes,
		GoodDefinition const& good
	) {
		return std::any_of(
			routes.begin(), routes.end(),
			[&good](ProductiveSiteResourceRoute const& route) {
				return route.good == &good;
			}
		);
	}

public:
	[[nodiscard]] static ResourceFlowResult resolve(
		std::vector<ProductiveSiteMaterialInput> inputs,
		std::vector<ProductiveSiteMaterialTarget> const& material_targets,
		std::vector<ProductiveSiteResourceRoute> const& routes,
		LogisticsGraph const& logistics_graph
	) {
		std::vector<InputFlow> flows;
		flows.reserve(inputs.size());

		for (ProductiveSiteMaterialInput& input : inputs) {
			if (input.good == nullptr || input.network == nullptr) { continue; }
			flows.push_back(InputFlow {
				.good = input.good,
				.desired_flow = input.desired_flow,
				.network = input.network,
				.source_access = std::move(input.source_access),
				.primary = input.primary
			});
		}

		std::sort(
			flows.begin(), flows.end(),
			[](InputFlow const& lhs, InputFlow const& rhs) {
				if (lhs.primary != rhs.primary) { return lhs.primary; }
				return lhs.good->get_identifier() < rhs.good->get_identifier();
			}
		);

		std::vector<LogisticsGraphFlowRequest> graph_requests;

		for (InputFlow& input : flows) {
			GoodDefinition const& good = *input.good;

			for (ProductiveSiteMaterialTarget const& target : material_targets) {
				if (target.producer == nullptr || target.bridge == nullptr) { continue; }

				input.targets.push_back(SourceTarget {
					.employer_id = target.employer_id,
					.producer = target.producer,
					.shortfall = target.bridge->calculate_input_shortfall(good)
				});
			}

			std::sort(
				input.targets.begin(), input.targets.end(),
				[](SourceTarget const& lhs, SourceTarget const& rhs) {
					return lhs.employer_id < rhs.employer_id;
				}
			);

			bool const has_explicit_routes = has_routes_for(routes, good);

			for (SourceTarget& target : input.targets) {
				if (!has_explicit_routes || target.shortfall <= fixed_point_t::_0) {
					target.accessible_request = target.shortfall;
					continue;
				}

				ProductiveSiteResourceRoute const* const route =
					find_route(routes, target.employer_id, good);

				if (route == nullptr) {
					target.accessible_request = target.shortfall;
					continue;
				}

				target.flow_id = std::string { good.get_identifier() };
				target.flow_id += "|";
				target.flow_id += target.employer_id;

				graph_requests.push_back(LogisticsGraphFlowRequest {
					.flow_id = target.flow_id,
					.source = route->source_node,
					.destination = route->destination_node,
					.requested = target.shortfall
				});
			}
		}

		auto const graph_allocations = logistics_graph.allocate_flows(graph_requests);

		for (LogisticsGraphFlowAllocation const& allocation : graph_allocations) {
			for (InputFlow& input : flows) {
				auto const target_it = std::find_if(
					input.targets.begin(), input.targets.end(),
					[&allocation](SourceTarget const& target) {
						return !target.flow_id.empty() && target.flow_id == allocation.flow_id;
					}
				);

				if (target_it != input.targets.end()) {
					target_it->accessible_request =
						std::min(target_it->shortfall, allocation.allocated);
					break;
				}
			}
		}

		ResourceFlowResult primary_result {};

		for (InputFlow& input : flows) {
			fixed_point_t total_accessible_request = fixed_point_t::_0;
			for (SourceTarget const& target : input.targets) {
				total_accessible_request += target.accessible_request;
			}

			fixed_point_t const desired_source_flow =
				std::max(input.desired_flow, fixed_point_t::_0);

			fixed_point_t const reachable_demand =
				std::min(desired_source_flow, total_accessible_request);

			input.result = input.network->fulfill(
				reachable_demand,
				input.source_access
			);

			fixed_point_t const route_unmet = desired_source_flow - reachable_demand;

			if (
				route_unmet > fixed_point_t::_0 &&
				total_accessible_request < desired_source_flow
			) {
				input.result.requested = desired_source_flow;
				input.result.unmet += route_unmet;
				input.result.source_access_limited = true;
			}

			if (
				input.result.delivered > fixed_point_t::_0 &&
				total_accessible_request > fixed_point_t::_0
			) {
				fixed_point_t remaining = input.result.delivered;

				for (SourceTarget& target : input.targets) {
					if (target.accessible_request <= fixed_point_t::_0) { continue; }

					fixed_point_t const share = std::min(
						target.accessible_request,
						input.result.delivered * target.accessible_request /
							total_accessible_request
					);

					target.producer->add_inventory(*input.good, share);
					target.allocated += share;
					remaining -= share;
				}

				for (SourceTarget& target : input.targets) {
					if (remaining <= fixed_point_t::_0) { break; }

					fixed_point_t const unmet = std::max(
						target.accessible_request - target.allocated,
						fixed_point_t::_0
					);

					fixed_point_t const extra = std::min(unmet, remaining);

					if (extra > fixed_point_t::_0) {
						target.producer->add_inventory(*input.good, extra);
						target.allocated += extra;
						remaining -= extra;
					}
				}
			}

			if (input.primary) { primary_result = input.result; }
		}

		return primary_result;
	}
};

}