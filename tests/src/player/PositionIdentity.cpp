#include "openvic-simulation/player/PlayerManager.hpp"
#include "openvic-simulation/player/PositionIdentity.hpp"

#include <cstdint>
#include <string>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	PositionOccupancy make_pm_occupancy(PositionControllerKind kind, std::string controller_id) {
		return PositionOccupancy {
			.position = PositionIdentity {
				.position_id = "position:prime_minister:GBR",
				.jurisdiction_id = "country:GBR"
			},
			.controller_id = std::move(controller_id),
			.controller_kind = kind
		};
	}
}

TEST_CASE("Position identity requires stable position and jurisdiction", "[vertical][nation][position]") {
	CHECK_FALSE(PositionIdentity {}.is_canonical());
	CHECK_FALSE(PositionIdentity {
		.position_id = "position:prime_minister:GBR",
		.jurisdiction_id = ""
	}.is_canonical());

	PositionIdentity const identity {
		.position_id = "position:prime_minister:GBR",
		.jurisdiction_id = "country:GBR"
	};
	CHECK(identity.is_canonical());
	CHECK(identity.authority_actor_id() == "position:prime_minister:GBR");
}

TEST_CASE("Human and AI occupants preserve identical authority identity", "[vertical][nation][position][symmetry]") {
	PositionOccupancy const human = make_pm_occupancy(
		PositionControllerKind::human,
		"controller:human:local-player"
	);
	PositionOccupancy const ai = make_pm_occupancy(
		PositionControllerKind::ai,
		"controller:ai:strategic-gbr"
	);

	REQUIRE(human.is_canonical());
	REQUIRE(ai.is_canonical());

	CHECK(human.authority_actor_id() == ai.authority_actor_id());
	CHECK(human.jurisdiction_id() == ai.jurisdiction_id());
	CHECK(human.controller_id != ai.controller_id);
	CHECK(human.controller_kind != ai.controller_kind);
}

TEST_CASE("Occupant identity cannot substitute for authority-bearing position", "[vertical][nation][position][authority]") {
	PositionOccupancy const occupant = make_pm_occupancy(
		PositionControllerKind::human,
		"controller:human:local-player"
	);

	REQUIRE(occupant.is_canonical());
	CHECK(occupant.authority_actor_id() == "position:prime_minister:GBR");
	CHECK(occupant.authority_actor_id() != occupant.controller_id);
}

TEST_CASE("PlayerManager preserves legacy country seam while gaining position occupancy", "[vertical][nation][player][compatibility]") {
	PlayerManager player;

	CHECK_FALSE(player.has_position_occupancy());
	CHECK_FALSE(player.get_authority_actor_id().has_value());
	CHECK_FALSE(player.get_jurisdiction_id().has_value());

	REQUIRE(player.set_position_occupancy(make_pm_occupancy(
		PositionControllerKind::human,
		"controller:human:local-player"
	)));

	CHECK(player.has_position_occupancy());
	REQUIRE(player.get_authority_actor_id().has_value());
	REQUIRE(player.get_jurisdiction_id().has_value());
	CHECK(*player.get_authority_actor_id() == "position:prime_minister:GBR");
	CHECK(*player.get_jurisdiction_id() == "country:GBR");

	// The old country pointer API remains legal during migration.
	player.set_country(nullptr);
	CHECK(player.get_country() == nullptr);

	player.clear_position_occupancy();
	CHECK_FALSE(player.has_position_occupancy());
	CHECK(player.get_country() == nullptr);
}

TEST_CASE("PlayerManager rejects malformed occupancy transactionally", "[vertical][nation][position][validation]") {
	PlayerManager player;
	PositionOccupancy const valid = make_pm_occupancy(
		PositionControllerKind::human,
		"controller:human:local-player"
	);
	REQUIRE(player.set_position_occupancy(valid));

	PositionOccupancy invalid = valid;
	invalid.position.jurisdiction_id.clear();

	CHECK_FALSE(player.set_position_occupancy(std::move(invalid)));
	REQUIRE(player.get_position_occupancy() != nullptr);
	CHECK(*player.get_position_occupancy() == valid);
}