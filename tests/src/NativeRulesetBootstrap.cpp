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
