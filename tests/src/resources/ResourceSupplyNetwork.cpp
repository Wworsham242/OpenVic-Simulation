#include "openvic-simulation/resources/ResourceSupplyNetwork.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Resource network aggregates multiple sources and buffers partial disruption",
	"[resources][network][buffer]"
) {
	ResourceSupplyNetwork network {
		{
			ResourceSourceState {
				.source_id = "source_a",
				.node = market_node_index_t { 10 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			},
			ResourceSourceState {
				.source_id = "source_b",
				.node = market_node_index_t { 20 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			}
		},
		ResourceBufferState {
			.capacity = fixed_point_t(4),
			.inventory = fixed_point_t(4)
		}
	};

	REQUIRE(network.is_valid());
	CHECK(network.source_count() == 2);
	CHECK(network.nominal_supply_per_tick() == fixed_point_t(4));
	CHECK(network.accessible_supply_per_tick() == fixed_point_t(4));

	REQUIRE(network.set_source_availability("source_a", fixed_point_t::_0));
	CHECK(network.nominal_supply_per_tick() == fixed_point_t(4));
	CHECK(network.accessible_supply_per_tick() == fixed_point_t(2));

	ResourceFlowResult first = network.fulfill(fixed_point_t(4));
	CHECK(first.delivered == fixed_point_t(4));
	CHECK(first.buffer_draw == fixed_point_t(2));
	CHECK(first.unmet == fixed_point_t::_0);
	CHECK(network.buffer_inventory() == fixed_point_t(2));

	ResourceFlowResult second = network.fulfill(fixed_point_t(4));
	CHECK(second.delivered == fixed_point_t(4));
	CHECK(network.buffer_inventory() == fixed_point_t::_0);

	ResourceFlowResult third = network.fulfill(fixed_point_t(4));
	CHECK(third.delivered == fixed_point_t(2));
	CHECK(third.unmet == fixed_point_t(2));
}
TEST_CASE(
	"Logistics access can isolate an intact resource source",
	"[resources][network][logistics]"
) {
	ResourceSupplyNetwork network {
		{
			ResourceSourceState {
				.source_id = "source_a",
				.node = market_node_index_t { 10 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			},
			ResourceSourceState {
				.source_id = "source_b",
				.node = market_node_index_t { 20 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			}
		}
	};

	REQUIRE(network.is_valid());
	CHECK(network.accessible_supply_per_tick() == fixed_point_t(4));

	std::vector<ResourceSourceAccess> access {
		ResourceSourceAccess {
			.source_id = "source_a",
			.delivery_capacity = fixed_point_t(2),
			.accessible_fraction = fixed_point_t::_1,
			.access_allowed = false
		},
		ResourceSourceAccess {
			.source_id = "source_b",
			.delivery_capacity = fixed_point_t(2),
			.accessible_fraction = fixed_point_t::_1,
			.access_allowed = true
		}
	};

	CHECK(network.deliverable_supply_per_tick(access) == fixed_point_t(2));

	ResourceFlowResult flow = network.fulfill(fixed_point_t(4), access);
	CHECK(flow.delivered == fixed_point_t(2));
	CHECK(flow.unmet == fixed_point_t(2));
}