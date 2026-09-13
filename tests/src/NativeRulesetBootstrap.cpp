#include <utility>
#include <string>
#include "openvic-simulation/GameManager.hpp"

#include "openvic-simulation/core/simulation/SettingCapabilityManifest.hpp"
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
TEST_CASE(
	"Alternative route absorbs primary-route capacity shortfall",
	"[convergence][native-ruleset][alternative-routing]"
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

	TransportCorridor mine_a_primary {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	mine_a_primary.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(1),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	TransportCorridor mine_b_primary {
		market_node_index_t { 12 },
		market_node_index_t { 22 }
	};
	mine_b_primary.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	REQUIRE(instance->configure_live_resource_source_routes(
		std::vector<ResourceSourceRoute> {
			ResourceSourceRoute { "mine_a", std::move(mine_a_primary) },
			ResourceSourceRoute { "mine_b", std::move(mine_b_primary) }
		}
	));

	TransportCorridor mine_a_alternate {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	mine_a_alternate.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(1),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	REQUIRE(instance->configure_live_resource_alternative_routes(
		std::vector<ResourceAlternativeRoute> {
			ResourceAlternativeRoute {
				"mine_a",
				"mine_a_alt",
				std::move(mine_a_alternate)
			}
		}
	));

	REQUIRE(manager.start_game_session());

	instance->force_tick_and_update();
	LiveEconomyStatus with_alternate = instance->get_live_economy_status();

	CHECK(with_alternate.source_nominal_inflow == fixed_point_t(4));
	CHECK(with_alternate.source_unmet_inflow == fixed_point_t::_0);
	CHECK(with_alternate.upstream_output == fixed_point_t(4));

	REQUIRE(instance->set_live_resource_alternative_route_access(
		"mine_a_alt",
		false
	));

	instance->force_tick_and_update();
	LiveEconomyStatus without_alternate = instance->get_live_economy_status();

	CHECK(without_alternate.source_nominal_inflow == fixed_point_t(4));
	CHECK(without_alternate.source_unmet_inflow == fixed_point_t(1));
	CHECK(without_alternate.upstream_output == fixed_point_t(3));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Deterministic logistics graph reroutes intact resource supply",
	"[convergence][native-ruleset][logistics-graph]"
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

	// Explicit source routes remain as compatibility/fallback definitions.
	TransportCorridor fallback_a {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	fallback_a.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2)
	});

	TransportCorridor fallback_b {
		market_node_index_t { 12 },
		market_node_index_t { 22 }
	};
	fallback_b.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2)
	});

	REQUIRE(instance->configure_live_resource_source_routes(
		std::vector<ResourceSourceRoute> {
			ResourceSourceRoute { "mine_a", std::move(fallback_a) },
			ResourceSourceRoute { "mine_b", std::move(fallback_b) }
		}
	));

	REQUIRE(instance->configure_live_logistics_graph({
		LogisticsGraphEdge {
			.edge_id = "a_mine_a_primary_1",
			.source = market_node_index_t { 11 },
			.destination = market_node_index_t { 31 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "a_mine_a_primary_2",
			.source = market_node_index_t { 31 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "b_mine_a_alternate_1",
			.source = market_node_index_t { 11 },
			.destination = market_node_index_t { 41 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "b_mine_a_alternate_2",
			.source = market_node_index_t { 41 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "mine_b_direct",
			.source = market_node_index_t { 12 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		}
	}));

	REQUIRE(instance->configure_live_resource_graph_routes({
		ResourceGraphRoute {
			.source_id = "mine_a",
			.source_node = market_node_index_t { 11 },
			.destination_node = market_node_index_t { 22 }
		},
		ResourceGraphRoute {
			.source_id = "mine_b",
			.source_node = market_node_index_t { 12 },
			.destination_node = market_node_index_t { 22 }
		}
	}));

	REQUIRE(manager.start_game_session());

	instance->force_tick_and_update();
	LiveEconomyStatus primary = instance->get_live_economy_status();

	CHECK(primary.source_nominal_inflow == fixed_point_t(4));
	CHECK(primary.source_unmet_inflow == fixed_point_t::_0);
	CHECK(primary.upstream_output == fixed_point_t(4));

	REQUIRE(instance->set_live_logistics_graph_edge_open(
		"a_mine_a_primary_2",
		false
	));

	instance->force_tick_and_update();
	LiveEconomyStatus rerouted = instance->get_live_economy_status();

	CHECK(rerouted.source_nominal_inflow == fixed_point_t(4));
	CHECK(rerouted.source_unmet_inflow == fixed_point_t(1));
	CHECK(rerouted.upstream_output == fixed_point_t(3));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Graph-routed resource flows compete for overlapping edge capacity",
	"[convergence][native-ruleset][shared-graph-edge]"
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

	TransportCorridor fallback_a {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	fallback_a.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2)
	});

	TransportCorridor fallback_b {
		market_node_index_t { 12 },
		market_node_index_t { 22 }
	};
	fallback_b.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2)
	});

	REQUIRE(instance->configure_live_resource_source_routes(
		std::vector<ResourceSourceRoute> {
			ResourceSourceRoute { "mine_a", std::move(fallback_a) },
			ResourceSourceRoute { "mine_b", std::move(fallback_b) }
		}
	));

	REQUIRE(instance->configure_live_logistics_graph({
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

	REQUIRE(instance->configure_live_resource_graph_routes({
		ResourceGraphRoute {
			.source_id = "mine_a",
			.source_node = market_node_index_t { 11 },
			.destination_node = market_node_index_t { 22 }
		},
		ResourceGraphRoute {
			.source_id = "mine_b",
			.source_node = market_node_index_t { 12 },
			.destination_node = market_node_index_t { 22 }
		}
	}));

	REQUIRE(manager.start_game_session());

	instance->force_tick_and_update();
	LiveEconomyStatus status = instance->get_live_economy_status();

	CHECK(status.source_nominal_inflow == fixed_point_t(4));
	CHECK(status.source_unmet_inflow == fixed_point_t(1));
	CHECK(status.upstream_output == fixed_point_t(3));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Residual resource flow reroutes after primary graph allocation",
	"[convergence][native-ruleset][allocation-rerouting]"
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

	TransportCorridor fallback_a {
		market_node_index_t { 11 },
		market_node_index_t { 22 }
	};
	fallback_a.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2)
	});

	TransportCorridor fallback_b {
		market_node_index_t { 12 },
		market_node_index_t { 22 }
	};
	fallback_b.add_leg(TransportLeg {
		.nominal_capacity = fixed_point_t(2)
	});

	REQUIRE(instance->configure_live_resource_source_routes(
		std::vector<ResourceSourceRoute> {
			ResourceSourceRoute { "mine_a", std::move(fallback_a) },
			ResourceSourceRoute { "mine_b", std::move(fallback_b) }
		}
	));

	REQUIRE(instance->configure_live_logistics_graph({
		LogisticsGraphEdge {
			.edge_id = "a_mine_a_primary_1",
			.source = market_node_index_t { 11 },
			.destination = market_node_index_t { 31 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "a_mine_a_primary_2",
			.source = market_node_index_t { 31 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "b_mine_a_alternate_1",
			.source = market_node_index_t { 11 },
			.destination = market_node_index_t { 41 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "b_mine_a_alternate_2",
			.source = market_node_index_t { 41 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(1)
			}
		},
		LogisticsGraphEdge {
			.edge_id = "mine_b_direct",
			.source = market_node_index_t { 12 },
			.destination = market_node_index_t { 22 },
			.leg = TransportLeg {
				.nominal_capacity = fixed_point_t(2)
			}
		}
	}));

	REQUIRE(instance->configure_live_resource_graph_routes({
		ResourceGraphRoute {
			.source_id = "mine_a",
			.source_node = market_node_index_t { 11 },
			.destination_node = market_node_index_t { 22 }
		},
		ResourceGraphRoute {
			.source_id = "mine_b",
			.source_node = market_node_index_t { 12 },
			.destination_node = market_node_index_t { 22 }
		}
	}));

	REQUIRE(manager.start_game_session());

	instance->force_tick_and_update();
	LiveEconomyStatus rerouted = instance->get_live_economy_status();

	CHECK(rerouted.source_nominal_inflow == fixed_point_t(4));
	CHECK(rerouted.source_unmet_inflow == fixed_point_t::_0);
	CHECK(rerouted.upstream_output == fixed_point_t(4));

	REQUIRE(instance->set_live_logistics_graph_edge_open(
		"b_mine_a_alternate_2",
		false
	));

	instance->force_tick_and_update();
	LiveEconomyStatus constrained = instance->get_live_economy_status();

	CHECK(constrained.source_nominal_inflow == fixed_point_t(4));
	CHECK(constrained.source_unmet_inflow == fixed_point_t(1));
	CHECK(constrained.upstream_output == fixed_point_t(3));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Native catalog uses setting-general OpenVic production processes",
	"[convergence][native-ruleset][production-process]"
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

	EconomyManager const& economy =
		manager.get_definition_manager().get_economy_manager();

	ProductionType const* const steel_process =
		economy.get_production_type_manager()
			.get_production_type_by_identifier("native_ore_to_steel");

	REQUIRE(steel_process != nullptr);
	CHECK(
		steel_process->template_type
		== ProductionType::template_type_t::PROCESS
	);
	CHECK(steel_process->is_setting_general_process());
	CHECK(steel_process->input_goods.size() == 1);
	CHECK(steel_process->maintenance_requirements.size() == 1);
	CHECK(
		steel_process->output_good.get_identifier()
		== "native_primary_steel"
	);
	CHECK(steel_process->base_output_quantity == fixed_point_t(1));
}
TEST_CASE(
	"Native facility catalog reuses OpenVic BuildingType capacity machinery",
	"[convergence][native-ruleset][facility-capacity]"
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

	EconomyManager const& economy =
		manager.get_definition_manager().get_economy_manager();

	BuildingType const* const facility =
		economy.get_building_type_manager()
			.get_building_type_by_identifier("native_primary_steel_capacity");

	REQUIRE(facility != nullptr);
	CHECK(facility->is_setting_general_capacity_asset());
	CHECK(facility->capacity_per_level == fixed_point_t(2));
	CHECK(facility->max_level == building_level_t(5));
	REQUIRE(facility->production_type != nullptr);
	CHECK(facility->production_type->is_setting_general_process());
	CHECK(
		facility->production_type->get_identifier()
		== "native_ore_to_steel"
	);
	CHECK(facility->goods_cost.size() == 2);
	CHECK(
		facility->calculate_installed_capacity(building_level_t(3))
		== fixed_point_t(6)
	);

	BuildingInstance instance { *facility, building_level_t(2) };
	CHECK(instance.get_level() == building_level_t(2));
	CHECK(
		facility->calculate_installed_capacity(instance.get_level())
		== fixed_point_t(4)
	);
}
TEST_CASE(
	"Installed facility level causally constrains live production",
	"[convergence][native-ruleset][facility-production-capacity]"
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

	EconomyManager const& economy =
		manager.get_definition_manager().get_economy_manager();

	BuildingType const* const facility =
		economy.get_building_type_manager()
			.get_building_type_by_identifier("native_primary_steel_capacity");

	REQUIRE(facility != nullptr);

	// Level 1 installs only two units of capacity.
	REQUIRE(
		instance->set_live_upstream_capacity_from_facility(
			*facility,
			building_level_t(1)
		)
	);

	REQUIRE(manager.start_game_session());
	instance->force_tick_and_update();

	LiveEconomyStatus constrained = instance->get_live_economy_status();
	CHECK(constrained.upstream_output == fixed_point_t(2));

	// Expanding to level 2 installs four units, restoring the live scenario's
	// full upstream throughput on the following production tick.
	REQUIRE(
		instance->set_live_upstream_capacity_from_facility(
			*facility,
			building_level_t(2)
		)
	);

	instance->force_tick_and_update();

	LiveEconomyStatus expanded = instance->get_live_economy_status();
	CHECK(expanded.upstream_output == fixed_point_t(4));

	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Available workforce causally constrains live production",
	"[convergence][native-ruleset][workforce-production-capacity]"
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

	EconomyManager const& economy =
		manager.get_definition_manager().get_economy_manager();

	BuildingType const* const facility =
		economy.get_building_type_manager()
			.get_building_type_by_identifier("native_primary_steel_capacity");

	REQUIRE(facility != nullptr);

	REQUIRE(
		instance->set_live_upstream_capacity_from_facility(
			*facility,
			building_level_t(2)
		)
	);

	// The process requires ten workers per unit of capacity.
	// Twenty workers can therefore support only two capacity units even
	// though the installed facility can support four.
	REQUIRE(instance->set_live_upstream_available_workforce(fixed_point_t(20)));

	REQUIRE(manager.start_game_session());
	instance->force_tick_and_update();

	LiveEconomyStatus labor_limited = instance->get_live_economy_status();
	CHECK(labor_limited.upstream_output == fixed_point_t(2));

	// Forty workers support all four installed units.
	REQUIRE(instance->set_live_upstream_available_workforce(fixed_point_t(40)));
	instance->force_tick_and_update();

	LiveEconomyStatus fully_staffed = instance->get_live_economy_status();
	CHECK(fully_staffed.upstream_output == fixed_point_t(4));

	REQUIRE(manager.end_game_session());
}
TEST_CASE("Legacy days dispatch exactly one cadenced live economy cycle",
	"[convergence][native-ruleset][cadence-authority]") {
	GameManager manager { []() {}, []() -> uint64_t { return 0; }, []() -> uint64_t { return 0; } };
	auto const data_root = std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";
	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());
	auto* instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	REQUIRE(manager.start_game_session());
	CHECK(instance->get_live_economy_status().completed_daily_ticks == 0);
	for (uint64_t day = 1; day <= 3; ++day) {
		instance->force_tick_and_update();
		CHECK(instance->get_simulation_time() == SimTime { static_cast<int64_t>(24 * day) });
		auto const status = instance->get_live_economy_status();
		CHECK(status.completed_daily_ticks == day);
		CHECK(status.upstream_output == fixed_point_t { 4 });
		// A second clearing would overwrite the market totals with zero;
		// clearing before pre-market work would leave these orders unfilled.
		CHECK(status.intermediate_supply_yesterday == fixed_point_t { 4 });
		CHECK(status.intermediate_quantity_traded_yesterday == fixed_point_t { 4 });
		CHECK(status.downstream_output == fixed_point_t { 2 });
		CHECK(status.final_inventory == fixed_point_t { static_cast<int32_t>(2 * day) });
	}
	REQUIRE(manager.end_game_session());
}
TEST_CASE(
	"Native manifest without nutrition-health disables production PopDeps capability",
	"[convergence][setting-composition][population-capability]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	SettingCapabilityManifest manifest {
		.package_id = "reference.bronze-age",
		.capabilities = {
			"governance.basic",
			"logistics.land",
			"population.basic",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);

	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	CHECK_FALSE(
		instance->is_population_nutrition_health_capability_enabled()
	);
}

TEST_CASE(
	"Native manifest with nutrition-health enables production PopDeps capability",
	"[convergence][setting-composition][population-capability]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	SettingCapabilityManifest manifest {
		.package_id = "reference.modern",
		.capabilities = {
			"governance.basic",
			"logistics.land",
			"population.basic",
			"population.nutrition-health",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);

	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	CHECK(
		instance->is_population_nutrition_health_capability_enabled()
	);
}

TEST_CASE(
	"Native setup without capability manifest preserves compatibility behavior",
	"[convergence][setting-composition][compatibility]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	CHECK(
		instance->is_population_nutrition_health_capability_enabled()
	);
}

TEST_CASE(
	"Native setup rejects noncanonical setting capability manifest",
	"[convergence][setting-composition][validation]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = SettingCapabilityManifest {
		.package_id = "invalid.unsorted",
		.capabilities = {
			"production.basic",
			"population.basic"
		}
	};

	CHECK_FALSE(manager.setup_native_instance(std::move(bootstrap)));
	CHECK(manager.get_instance_manager() == nullptr);
}
TEST_CASE(
	"Setting manifest can omit aggregate production chain while preserving base economy",
	"[convergence][setting-composition][economy-capability]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	SettingCapabilityManifest manifest {
		.package_id = "reference.minimal-production",
		.capabilities = {
			"governance.basic",
			"logistics.land",
			"population.basic",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);
	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	CHECK_FALSE(instance->is_live_aggregate_production_chain_enabled());
	CHECK_FALSE(instance->get_live_economy_status().configured);
	CHECK(instance->get_simulation_time() == SimTime { 0 });
}

TEST_CASE(
	"Setting manifest can enable aggregate production chain independently",
	"[convergence][setting-composition][economy-capability]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	SettingCapabilityManifest manifest {
		.package_id = "reference.aggregate-production",
		.capabilities = {
			"economy.aggregate-production-chain",
			"governance.basic",
			"logistics.land",
			"population.basic",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);
	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	CHECK(instance->is_live_aggregate_production_chain_enabled());

	LiveEconomyStatus const status = instance->get_live_economy_status();
	CHECK(status.configured);
	CHECK(status.completed_daily_ticks == 0);
}

TEST_CASE(
	"Nutrition capability can be enabled while aggregate economy is omitted",
	"[convergence][setting-composition][cross-domain]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	SettingCapabilityManifest manifest {
		.package_id = "reference.population-rich-economy-minimal",
		.capabilities = {
			"governance.basic",
			"logistics.land",
			"population.basic",
			"population.nutrition-health",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);
	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK(instance->is_population_nutrition_health_capability_enabled());
	CHECK_FALSE(instance->is_live_aggregate_production_chain_enabled());
	CHECK_FALSE(instance->get_live_economy_status().configured);
}

TEST_CASE(
	"Aggregate economy capability can be enabled while nutrition history is omitted",
	"[convergence][setting-composition][cross-domain]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	SettingCapabilityManifest manifest {
		.package_id = "reference.economy-rich-population-minimal",
		.capabilities = {
			"economy.aggregate-production-chain",
			"governance.basic",
			"logistics.land",
			"population.basic",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);
	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK_FALSE(instance->is_population_nutrition_health_capability_enabled());
	CHECK(instance->is_live_aggregate_production_chain_enabled());
	CHECK(instance->get_live_economy_status().configured);
}

TEST_CASE(
	"No manifest preserves aggregate economy compatibility behavior",
	"[convergence][setting-composition][economy-compatibility]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	CHECK(instance->is_live_aggregate_production_chain_enabled());
	CHECK(instance->get_live_economy_status().configured);
}
TEST_CASE(
	"Reference setting packages compose distinct production runtimes through one engine path",
	"[convergence][setting-composition][closure]"
) {
	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	auto make_bronze_reference = []() {
		SettingCapabilityManifest manifest {
			.package_id = "reference.bronze-age",
			.capabilities = {
				"governance.basic",
				"logistics.land",
				"population.basic",
				"production.basic",
				"trade.physical"
			}
		};
		REQUIRE(manifest.canonicalize());
		return manifest;
	};

	auto make_modern_reference = []() {
		SettingCapabilityManifest manifest {
			.package_id = "reference.modern",
			.capabilities = {
				"economy.aggregate-production-chain",
				"finance.banking",
				"finance.credit",
				"governance.basic",
				"information.cyber",
				"infrastructure.electric-grid",
				"logistics.land",
				"military.air",
				"population.basic",
				"population.nutrition-health",
				"production.basic",
				"trade.physical"
			}
		};
		REQUIRE(manifest.canonicalize());
		return manifest;
	};

	SettingCapabilityManifest bronze_manifest = make_bronze_reference();
	SettingCapabilityManifest modern_manifest = make_modern_reference();

	REQUIRE(bronze_manifest.is_canonical());
	REQUIRE(modern_manifest.is_canonical());

	CHECK(bronze_manifest.package_id == "reference.bronze-age");
	CHECK(modern_manifest.package_id == "reference.modern");
	CHECK(bronze_manifest.checksum() != modern_manifest.checksum());

	/* Shared baseline capabilities: same universal engine path, not era branches. */
	CHECK(bronze_manifest.has("population.basic"));
	CHECK(modern_manifest.has("population.basic"));
	CHECK(bronze_manifest.has("production.basic"));
	CHECK(modern_manifest.has("production.basic"));
	CHECK(bronze_manifest.has("trade.physical"));
	CHECK(modern_manifest.has("trade.physical"));
	CHECK(bronze_manifest.has("logistics.land"));
	CHECK(modern_manifest.has("logistics.land"));
	CHECK(bronze_manifest.has("governance.basic"));
	CHECK(modern_manifest.has("governance.basic"));

	/* Modern reference contains capabilities not yet runtime-bound by 006A3. */
	CHECK(modern_manifest.has("finance.banking"));
	CHECK(modern_manifest.has("finance.credit"));
	CHECK(modern_manifest.has("information.cyber"));
	CHECK(modern_manifest.has("infrastructure.electric-grid"));
	CHECK(modern_manifest.has("military.air"));

	GameManager bronze_manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};
	REQUIRE(bronze_manager.load_native_economy_bootstrap(data_root));

	NativeInstanceBootstrap bronze_bootstrap;
	bronze_bootstrap.setting_capabilities = bronze_manifest;
	REQUIRE(bronze_manager.setup_native_instance(std::move(bronze_bootstrap)));

	InstanceManager* const bronze_instance =
		bronze_manager.get_instance_manager();
	REQUIRE(bronze_instance != nullptr);

	CHECK_FALSE(
		bronze_instance->is_population_nutrition_health_capability_enabled()
	);
	CHECK_FALSE(
		bronze_instance->is_live_aggregate_production_chain_enabled()
	);
	CHECK_FALSE(bronze_instance->get_live_economy_status().configured);

	GameManager modern_manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};
	REQUIRE(modern_manager.load_native_economy_bootstrap(data_root));

	NativeInstanceBootstrap modern_bootstrap;
	modern_bootstrap.setting_capabilities = modern_manifest;
	REQUIRE(modern_manager.setup_native_instance(std::move(modern_bootstrap)));

	InstanceManager* const modern_instance =
		modern_manager.get_instance_manager();
	REQUIRE(modern_instance != nullptr);

	CHECK(
		modern_instance->is_population_nutrition_health_capability_enabled()
	);
	CHECK(
		modern_instance->is_live_aggregate_production_chain_enabled()
	);
	CHECK(modern_instance->get_live_economy_status().configured);

	/*
	 * 006A3 closure statement:
	 *
	 * - Both reference packages use the same GameManager -> InstanceManager
	 *   production setup path and the same engine binary.
	 * - Engine code does not branch on "Bronze" or "Modern" era values.
	 * - Runtime differences demonstrated here come only from bound capability
	 *   membership.
	 * - finance.*, information.cyber, infrastructure.electric-grid and
	 *   military.air remain declarative package entries only until their
	 *   owning domains implement explicit runtime bindings.
	 * - Their declaration here must not be interpreted as proof that those
	 *   mechanisms currently exist.
	 */
}

TEST_CASE(
	"Reference setting package labels are data and do not grant runtime capabilities",
	"[convergence][setting-composition][closure][no-era-special-case]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));

	/*
	 * Deliberately use the "modern" package label with only baseline
	 * capabilities. If engine era-name special casing existed, the optional
	 * mechanisms could spuriously activate. They must remain off.
	 */
	SettingCapabilityManifest manifest {
		.package_id = "reference.modern",
		.capabilities = {
			"governance.basic",
			"logistics.land",
			"population.basic",
			"production.basic",
			"trade.physical"
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);
	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK_FALSE(
		instance->is_population_nutrition_health_capability_enabled()
	);
	CHECK_FALSE(
		instance->is_live_aggregate_production_chain_enabled()
	);
	CHECK_FALSE(instance->get_live_economy_status().configured);
}
TEST_CASE(
	"Production instance delivers actor reports on simulation time",
	"[convergence][actor-perception][integration]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK(instance->get_simulation_time() == SimTime::from_ticks(0));

	ObservationReport report {
		.recipient_actor_id = "cabinet.analysis",
		.source_id = "provincial.office",
		.fact_type = "harvest.output",
		.subject_id = "province.17",
		.observed_at = SimTime::from_ticks(0),
		.deliver_at = SimTime::from_ticks(24),
		.payload = { 0x2A },
		.integrity = ObservationIntegrity::ESTIMATED,
		.confidence_basis_points = 7'500
	};

	REQUIRE(
		instance->submit_actor_observation_report(std::move(report)).has_value()
	);
	CHECK(instance->get_pending_actor_report_count() == 1);
	CHECK(
		instance->get_actor_knowledge(
			"cabinet.analysis",
			"harvest.output",
			"province.17"
		) == nullptr
	);

	REQUIRE(instance->start_game_session());
	instance->force_tick_and_update();

	CHECK(instance->get_simulation_time() == SimTime::from_ticks(24));
	CHECK(instance->get_pending_actor_report_count() == 0);

	ActorKnowledgeRecord const* const knowledge =
		instance->get_actor_knowledge(
			"cabinet.analysis",
			"harvest.output",
			"province.17"
		);

	REQUIRE(knowledge != nullptr);
	CHECK(knowledge->observed_at == SimTime::from_ticks(0));
	CHECK(knowledge->received_at == SimTime::from_ticks(24));
	CHECK(knowledge->payload == std::vector<uint8_t> { 0x2A });
	CHECK(knowledge->integrity == ObservationIntegrity::ESTIMATED);
}

TEST_CASE(
	"Production instance keeps future reports pending across ticks",
	"[convergence][actor-perception][integration]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	ObservationReport report {
		.recipient_actor_id = "palace",
		.source_id = "messenger.frontier",
		.fact_type = "border.condition",
		.subject_id = "frontier.east",
		.observed_at = SimTime::from_ticks(0),
		.deliver_at = SimTime::from_ticks(72),
		.payload = { 1 },
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 8'000
	};

	REQUIRE(
		instance->submit_actor_observation_report(std::move(report)).has_value()
	);

	REQUIRE(instance->start_game_session());

	instance->force_tick_and_update();
	CHECK(instance->get_simulation_time() == SimTime::from_ticks(24));
	CHECK(instance->get_pending_actor_report_count() == 1);
	CHECK(
		instance->get_actor_knowledge(
			"palace", "border.condition", "frontier.east"
		) == nullptr
	);

	instance->force_tick_and_update();
	CHECK(instance->get_simulation_time() == SimTime::from_ticks(48));
	CHECK(instance->get_pending_actor_report_count() == 1);

	instance->force_tick_and_update();
	CHECK(instance->get_simulation_time() == SimTime::from_ticks(72));
	CHECK(instance->get_pending_actor_report_count() == 0);
	REQUIRE(
		instance->get_actor_knowledge(
			"palace", "border.condition", "frontier.east"
		) != nullptr
	);
}
TEST_CASE(
	"Authoritative economy provenance produces delayed actor knowledge",
	"[convergence][actor-perception][economy-producer]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	REQUIRE(
		instance->configure_live_economy_observation_delivery(
			LiveEconomyObservationPolicy {
				.recipient_actor_id = "cabinet.economic-analysis",
				.delivery_delay_ticks = 24
			}
		)
	);

	REQUIRE(instance->start_game_session());

	instance->force_tick_and_update();
	CHECK(instance->get_simulation_time() == SimTime::from_ticks(24));

	auto const cycle_one = instance->get_live_economy_provenance();
	REQUIRE(cycle_one.has_value());
	REQUIRE(cycle_one->due_time.has_value());
	CHECK(*cycle_one->due_time == SimTime::from_ticks(24));

	bool const authoritative_transaction_limited =
		cycle_one->upstream_market.transaction_limited();
	std::string const authoritative_subject =
		cycle_one->intermediate_good_id;

	CHECK(
		instance->get_actor_knowledge(
			"cabinet.economic-analysis",
			"economy.market.transaction-limited",
			authoritative_subject
		) == nullptr
	);
	CHECK(instance->get_pending_actor_report_count() == 1);

	instance->force_tick_and_update();
	CHECK(instance->get_simulation_time() == SimTime::from_ticks(48));

	ActorKnowledgeRecord const* const perceived =
		instance->get_actor_knowledge(
			"cabinet.economic-analysis",
			"economy.market.transaction-limited",
			authoritative_subject
		);

	REQUIRE(perceived != nullptr);
	CHECK(perceived->observed_at == SimTime::from_ticks(24));
	CHECK(perceived->received_at == SimTime::from_ticks(48));
	REQUIRE(perceived->payload.size() == 1);
	CHECK(
		perceived->payload[0]
			== static_cast<uint8_t>(
				authoritative_transaction_limited ? 1 : 0
			)
	);
	CHECK(perceived->integrity == ObservationIntegrity::DIRECT);
	CHECK(perceived->confidence_basis_points == 10'000);

	CHECK(instance->get_pending_actor_report_count() == 1);

	CHECK(
		instance->get_actor_knowledge(
			"cabinet.foreign-affairs",
			"economy.market.transaction-limited",
			authoritative_subject
		) == nullptr
	);
}

TEST_CASE(
	"Economy observation delivery is opt-in and does not alter economy authority",
	"[convergence][actor-perception][economy-producer]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	auto const data_root =
		std::filesystem::path { __FILE__ }.parent_path().parent_path()
		/ "data" / "native-ruleset-bootstrap";

	REQUIRE(manager.load_native_economy_bootstrap(data_root));
	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);
	REQUIRE(instance->start_game_session());

	instance->force_tick_and_update();

	auto const provenance = instance->get_live_economy_provenance();
	REQUIRE(provenance.has_value());

	CHECK(instance->get_pending_actor_report_count() == 0);
	CHECK(instance->get_actor_knowledge_record_count() == 0);
	CHECK(instance->get_live_economy_status().completed_daily_ticks == 1);
}
