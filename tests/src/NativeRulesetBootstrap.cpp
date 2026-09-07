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