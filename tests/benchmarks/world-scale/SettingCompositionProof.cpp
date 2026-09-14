#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "openvic-simulation/core/simulation/SettingCapabilityManifest.hpp"

using namespace OpenVic;

namespace {

SettingCapabilityManifest make_package(
	std::string id,
	std::vector<std::string> capabilities
) {
	SettingCapabilityManifest manifest {
		.package_id = std::move(id),
		.capabilities = std::move(capabilities)
	};

	if (!manifest.canonicalize()) {
		std::cerr << "Failed to canonicalize setting package\n";
		std::exit(2);
	}

	return manifest;
}

}

int main() {
	/*
	 * Reference packages are test data only. The engine does not infer era
	 * semantics from these package IDs.
	 */
	SettingCapabilityManifest bronze = make_package(
		"reference.bronze-age",
		{
			"governance.basic",
			"logistics.land",
			"population.basic",
			"production.basic",
			"trade.physical"
		}
	);

	SettingCapabilityManifest modern = make_package(
		"reference.modern",
		{
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
	);

	SettingCapabilityManifest modern_repeat = make_package(
		"reference.modern",
		{
			"trade.physical",
			"military.air",
			"finance.credit",
			"production.basic",
			"population.nutrition-health",
			"logistics.land",
			"information.cyber",
			"population.basic",
			"infrastructure.electric-grid",
			"governance.basic",
			"finance.banking"
		}
	);

	SettingCapabilityManifest duplicate {
		.package_id = "invalid.duplicate",
		.capabilities = {
			"population.basic",
			"population.basic"
		}
	};
	bool const duplicate_rejected = !duplicate.canonicalize();

	SettingCapabilityManifest empty_capability {
		.package_id = "invalid.empty",
		.capabilities = {
			"population.basic",
			""
		}
	};
	bool const empty_rejected = !empty_capability.canonicalize();

	bool const bronze_canonical = bronze.is_canonical();
	bool const modern_canonical = modern.is_canonical();

	bool const shared_population =
		bronze.has("population.basic") &&
		modern.has("population.basic");

	bool const shared_production =
		bronze.has("production.basic") &&
		modern.has("production.basic");

	bool const bronze_omits_modern_finance =
		!bronze.has("finance.banking") &&
		!bronze.has("finance.credit");

	bool const bronze_omits_modern_infrastructure =
		!bronze.has("infrastructure.electric-grid") &&
		!bronze.has("information.cyber") &&
		!bronze.has("military.air");

	bool const bronze_omits_optional_nutrition_history =
		!bronze.has("population.nutrition-health");

	bool const modern_enables_optional_nutrition_history =
		modern.has("population.nutrition-health");

	bool const deterministic_composition =
		modern.checksum() == modern_repeat.checksum() &&
		modern.capabilities == modern_repeat.capabilities;

	bool const packages_are_distinct =
		bronze.checksum() != modern.checksum();

	std::cout
		<< "{\n"
		<< "  \"increment\": \"PROJECT-CONVERGENCE-006A3.2\",\n"
		<< "  \"scope\": \"era-neutral-setting-capability-contract\",\n"
		<< "  \"packages\": {\n"
		<< "    \"reference_bronze_age\": {\n"
		<< "      \"capability_count\": " << bronze.capabilities.size() << ",\n"
		<< "      \"checksum\": " << bronze.checksum() << "\n"
		<< "    },\n"
		<< "    \"reference_modern\": {\n"
		<< "      \"capability_count\": " << modern.capabilities.size() << ",\n"
		<< "      \"checksum\": " << modern.checksum() << "\n"
		<< "    }\n"
		<< "  },\n"
		<< "  \"validation\": {\n"
		<< "    \"bronze_canonical\": " << (bronze_canonical ? "true" : "false") << ",\n"
		<< "    \"modern_canonical\": " << (modern_canonical ? "true" : "false") << ",\n"
		<< "    \"duplicate_rejected\": " << (duplicate_rejected ? "true" : "false") << ",\n"
		<< "    \"empty_capability_rejected\": " << (empty_rejected ? "true" : "false") << ",\n"
		<< "    \"shared_population_substrate\": " << (shared_population ? "true" : "false") << ",\n"
		<< "    \"shared_production_substrate\": " << (shared_production ? "true" : "false") << ",\n"
		<< "    \"bronze_omits_modern_finance\": " << (bronze_omits_modern_finance ? "true" : "false") << ",\n"
		<< "    \"bronze_omits_modern_infrastructure\": " << (bronze_omits_modern_infrastructure ? "true" : "false") << ",\n"
		<< "    \"bronze_omits_optional_nutrition_history\": " << (bronze_omits_optional_nutrition_history ? "true" : "false") << ",\n"
		<< "    \"modern_enables_optional_nutrition_history\": " << (modern_enables_optional_nutrition_history ? "true" : "false") << ",\n"
		<< "    \"deterministic_composition\": " << (deterministic_composition ? "true" : "false") << ",\n"
		<< "    \"packages_are_distinct\": " << (packages_are_distinct ? "true" : "false") << "\n"
		<< "  },\n"
		<< "  \"limitations\": [\n"
		<< "    \"reference package IDs are test data and are not engine era enums\",\n"
		<< "    \"this increment proves canonical capability composition plus dependency-contract validation for currently bound optional mechanisms\",\n"
		<< "    \"population.nutrition-health and economy.aggregate-production-chain have concrete runtime composition seams\",\n"
		<< "    \"additional mechanisms must be runtime-bound only when they have clean ownership seams and without creating parallel authorities\"\n"
		<< "  ]\n"
		<< "}\n";

	bool const pass =
		bronze_canonical &&
		modern_canonical &&
		duplicate_rejected &&
		empty_rejected &&
		shared_population &&
		shared_production &&
		bronze_omits_modern_finance &&
		bronze_omits_modern_infrastructure &&
		bronze_omits_optional_nutrition_history &&
		modern_enables_optional_nutrition_history &&
		deterministic_composition &&
		packages_are_distinct;

	return pass ? 0 : 1;
}
