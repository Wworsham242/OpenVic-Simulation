#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

#include "openvic-simulation/GameManager.hpp"
#include "openvic-simulation/core/simulation/SettingCapabilityManifest.hpp"

using namespace OpenVic;

namespace {

std::string cap(std::string_view value) {
	return std::string { value };
}

std::filesystem::path native_ruleset_data_root() {
	std::filesystem::path root { __FILE__ };
	for (int i = 0; i < 4; ++i) {
		root = root.parent_path();
	}
	return root / "data" / "native-ruleset-bootstrap";
}

SettingCapabilityManifest baseline_manifest(std::string package_id) {
	SettingCapabilityManifest manifest {
		.package_id = std::move(package_id),
		.capabilities = {
			cap(setting_capability::POPULATION_BASIC),
			cap(setting_capability::PRODUCTION_BASIC),
			cap(setting_capability::TRADE_PHYSICAL)
		}
	};
	if (!manifest.canonicalize()) {
		std::abort();
	}
	return manifest;
}

GameManager make_manager() {
	return GameManager {
		[]() {},
		[]() -> uint64_t { return 0; },
		[]() -> uint64_t { return 0; }
	};
}

}

TEST_CASE(
	"Capability contract canonicalization remains deterministic",
	"[convergence][setting-composition][006A3.2][determinism]"
) {
	SettingCapabilityManifest first {
		.package_id = "reference.contract-determinism",
		.capabilities = {
			cap(setting_capability::TRADE_PHYSICAL),
			cap(setting_capability::POPULATION_BASIC),
			cap(setting_capability::ECONOMY_AGGREGATE_PRODUCTION_CHAIN),
			cap(setting_capability::PRODUCTION_BASIC),
			cap(setting_capability::POPULATION_NUTRITION_HEALTH)
		}
	};

	SettingCapabilityManifest second {
		.package_id = "reference.contract-determinism",
		.capabilities = {
			cap(setting_capability::POPULATION_NUTRITION_HEALTH),
			cap(setting_capability::PRODUCTION_BASIC),
			cap(setting_capability::POPULATION_BASIC),
			cap(setting_capability::TRADE_PHYSICAL),
			cap(setting_capability::ECONOMY_AGGREGATE_PRODUCTION_CHAIN)
		}
	};

	REQUIRE(first.canonicalize());
	REQUIRE(second.canonicalize());
	REQUIRE(first.is_valid_contract());
	REQUIRE(second.is_valid_contract());

	CHECK(first.capabilities == second.capabilities);
	CHECK(first.checksum() == second.checksum());
}

TEST_CASE(
	"Nutrition health requires basic population capability",
	"[convergence][setting-composition][006A3.2][dependencies]"
) {
	SettingCapabilityManifest manifest {
		.package_id = "invalid.nutrition-without-population",
		.capabilities = {
			cap(setting_capability::POPULATION_NUTRITION_HEALTH)
		}
	};

	REQUIRE(manifest.canonicalize());
	CHECK_FALSE(manifest.dependencies_are_satisfied());
	CHECK_FALSE(manifest.is_valid_contract());

	auto const failure = manifest.first_dependency_failure();
	REQUIRE(failure.has_value());
	CHECK(failure->capability == setting_capability::POPULATION_NUTRITION_HEALTH);
	CHECK(failure->required_capability == setting_capability::POPULATION_BASIC);
}

TEST_CASE(
	"Aggregate production requires basic production capability",
	"[convergence][setting-composition][006A3.2][dependencies]"
) {
	SettingCapabilityManifest manifest {
		.package_id = "invalid.aggregate-without-production",
		.capabilities = {
			cap(setting_capability::ECONOMY_AGGREGATE_PRODUCTION_CHAIN),
			cap(setting_capability::TRADE_PHYSICAL)
		}
	};

	REQUIRE(manifest.canonicalize());
	CHECK_FALSE(manifest.is_valid_contract());

	auto const failure = manifest.first_dependency_failure();
	REQUIRE(failure.has_value());
	CHECK(failure->capability == setting_capability::ECONOMY_AGGREGATE_PRODUCTION_CHAIN);
	CHECK(failure->required_capability == setting_capability::PRODUCTION_BASIC);
}

TEST_CASE(
	"Aggregate production requires physical trade capability",
	"[convergence][setting-composition][006A3.2][dependencies]"
) {
	SettingCapabilityManifest manifest {
		.package_id = "invalid.aggregate-without-trade",
		.capabilities = {
			cap(setting_capability::ECONOMY_AGGREGATE_PRODUCTION_CHAIN),
			cap(setting_capability::PRODUCTION_BASIC)
		}
	};

	REQUIRE(manifest.canonicalize());
	CHECK_FALSE(manifest.is_valid_contract());

	auto const failure = manifest.first_dependency_failure();
	REQUIRE(failure.has_value());
	CHECK(failure->capability == setting_capability::ECONOMY_AGGREGATE_PRODUCTION_CHAIN);
	CHECK(failure->required_capability == setting_capability::TRADE_PHYSICAL);
}

TEST_CASE(
	"Future declarative capabilities do not invent dependency rules",
	"[convergence][setting-composition][006A3.2][future-capabilities]"
) {
	SettingCapabilityManifest manifest = baseline_manifest("reference.future-declarations");
	manifest.capabilities.push_back("finance.banking");
	manifest.capabilities.push_back("information.cyber");
	manifest.capabilities.push_back("infrastructure.electric-grid");
	manifest.capabilities.push_back("military.air");

	REQUIRE(manifest.canonicalize());
	CHECK(manifest.is_valid_contract());
}

TEST_CASE(
	"Invalid explicit capability package is rejected before runtime construction",
	"[convergence][setting-composition][006A3.2][preconstruction]"
) {
	GameManager manager = make_manager();

	SettingCapabilityManifest manifest {
		.package_id = "invalid.runtime-bootstrap",
		.capabilities = {
			cap(setting_capability::POPULATION_NUTRITION_HEALTH)
		}
	};
	REQUIRE(manifest.canonicalize());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);

	CHECK_FALSE(manager.setup_native_instance(std::move(bootstrap)));
	CHECK(manager.get_instance_manager() == nullptr);
}

TEST_CASE(
	"Universal substrate remains available with a minimal explicit package",
	"[convergence][setting-composition][006A3.2][substrate]"
) {
	GameManager manager = make_manager();
	REQUIRE(manager.load_native_economy_bootstrap(native_ruleset_data_root()));

	SettingCapabilityManifest manifest = baseline_manifest("reference.substrate-proof");
	REQUIRE(manifest.is_valid_contract());

	NativeInstanceBootstrap bootstrap;
	bootstrap.setting_capabilities = std::move(manifest);
	REQUIRE(manager.setup_native_instance(std::move(bootstrap)));

	InstanceManager* const instance = manager.get_instance_manager();
	REQUIRE(instance != nullptr);

	CHECK_FALSE(instance->is_population_nutrition_health_capability_enabled());
	CHECK_FALSE(instance->is_live_aggregate_production_chain_enabled());
	CHECK(instance->get_simulation_time() == SimTime::from_ticks(0));

	REQUIRE(instance->bootstrap_position_occupancy(
		PositionOccupancy {
			.position = PositionIdentity {
				.position_id = "substrate.position",
				.jurisdiction_id = "substrate.jurisdiction"
			},
			.controller_id = "substrate.controller",
			.controller_kind = PositionControllerKind::human
		},
		std::vector<AuthorityGrant> {
			AuthorityGrant {
				.command_type = "substrate.command",
				.jurisdiction_id = "substrate.jurisdiction"
			}
		}
	));

	CHECK(
		instance->submit_occupied_position_command(
			"substrate.command",
			std::vector<uint8_t> {}
		) == CommandAdmissionResult::accepted
	);

	ObservationReport report {
		.recipient_actor_id = "substrate.actor",
		.source_id = "substrate.source",
		.fact_type = "substrate.fact",
		.subject_id = "substrate.subject",
		.observed_at = SimTime::from_ticks(0),
		.deliver_at = SimTime::from_ticks(24),
		.payload = { 0x01 },
		.integrity = ObservationIntegrity::DIRECT,
		.confidence_basis_points = 10'000
	};

	REQUIRE(instance->submit_actor_observation_report(std::move(report)).has_value());
	CHECK(instance->get_pending_actor_report_count() == 1);
}
