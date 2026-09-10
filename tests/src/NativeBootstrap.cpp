#include "openvic-simulation/GameManager.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"GameManager can initialise and start the authoritative runtime without a legacy bookmark",
	"[convergence][native-bootstrap]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	REQUIRE(manager.setup_native_instance());
	CHECK(manager.is_game_instance_setup());
	CHECK_FALSE(manager.is_bookmark_loaded());

	REQUIRE(manager.start_game_session());
	CHECK(manager.is_game_session_active());
	CHECK_FALSE(manager.is_bookmark_loaded());

	REQUIRE(manager.end_game_session());
	CHECK_FALSE(manager.is_game_instance_setup());
	CHECK_FALSE(manager.is_game_session_active());
}

TEST_CASE(
	"Native instance creation retains single authoritative ownership",
	"[convergence][native-bootstrap][ownership]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	REQUIRE(manager.setup_native_instance());
	CHECK_FALSE(manager.setup_native_instance());
	REQUIRE(manager.end_game_session());
}

TEST_CASE(
	"Native bootstrap leaves optional position capability absent when not requested",
	"[convergence][native-bootstrap][capability-composition]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	REQUIRE(manager.setup_native_instance());

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK(
		instance->submit_occupied_position_command(
			"native.policy.test",
			std::vector<uint8_t> {}
		) == CommandAdmissionResult::invalid_request
	);

	REQUIRE(manager.start_game_session());
	REQUIRE(manager.end_game_session());
}

TEST_CASE(
	"Native bootstrap composes optional position capability through authoritative owners",
	"[convergence][native-bootstrap][capability-composition][authority]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	NativeInstanceBootstrap bootstrap {
		.position = NativePositionBootstrap {
			.occupancy = PositionOccupancy {
				.position = PositionIdentity {
					.position_id = "native_national_executive",
					.jurisdiction_id = "native_state"
				},
				.controller_id = "test_human_controller",
				.controller_kind = PositionControllerKind::human
			},
			.grants = std::vector<AuthorityGrant> {
				AuthorityGrant {
					.command_type = "native.policy.test",
					.jurisdiction_id = "native_state"
				}
			}
		}
	};

	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK(manager.is_game_instance_setup());
	CHECK_FALSE(manager.is_bookmark_loaded());

	CHECK(
		instance->submit_occupied_position_command(
			"native.policy.test",
			std::vector<uint8_t> {}
		) == CommandAdmissionResult::accepted
	);

	CHECK(instance->get_accepted_command_count() == 1);

	REQUIRE(manager.start_game_session());
	CHECK(manager.is_game_session_active());

	REQUIRE(manager.end_game_session());
	CHECK_FALSE(manager.is_game_instance_setup());
}

TEST_CASE(
	"Invalid optional native capability aborts authoritative instance construction",
	"[convergence][native-bootstrap][capability-composition][atomicity]"
) {
	GameManager manager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};

	NativeInstanceBootstrap bootstrap {
		.position = NativePositionBootstrap {
			.occupancy = PositionOccupancy {
				.position = PositionIdentity {
					.position_id = "native_national_executive",
					.jurisdiction_id = "native_state"
				},
				.controller_id = "test_human_controller",
				.controller_kind = PositionControllerKind::human
			},
			.grants = std::vector<AuthorityGrant> {
				AuthorityGrant {
					.command_type = "native.policy.test",
					.jurisdiction_id = "native_state"
				},
				AuthorityGrant {
					.command_type = "native.policy.test",
					.jurisdiction_id = "native_state"
				}
			}
		}
	};

	CHECK_FALSE(manager.setup_native_instance(std::move(bootstrap)));

	CHECK(manager.get_instance_manager() == nullptr);
	CHECK_FALSE(manager.is_game_instance_setup());
	CHECK_FALSE(manager.is_game_session_active());
}
