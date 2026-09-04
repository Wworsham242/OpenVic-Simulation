#pragma once

#include <optional>
#include <string>
#include <vector>

#include <openvic-dataloader/v2script/Parser.hpp>

#include "openvic-simulation/dataloader/NodeTools.hpp"
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
		}
		bool load_live_economy_scenario_file(ast::NodeCPtr root) {
			using namespace NodeTools;

			std::string_view upstream_process_identifier;
			std::string_view downstream_process_identifier;
			std::string_view source_inflow_good_identifier;

			fixed_point_t upstream_capacity = 0;
			fixed_point_t upstream_utilization = fixed_point_t::_1;
			fixed_point_t downstream_capacity = 0;
			fixed_point_t downstream_utilization = fixed_point_t::_1;
			fixed_point_t source_inflow_per_daily_tick = 0;

			market_node_index_t source_node {};
			market_node_index_t destination_node {};

			memory::vector<fixed_point_t> corridor_capacities;

			bool ret = expect_dictionary_keys(
				"upstream_process", ONE_EXACTLY,
					expect_identifier(
						[&upstream_process_identifier](std::string_view value) -> bool {
							upstream_process_identifier = value;
							return true;
						}
					),
				"downstream_process", ONE_EXACTLY,
					expect_identifier(
						[&downstream_process_identifier](std::string_view value) -> bool {
							downstream_process_identifier = value;
							return true;
						}
					),
				"upstream_capacity", ONE_EXACTLY,
					expect_fixed_point(assign_variable_callback(upstream_capacity)),
				"upstream_utilization", ZERO_OR_ONE,
					expect_fixed_point(assign_variable_callback(upstream_utilization)),
				"downstream_capacity", ONE_EXACTLY,
					expect_fixed_point(assign_variable_callback(downstream_capacity)),
				"downstream_utilization", ZERO_OR_ONE,
					expect_fixed_point(assign_variable_callback(downstream_utilization)),
				"source_inflow_good", ONE_EXACTLY,
					expect_identifier(
						[&source_inflow_good_identifier](std::string_view value) -> bool {
							source_inflow_good_identifier = value;
							return true;
						}
					),
				"source_inflow_per_daily_tick", ONE_EXACTLY,
					expect_fixed_point(assign_variable_callback(source_inflow_per_daily_tick)),
				"source_node", ONE_EXACTLY,
					expect_index<market_node_index_t>(assign_variable_callback(source_node)),
				"destination_node", ONE_EXACTLY,
					expect_index<market_node_index_t>(assign_variable_callback(destination_node)),
				"corridor_capacities", ONE_EXACTLY,
					expect_list_reserve_length(
						corridor_capacities,
						expect_fixed_point(vector_callback(corridor_capacities))
					)
			)(root);

			ProductionType const* const upstream_process =
				production_type_manager.get_production_type_by_identifier(
					upstream_process_identifier
				);
			ProductionType const* const downstream_process =
				production_type_manager.get_production_type_by_identifier(
					downstream_process_identifier
				);
			GoodDefinition const* const source_inflow_good =
				good_definition_manager.get_good_definition_by_identifier(
					source_inflow_good_identifier
				);

			if (
				upstream_process == nullptr ||
				downstream_process == nullptr ||
				source_inflow_good == nullptr
			) {
				return false;
			}

			LiveEconomyScenarioDefinition scenario {
				.upstream_process = upstream_process,
				.downstream_process = downstream_process,
				.upstream_capacity = upstream_capacity,
				.upstream_utilization = upstream_utilization,
				.downstream_capacity = downstream_capacity,
				.downstream_utilization = downstream_utilization,
				.source_inflow_good = source_inflow_good,
				.source_inflow_per_daily_tick = source_inflow_per_daily_tick,
				.source_node = source_node,
				.destination_node = destination_node,
				.corridor_legs = {}
			};

			scenario.corridor_legs.reserve(corridor_capacities.size());
			for (fixed_point_t const capacity : corridor_capacities) {
				scenario.corridor_legs.push_back(TransportLeg {
					.nominal_capacity = capacity,
					.availability_fraction = fixed_point_t::_1,
					.open = true
				});
			}

			if (!ret || !scenario.is_valid()) {
				return false;
			}

			return configure_live_economy_scenario(std::move(scenario));
		}	};
}
