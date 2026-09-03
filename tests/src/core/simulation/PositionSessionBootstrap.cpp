#include "openvic-simulation/core/simulation/PositionSessionBootstrap.hpp"

#include <string>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	PositionOccupancy make_occupancy(PositionControllerKind kind, std::string controller_id) {
		return PositionOccupancy {
			.position = PositionIdentity {
				.position_id = "position:prime_minister:GBR",
				.jurisdiction_id = "country:GBR"
			},
			.controller_id = std::move(controller_id),
			.controller_kind = kind
		};
	}

	std::vector<AuthorityGrant> make_grants() {
		return {
			AuthorityGrant {
				.command_type = "policy.set_priority",
				.jurisdiction_id = "country:GBR"
			},
			AuthorityGrant {
				.command_type = "military.set_mobilised",
				.jurisdiction_id = "country:GBR"
			}
		};
	}
}

TEST_CASE("Session bootstrap binds occupancy to authority-bearing position", "[vertical][nation][session][authority]") {
	PlayerManager player;
	AuthorityRegistry authority;

	REQUIRE(PositionSessionBootstrap::configure(
		player,
		authority,
		make_occupancy(PositionControllerKind::human, "controller:human:local-player"),
		make_grants()
	));

	REQUIRE(player.get_position_occupancy() != nullptr);
	CHECK(player.get_position_occupancy()->authority_actor_id() == "position:prime_minister:GBR");
	CHECK(authority.contains("position:prime_minister:GBR"));
	CHECK(authority.authorizes(
		"position:prime_minister:GBR",
		"military.set_mobilised",
		"country:GBR"
	));
	CHECK_FALSE(authority.contains("controller:human:local-player"));
}

TEST_CASE("Human and AI session bootstraps produce the same authority profile", "[vertical][nation][session][symmetry]") {
	PlayerManager human_player;
	PlayerManager ai_player;
	AuthorityRegistry human_authority;
	AuthorityRegistry ai_authority;

	REQUIRE(PositionSessionBootstrap::configure(
		human_player,
		human_authority,
		make_occupancy(PositionControllerKind::human, "controller:human:local-player"),
		make_grants()
	));
	REQUIRE(PositionSessionBootstrap::configure(
		ai_player,
		ai_authority,
		make_occupancy(PositionControllerKind::ai, "controller:ai:strategic-gbr"),
		make_grants()
	));

	CHECK(human_player.get_authority_actor_id() == ai_player.get_authority_actor_id());
	CHECK(human_player.get_jurisdiction_id() == ai_player.get_jurisdiction_id());
	CHECK(human_authority.authorizes(
		"position:prime_minister:GBR",
		"military.set_mobilised",
		"country:GBR"
	));
	CHECK(ai_authority.authorizes(
		"position:prime_minister:GBR",
		"military.set_mobilised",
		"country:GBR"
	));
}

TEST_CASE("Session bootstrap rejects duplicate grants before mutation", "[vertical][nation][session][validation]") {
	PlayerManager player;
	AuthorityRegistry authority;

	auto grants = make_grants();
	grants.push_back(AuthorityGrant {
		.command_type = "military.set_mobilised",
		.jurisdiction_id = "country:GBR"
	});

	CHECK_FALSE(PositionSessionBootstrap::configure(
		player,
		authority,
		make_occupancy(PositionControllerKind::human, "controller:human:local-player"),
		std::move(grants)
	));

	CHECK_FALSE(player.has_position_occupancy());
	CHECK_FALSE(authority.contains("position:prime_minister:GBR"));
}

TEST_CASE("Session bootstrap does not replace an existing occupancy", "[vertical][nation][session][lifecycle]") {
	PlayerManager player;
	AuthorityRegistry authority;

	REQUIRE(PositionSessionBootstrap::configure(
		player,
		authority,
		make_occupancy(PositionControllerKind::human, "controller:human:first"),
		make_grants()
	));

	AuthorityRegistry second_authority;
	CHECK_FALSE(PositionSessionBootstrap::configure(
		player,
		second_authority,
		make_occupancy(PositionControllerKind::ai, "controller:ai:replacement"),
		make_grants()
	));

	REQUIRE(player.get_position_occupancy() != nullptr);
	CHECK(player.get_position_occupancy()->controller_id == "controller:human:first");
	CHECK_FALSE(second_authority.contains("position:prime_minister:GBR"));
}