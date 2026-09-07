#include "openvic-simulation/GameManager.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Application-owned native ruleset starts authoritative economy and advances causal production",
	"[convergence][native-ruleset][live-economy]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	std::filesystem::path const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	LiveEconomyStatus status = instance->get_live_economy_status();
	REQUIRE(status.configured);
	CHECK(status.completed_daily_ticks == 0);

	REQUIRE(instance->bootstrap_position_occupancy(
		PositionOccupancy {
			.position = PositionIdentity {
				.position_id = "native_national_executive",
				.jurisdiction_id = "native_state"
			},
			.controller_id = "test_human_controller",
			.controller_kind = PositionControllerKind::human
		},
		std::vector<AuthorityGrant> {
			AuthorityGrant {
				.command_type = "native.policy.test",
				.jurisdiction_id = "native_state"
			}
		}
	));

	CHECK(
		instance->submit_occupied_position_command(
			"native.policy.test",
			std::vector<uint8_t> {}
		) == CommandAdmissionResult::accepted
	);
	CHECK(instance->get_accepted_command_count() == 1);

	REQUIRE(manager.start_game_session());

	SimTime const before = instance->get_simulation_time();

	for (int i = 0; i < 8; ++i) {
		instance->force_tick_and_update();
	}

	SimTime const after = instance->get_simulation_time();
	status = instance->get_live_economy_status();

	CHECK(after > before);
	CHECK(status.configured);
	CHECK(status.completed_daily_ticks > 0);
	CHECK(status.upstream_output > fixed_point_t::_0);
	CHECK(status.intermediate_quantity_traded_yesterday > fixed_point_t::_0);

	REQUIRE(manager.end_game_session());
}

TEST_CASE(
	"Native economy bootstrap rejects incomplete application package",
	"[convergence][native-ruleset][validation]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	std::filesystem::path const missing =
		std::filesystem::temp_directory_path()
		/ "openvic-native-ruleset-bootstrap-does-not-exist";

	CHECK_FALSE(manager.load_native_economy_bootstrap(missing));
}
TEST_CASE(
	"Resource availability shock propagates into authoritative industrial output",
	"[convergence][native-ruleset][resource-coupling]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	std::filesystem::path const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	REQUIRE(manager.start_game_session());

	instance->force_tick_and_update();

	LiveEconomyStatus before_shock = instance->get_live_economy_status();
	REQUIRE(before_shock.completed_daily_ticks > 0);
	CHECK(before_shock.source_nominal_inflow == fixed_point_t(4));
	CHECK(before_shock.source_availability_fraction == fixed_point_t::_1);
	CHECK(before_shock.source_accessible_inflow == fixed_point_t(4));
	CHECK(before_shock.upstream_output == fixed_point_t(4));

	// The resources domain publishes a total source outage. The economy does
	// not receive a scripted "-production" modifier; it receives zero
	// accessible physical feedstock and reacts through normal production.
	REQUIRE(instance->set_live_resource_availability(fixed_point_t::_0));

	instance->force_tick_and_update();

	LiveEconomyStatus after_shock = instance->get_live_economy_status();
	CHECK(after_shock.source_nominal_inflow == fixed_point_t(4));
	CHECK(after_shock.source_availability_fraction == fixed_point_t::_0);
	CHECK(after_shock.source_accessible_inflow == fixed_point_t::_0);
	CHECK(after_shock.upstream_output == fixed_point_t::_0);

	REQUIRE(manager.end_game_session());
}

TEST_CASE(
	"Buffered multi-source disruption delays industrial output loss",
	"[convergence][native-ruleset][resource-network]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	std::filesystem::path const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	REQUIRE(instance->configure_live_resource_supply_network(
		std::vector<ResourceSourceState> {
			ResourceSourceState {
				.source_id = "mine_a",
				.node = market_node_index_t { 11 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			},
			ResourceSourceState {
				.source_id = "mine_b",
				.node = market_node_index_t { 12 },
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
	));

	REQUIRE(manager.start_game_session());

	LiveEconomyStatus configured = instance->get_live_economy_status();
	CHECK(configured.source_count == 2);
	CHECK(configured.source_nominal_inflow == fixed_point_t(4));
	CHECK(configured.source_accessible_inflow == fixed_point_t(4));
	CHECK(configured.source_buffer_inventory == fixed_point_t(4));

	REQUIRE(instance->set_live_resource_source_availability(
		"mine_a",
		fixed_point_t::_0
	));

	instance->force_tick_and_update();
	LiveEconomyStatus first = instance->get_live_economy_status();
	CHECK(first.source_accessible_inflow == fixed_point_t(2));
	CHECK(first.source_buffer_draw == fixed_point_t(2));
	CHECK(first.source_buffer_inventory == fixed_point_t(2));
	CHECK(first.source_unmet_inflow == fixed_point_t::_0);
	CHECK(first.upstream_output == fixed_point_t(4));

	instance->force_tick_and_update();
	LiveEconomyStatus second = instance->get_live_economy_status();
	CHECK(second.source_buffer_inventory == fixed_point_t::_0);
	CHECK(second.source_unmet_inflow == fixed_point_t::_0);
	CHECK(second.upstream_output == fixed_point_t(4));

	instance->force_tick_and_update();
	LiveEconomyStatus third = instance->get_live_economy_status();
	CHECK(third.source_buffer_inventory == fixed_point_t::_0);
	CHECK(third.source_unmet_inflow == fixed_point_t(2));
	CHECK(third.upstream_output == fixed_point_t(2));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Transport disruption isolates intact resource supply and consumes buffer",
	"[convergence][native-ruleset][resource-logistics]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	std::filesystem::path const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	REQUIRE(instance->configure_live_resource_supply_network(
		std::vector<ResourceSourceState> {
			ResourceSourceState {
				.source_id = "mine_a",
				.node = market_node_index_t { 11 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			},
			ResourceSourceState {
				.source_id = "mine_b",
				.node = market_node_index_t { 12 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			}
		},
		ResourceBufferState {
			.capacity = fixed_point_t(2),
			.inventory = fixed_point_t(2)
		}
	));

	TransportCorridor route_a {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	route_a.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	TransportCorridor route_b {
		market_node_index_t { 12 },
		market_node_index_t { 22 }
	};
	route_b.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	REQUIRE(instance->configure_live_resource_source_routes(
		std::vector<ResourceSourceRoute> {
			ResourceSourceRoute { "mine_a", std::move(route_a) },
			ResourceSourceRoute { "mine_b", std::move(route_b) }
		}
	));

	REQUIRE(manager.start_game_session());

	// The mine remains fully productive. Only its logistics route is denied.
	REQUIRE(instance->set_live_resource_route_access("mine_a", false));

	instance->force_tick_and_update();
	LiveEconomyStatus first = instance->get_live_economy_status();

	CHECK(first.source_nominal_inflow == fixed_point_t(4));
	CHECK(first.source_buffer_draw == fixed_point_t(2));
	CHECK(first.source_buffer_inventory == fixed_point_t::_0);
	CHECK(first.source_unmet_inflow == fixed_point_t::_0);
	CHECK(first.upstream_output == fixed_point_t(4));

	instance->force_tick_and_update();
	LiveEconomyStatus second = instance->get_live_economy_status();

	CHECK(second.source_nominal_inflow == fixed_point_t(4));
	CHECK(second.source_buffer_inventory == fixed_point_t::_0);
	CHECK(second.source_unmet_inflow == fixed_point_t(2));
	CHECK(second.upstream_output == fixed_point_t(2));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Shared logistics segment constrains competing intact resource flows",
	"[convergence][native-ruleset][shared-logistics]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	std::filesystem::path const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	REQUIRE(instance->configure_live_resource_supply_network(
		std::vector<ResourceSourceState> {
			ResourceSourceState {
				.source_id = "mine_a",
				.node = market_node_index_t { 11 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			},
			ResourceSourceState {
				.source_id = "mine_b",
				.node = market_node_index_t { 12 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t(2),
					.availability_fraction = fixed_point_t::_1
				}
			}
		},
		ResourceBufferState {}
	));

	TransportCorridor route_a {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	route_a.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	TransportCorridor route_b {
		market_node_index_t { 12 },
		market_node_index_t { 22 }
	};
	route_b.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	REQUIRE(instance->configure_live_resource_source_routes(
		std::vector<ResourceSourceRoute> {
			ResourceSourceRoute {
				"mine_a",
				std::move(route_a),
				true,
				fixed_point_t::_1,
				"shared_rail"
			},
			ResourceSourceRoute {
				"mine_b",
				std::move(route_b),
				true,
				fixed_point_t::_1,
				"shared_rail"
			}
		}
	));

	REQUIRE(instance->configure_live_shared_transport_capacities(
		std::vector<SharedTransportCapacity> {
			SharedTransportCapacity {
				"shared_rail",
				fixed_point_t(3)
			}
		}
	));

	REQUIRE(manager.start_game_session());

	instance->force_tick_and_update();
	LiveEconomyStatus status = instance->get_live_economy_status();

	CHECK(status.source_nominal_inflow == fixed_point_t(4));
	CHECK(status.source_unmet_inflow == fixed_point_t(1));
	CHECK(status.upstream_output == fixed_point_t(3));

	REQUIRE(manager.end_game_session());
}