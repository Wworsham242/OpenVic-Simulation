#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/trading/TransportCorridor.hpp"

namespace OpenVic {

struct LogisticsGraphEdge final {
	std::string edge_id;
	market_node_index_t source {};
	market_node_index_t destination {};
	TransportLeg leg {};
};

struct LogisticsGraphPath final {
	bool found = false;
	fixed_point_t bottleneck_capacity = 0;
	std::vector<std::string> edge_ids;
};
struct LogisticsGraphFlowRequest final {
	std::string flow_id;
	market_node_index_t source {};
	market_node_index_t destination {};
	fixed_point_t requested = 0;
};

struct LogisticsGraphFlowAllocation final {
	std::string flow_id;
	fixed_point_t requested = 0;
	fixed_point_t allocated = 0;
	LogisticsGraphPath path {};
};

/// Small deterministic graph for coarse strategic logistics routing.
///
/// Route choice is:
/// 1. usable/open edges only;
/// 2. fewest hops;
/// 3. lexicographically smallest edge-id sequence as deterministic tie-break.
///
/// This is not a microscopic vehicle router.
class LogisticsGraph final {
private:
	std::vector<LogisticsGraphEdge> edges;

	struct Candidate final {
		market_node_index_t node {};
		fixed_point_t bottleneck = fixed_point_t::usable_max;
		std::vector<std::string> edge_ids;
		std::vector<market_node_index_t> visited_nodes;
	};

	[[nodiscard]] static bool contains_node(
		std::vector<market_node_index_t> const& nodes,
		market_node_index_t node
	) {
		return std::find(nodes.begin(), nodes.end(), node) != nodes.end();
	}

public:
	[[nodiscard]] bool configure(std::vector<LogisticsGraphEdge> new_edges) {
		for (LogisticsGraphEdge const& edge : new_edges) {
			if (edge.edge_id.empty()) {
				return false;
			}
		}

		std::sort(
			new_edges.begin(),
			new_edges.end(),
			[](LogisticsGraphEdge const& lhs, LogisticsGraphEdge const& rhs) {
				return lhs.edge_id < rhs.edge_id;
			}
		);

		for (size_t i = 1; i < new_edges.size(); ++i) {
			if (new_edges[i - 1].edge_id == new_edges[i].edge_id) {
				return false;
			}
		}

		edges = std::move(new_edges);
		return true;
	}

	[[nodiscard]] bool set_edge_open(
		std::string_view edge_id,
		bool open
	) {
		for (LogisticsGraphEdge& edge : edges) {
			if (edge.edge_id == edge_id) {
				edge.leg.open = open;
				return true;
			}
		}
		return false;
	}
	[[nodiscard]] fixed_point_t get_edge_effective_capacity(
		std::string_view edge_id
	) const {
		for (LogisticsGraphEdge const& edge : edges) {
			if (edge.edge_id == edge_id) {
				return edge.leg.calculate_effective_capacity();
			}
		}
		return fixed_point_t::_0;
	}

	[[nodiscard]] LogisticsGraphPath find_route(
		market_node_index_t source,
		market_node_index_t destination
	) const {
		if (source == destination) {
			return LogisticsGraphPath {
				.found = true,
				.bottleneck_capacity = fixed_point_t::usable_max
			};
		}

		std::vector<Candidate> frontier {
			Candidate {
				.node = source,
				.bottleneck = fixed_point_t::usable_max,
				.visited_nodes = { source }
			}
		};

		while (!frontier.empty()) {
			std::vector<Candidate> next;

			std::sort(
				frontier.begin(),
				frontier.end(),
				[](Candidate const& lhs, Candidate const& rhs) {
					return lhs.edge_ids < rhs.edge_ids;
				}
			);

			for (Candidate const& candidate : frontier) {
				if (candidate.node == destination) {
					return LogisticsGraphPath {
						.found = true,
						.bottleneck_capacity = candidate.bottleneck,
						.edge_ids = candidate.edge_ids
					};
				}
			}

			for (Candidate const& candidate : frontier) {
				for (LogisticsGraphEdge const& edge : edges) {
					if (edge.source != candidate.node) {
						continue;
					}

					fixed_point_t const edge_capacity =
						edge.leg.calculate_effective_capacity();

					if (
						edge_capacity <= fixed_point_t::_0 ||
						contains_node(candidate.visited_nodes, edge.destination)
					) {
						continue;
					}

					Candidate expanded = candidate;
					expanded.node = edge.destination;
					expanded.bottleneck =
						std::min(expanded.bottleneck, edge_capacity);
					expanded.edge_ids.push_back(edge.edge_id);
					expanded.visited_nodes.push_back(edge.destination);
					next.push_back(std::move(expanded));
				}
			}

			frontier = std::move(next);
		}

		return {};
	}

	/// Allocate multiple independently routed flows across shared graph edges.
	///
	/// Each flow first receives a deterministic route. For every edge used by
	/// one or more flows, requested quantities are summed. If demand exceeds
	/// edge capacity, all users of that edge receive the same proportional
	/// physical-capacity factor. A flow's final allocation uses the tightest
	/// factor across all edges on its selected path.
	///
	/// This is intentionally conservative and deterministic. It proves shared
	/// physical edge contention before more advanced reservation/queue logic.
	[[nodiscard]] std::vector<LogisticsGraphFlowAllocation> allocate_flows(
		std::vector<LogisticsGraphFlowRequest> const& requests
	) const {
		std::vector<LogisticsGraphFlowAllocation> allocations;
		allocations.reserve(requests.size());

		for (LogisticsGraphFlowRequest const& request : requests) {
			allocations.push_back(LogisticsGraphFlowAllocation {
				.flow_id = request.flow_id,
				.requested = request.requested,
				.allocated = fixed_point_t::_0,
				.path = find_route(request.source, request.destination)
			});
		}

		for (size_t i = 0; i < allocations.size(); ++i) {
			LogisticsGraphFlowAllocation& allocation = allocations[i];

			if (
				!allocation.path.found ||
				allocation.requested <= fixed_point_t::_0
			) {
				continue;
			}

			fixed_point_t limiting_fraction = fixed_point_t::_1;

			for (std::string const& edge_id : allocation.path.edge_ids) {
				fixed_point_t total_edge_request = 0;

				for (size_t j = 0; j < allocations.size(); ++j) {
					LogisticsGraphFlowAllocation const& other = allocations[j];

					if (
						!other.path.found ||
						other.requested <= fixed_point_t::_0
					) {
						continue;
					}

					if (
						std::find(
							other.path.edge_ids.begin(),
							other.path.edge_ids.end(),
							edge_id
						) != other.path.edge_ids.end()
					) {
						total_edge_request += other.requested;
					}
				}

				fixed_point_t const capacity =
					get_edge_effective_capacity(edge_id);

				fixed_point_t const edge_fraction =
					total_edge_request > fixed_point_t::_0
						? std::min(
							fixed_point_t::_1,
							capacity / total_edge_request
						)
						: fixed_point_t::_1;

				limiting_fraction =
					std::min(limiting_fraction, edge_fraction);
			}

			allocation.allocated =
				allocation.requested * limiting_fraction;
		}

		return allocations;
	}
};

}