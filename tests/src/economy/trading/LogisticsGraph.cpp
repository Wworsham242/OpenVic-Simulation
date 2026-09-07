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
TEST_CASE(
	"Overlapping graph routes share physical edge capacity",
	"[logistics][graph][shared-edge]"
) {
	LogisticsGraph graph;

	REQUIRE(graph.configure({
		LogisticsGraphEdge {
			.edge_id = "mine_a_feeder",
			.source = market_node_index_t { 11 },
			.destination = market_node_index_t { 30 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "mine_b_feeder",
			.source = market_node_index_t { 12 },
			.destination = market_node_index_t { 30 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "shared_trunk",
			.source = market_node_index_t { 30 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(3)
			}
		}
	}));

	auto allocations = graph.allocate_flows({
		LogisticsGraphFlowRequest {
			.flow_id = "mine_a",
			.source = market_node_index_t { 11 },
			.destination = market_node_index_t { 22 },
			.requested = fixed_point_t(2)
		},
		LogisticsGraphFlowRequest {
			.flow_id = "mine_b",
			.source = market_node_index_t { 12 },
			.destination = market_node_index_t { 22 },
			.requested = fixed_point_t(2)
		}
	});

	REQUIRE(allocations.size() == 2);
	REQUIRE(allocations[0].path.found);
	REQUIRE(allocations[1].path.found);

	CHECK(
		allocations[0].allocated * fixed_point_t(2)
		== fixed_point_t(3)
	);
	CHECK(
		allocations[1].allocated * fixed_point_t(2)
		== fixed_point_t(3)
	);
	CHECK(
		allocations[0].allocated + allocations[1].allocated
		== fixed_point_t(3)
	);
}
TEST_CASE(
	"Residual constrained flow reroutes onto spare alternate capacity",
	"[logistics][graph][residual-rerouting]"
) {
	LogisticsGraph graph;

	REQUIRE(graph.configure({
		LogisticsGraphEdge {
			.edge_id = "a_primary_1",
			.source = market_node_index_t { 1 },
			.destination = market_node_index_t { 2 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "a_primary_2",
			.source = market_node_index_t { 2 },
			.destination = market_node_index_t { 4 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
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

	auto allocations = graph.allocate_flows({
		LogisticsGraphFlowRequest {
			.flow_id = "mine_a",
			.source = market_node_index_t { 1 },
			.destination = market_node_index_t { 4 },
			.requested = fixed_point_t(2)
		}
	});

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].requested == fixed_point_t(2));
	CHECK(allocations[0].allocated == fixed_point_t(2));
	CHECK(allocations[0].rerouted_allocated == fixed_point_t(1));
	REQUIRE(allocations[0].alternate_path.found);
	REQUIRE(allocations[0].alternate_path.edge_ids.size() == 2);
	CHECK(allocations[0].alternate_path.edge_ids[0] == "b_alternate_1");
}