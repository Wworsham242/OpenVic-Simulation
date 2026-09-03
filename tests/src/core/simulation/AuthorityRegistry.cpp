#include "openvic-simulation/core/simulation/AuthorityRegistry.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	AuthorityRegistry make_registry() {
		AuthorityRegistry registry;

		REQUIRE(registry.register_profile(ActorAuthorityProfile {
			.actor_id = "office:executive:USA",
			.grants = {
				AuthorityGrant { .command_type = "force.set_readiness", .jurisdiction_id = "country:USA" },
				AuthorityGrant { .command_type = "policy.set_priority", .jurisdiction_id = "country:USA" }
			}
		}));
		REQUIRE(registry.register_profile(ActorAuthorityProfile {
			.actor_id = "institution:central_bank:USA",
			.grants = {
				AuthorityGrant { .command_type = "monetary.set_target", .jurisdiction_id = "country:USA" }
			}
		}));
		REQUIRE(registry.register_profile(ActorAuthorityProfile {
			.actor_id = "command:theater:EUCOM",
			.grants = {
				AuthorityGrant { .command_type = "force.set_readiness", .jurisdiction_id = "theater:EUCOM" }
			}
		}));

		return registry;
	}
}

TEST_CASE("Authority profiles require canonical grants", "[foundation][authority][validation]") {
	AuthorityRegistry registry;

	CHECK_FALSE(registry.register_profile(ActorAuthorityProfile {}));

	CHECK_FALSE(registry.register_profile(ActorAuthorityProfile {
		.actor_id = "actor:a",
		.grants = {
			AuthorityGrant { .command_type = "z", .jurisdiction_id = "j" },
			AuthorityGrant { .command_type = "a", .jurisdiction_id = "j" }
		}
	}));

	REQUIRE(registry.register_profile(ActorAuthorityProfile {
		.actor_id = "actor:a",
		.grants = {
			AuthorityGrant { .command_type = "a", .jurisdiction_id = "j" }
		}
	}));
	CHECK_FALSE(registry.register_profile(ActorAuthorityProfile {
		.actor_id = "actor:a",
		.grants = {
			AuthorityGrant { .command_type = "a", .jurisdiction_id = "j" }
		}
	}));
}

TEST_CASE("Authority is actor command and jurisdiction specific", "[foundation][authority][jurisdiction]") {
	AuthorityRegistry registry = make_registry();

	CHECK(registry.authorizes(
		"office:executive:USA", "policy.set_priority", "country:USA"
	));
	CHECK_FALSE(registry.authorizes(
		"office:executive:USA", "monetary.set_target", "country:USA"
	));
	CHECK_FALSE(registry.authorizes(
		"office:executive:USA", "policy.set_priority", "country:FRA"
	));
	CHECK(registry.authorizes(
		"command:theater:EUCOM", "force.set_readiness", "theater:EUCOM"
	));
	CHECK_FALSE(registry.authorizes(
		"command:theater:EUCOM", "force.set_readiness", "country:USA"
	));
}

TEST_CASE("Only authorized commands enter accepted command log", "[foundation][authority][command]") {
	AuthorityRegistry registry = make_registry();
	OrderedCommandRuntime commands;
	CommandAdmissionRuntime admission { registry, commands };

	CHECK(admission.submit(
		SimTime::from_ticks(24),
		"office:executive:USA",
		"policy.set_priority",
		"country:USA",
		{ 1 }
	) == CommandAdmissionResult::accepted);

	CHECK(admission.submit(
		SimTime::from_ticks(24),
		"office:executive:USA",
		"monetary.set_target",
		"country:USA",
		{ 2 }
	) == CommandAdmissionResult::unauthorized);

	CHECK(admission.submit(
		SimTime::from_ticks(24),
		"unknown:actor",
		"policy.set_priority",
		"country:USA",
		{ 3 }
	) == CommandAdmissionResult::unknown_actor);

	CHECK(commands.accepted_command_count() == 1);

	const auto log = commands.capture_command_log();
	REQUIRE(log.size() == 1);
	CHECK(log[0].actor_id == "office:executive:USA");
	CHECK(log[0].command_type == "policy.set_priority");
	CHECK(log[0].jurisdiction_id == "country:USA");
}

TEST_CASE("Human and AI controllers can share identical office authority", "[foundation][authority][symmetry]") {
	AuthorityRegistry registry;

	REQUIRE(registry.register_profile(ActorAuthorityProfile {
		.actor_id = "human:office:PM:GBR",
		.grants = {
			AuthorityGrant { .command_type = "policy.propose", .jurisdiction_id = "country:GBR" }
		}
	}));
	REQUIRE(registry.register_profile(ActorAuthorityProfile {
		.actor_id = "ai:office:PM:GBR",
		.grants = {
			AuthorityGrant { .command_type = "policy.propose", .jurisdiction_id = "country:GBR" }
		}
	}));

	CHECK(registry.authorizes("human:office:PM:GBR", "policy.propose", "country:GBR"));
	CHECK(registry.authorizes("ai:office:PM:GBR", "policy.propose", "country:GBR"));
}

TEST_CASE("Invalid admission never mutates command runtime", "[foundation][authority][validation]") {
	AuthorityRegistry registry = make_registry();
	OrderedCommandRuntime commands;
	CommandAdmissionRuntime admission { registry, commands };

	CHECK(admission.submit(
		SimTime::from_ticks(0), "", "policy.set_priority", "country:USA", {}
	) == CommandAdmissionResult::invalid_request);
	CHECK(admission.submit(
		SimTime::from_ticks(0), "office:executive:USA", "", "country:USA", {}
	) == CommandAdmissionResult::invalid_request);
	CHECK(admission.submit(
		SimTime::from_ticks(0), "office:executive:USA", "policy.set_priority", "", {}
	) == CommandAdmissionResult::invalid_request);

	CHECK(commands.accepted_command_count() == 0);
}