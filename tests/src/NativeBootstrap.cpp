#include "openvic-simulation/GameManager.hpp"

#include <cstdint>

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