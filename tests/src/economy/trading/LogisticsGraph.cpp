#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Logistics graph reroutes deterministically when primary edge closes",
	"[logistics][graph][routing]"
) {
	LogisticsGraph graph;

	REQUIRE(graph.configure({
		LogisticsGraphEdge {
			.edge_id = "a_primary_1",
			.source = market_node_index_t { 1 },
			.destination = market_node_index_t { 2 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "a_primary_2",
			.source = market_node_index_t { 2 },
			.destination = market_node_index_t { 4 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "b_alternate_1",
			.source = market_node_index_t { 1 },
			.destination = market_node_index_t { 3 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "b_alternate_2",
			.source = market_node_index_t { 3 },
			.destination = market_node_index_t { 4 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		}
	}));

	LogisticsGraphPath primary = graph.find_route(
		market_node_index_t { 1 },
		market_node_index_t { 4 }
	);

	REQUIRE(primary.found);
	CHECK(primary.bottleneck_capacity == fixed_point_t(2));
	REQUIRE(primary.edge_ids.size() == 2);
	CHECK(primary.edge_ids[0] == "a_primary_1");

	REQUIRE(graph.set_edge_open("a_primary_2", false));

	LogisticsGraphPath alternate = graph.find_route(
		market_node_index_t { 1 },
		market_node_index_t { 4 }
	);

	REQUIRE(alternate.found);
	CHECK(alternate.bottleneck_capacity == fixed_point_t(1));
	REQUIRE(alternate.edge_ids.size() == 2);
	CHECK(alternate.edge_ids[0] == "b_alternate_1");
}