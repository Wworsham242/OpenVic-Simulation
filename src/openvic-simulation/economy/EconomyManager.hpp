#pragma once

#include <optional>

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/LiveEconomyScenario.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"

namespace OpenVic {
	struct EconomyManager {
	private:
		BuildingTypeManager PROPERTY_REF(building_type_manager);
		GoodDefinitionManager PROPERTY_REF(good_definition_manager);
		ProductionTypeManager PROPERTY_REF(production_type_manager);
		std::optional<LiveEconomyScenarioDefinition> live_economy_scenario_definition;

	public:
		inline bool load_production_types_file(
			GameRulesManager const& game_rules_manager,
			PopManager const& pop_manager,
			ovdl::v2script::Parser const& parser
		) {
			return production_type_manager.load_production_types_file(
				game_rules_manager,
				good_definition_manager,
				pop_manager,
				parser
			);
		}

		inline bool load_buildings_file(ModifierManager& modifier_manager, ast::NodeCPtr root) {
			return building_type_manager.load_buildings_file(
				good_definition_manager, production_type_manager, modifier_manager, root
			);
		}

		[[nodiscard]] bool configure_live_economy_scenario(
			LiveEconomyScenarioDefinition scenario
		) {
			if (!scenario.is_valid()) {
				return false;
			}
			live_economy_scenario_definition = std::move(scenario);
			return true;
		}

		[[nodiscard]] LiveEconomyScenarioDefinition const* get_live_economy_scenario() const {
			return live_economy_scenario_definition.has_value()
				? &*live_economy_scenario_definition
				: nullptr;
		}	};
}
