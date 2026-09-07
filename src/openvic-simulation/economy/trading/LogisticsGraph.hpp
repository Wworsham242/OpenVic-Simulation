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
};

}