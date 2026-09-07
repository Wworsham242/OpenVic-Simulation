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
#include "openvic-simulation/population/PopManager.hpp"

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

		/* Setting-general capacity assets reuse inherited BuildingType rather
		 * than creating a parallel facility system. */
		bool load_modern_facility_catalog_file(ast::NodeCPtr root) {
			using namespace NodeTools;

			return expect_dictionary(
				[this](std::string_view facility_identifier, ast::NodeCPtr facility_node) -> bool {
					std::string_view process_identifier;
					building_level_t max_level { 0 };
					fixed_point_t capacity_per_level = 0;
					fixed_point_map_t<GoodDefinition const*> goods_cost;
					Timespan build_time {};

					bool ret = expect_dictionary_keys(
						"production_process", ZERO_OR_ONE,
							expect_identifier(assign_variable_callback(process_identifier)),
						"max_level", ONE_EXACTLY,
							expect_strong_typedef<building_level_t>(assign_variable_callback(max_level)),
						"capacity_per_level", ONE_EXACTLY,
							expect_fixed_point(assign_variable_callback(capacity_per_level)),
						"goods_cost", ONE_EXACTLY,
							good_definition_manager.expect_good_definition_decimal_map(
								move_variable_callback(goods_cost)
							),
						"time", ONE_EXACTLY,
							expect_days(assign_variable_callback(build_time))
					)(facility_node);

					if (!ret || max_level <= building_level_t { 0 } || capacity_per_level <= fixed_point_t::_0) {
						return false;
					}

					ProductionType const* process = nullptr;
					if (!process_identifier.empty()) {
						process = production_type_manager.get_production_type_by_identifier(process_identifier);
						if (process == nullptr || !process->is_setting_general_process()) {
							return false;
						}
					}

					BuildingType::building_type_args_t args {};
					args.type = "facility";
					args.max_level = max_level;
					args.capacity_per_level = capacity_per_level;
					args.goods_cost = std::move(goods_cost);
					args.build_time = build_time;
					args.production_type = process;
					args.default_enabled = true;
					args.in_province = false;

					return building_type_manager.add_building_type(
						facility_identifier,
						args
					);
				}
			)(root);
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
		}
        /* LIVE-ECONOMY-005: preload additive modern goods before the
         * legacy goods loader performs its normal one-time registry lock. */
        bool load_modern_goods_catalog_file(ast::NodeCPtr root) {
            using namespace NodeTools;

            return expect_dictionary(
                [this](std::string_view category_identifier, ast::NodeCPtr category_node) -> bool {
                    size_t expected_goods = 0;
                    bool ret = expect_length(assign_variable_callback(expected_goods))(category_node);
                    ret &= good_definition_manager.add_good_category(category_identifier, expected_goods);

                    GoodCategory const* const category_const =
                        good_definition_manager.get_good_category_by_identifier(category_identifier);
                    if (category_const == nullptr) {
                        return false;
                    }
                    GoodCategory& category = const_cast<GoodCategory&>(*category_const);

                    ret &= expect_dictionary(
                        [this, &category](std::string_view good_identifier, ast::NodeCPtr good_node) -> bool {
                            colour_t colour = colour_t::null();
                            fixed_point_t cost = 0;

                            bool good_ret = expect_dictionary_keys(
                                "color", ONE_EXACTLY, expect_colour(assign_variable_callback(colour)),
                                "cost", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(cost))
                            )(good_node);

                            good_ret &= good_definition_manager.add_good_definition(
                                good_identifier, colour, category, cost,
                                true, true, false, false
                            );
                            return good_ret;
                        }
                    )(category_node);

                    return ret;
                }
            )(root);
        }

        /* Add setting-general production processes after legacy production
         * types are loaded. PROCESS reuses OpenVic's ProductionType inputs,
         * outputs, workforce scale and maintenance requirements without
         * Victoria owner/job actor semantics. */
        bool load_modern_production_catalog_file(
            GameRulesManager const& game_rules_manager,
            PopManager const& pop_manager,
            ast::NodeCPtr root
        ) {
            using namespace NodeTools;

            return expect_dictionary(
                [this, &game_rules_manager, &pop_manager](
                    std::string_view process_identifier,
                    ast::NodeCPtr process_node
                ) -> bool {
                    pop_size_t workforce { 1 };
                    fixed_point_map_t<GoodDefinition const*> input_goods;
                    fixed_point_map_t<GoodDefinition const*> maintenance_requirements;
                    GoodDefinition const* output_good = nullptr;
                    fixed_point_t output_value = 0;

                    bool ret = expect_dictionary_keys(
                        "workforce", ZERO_OR_ONE,
                            expect_strong_typedef<pop_size_t>(assign_variable_callback(workforce)),
                        "input_goods", ONE_EXACTLY,
                            good_definition_manager.expect_good_definition_decimal_map(
                                move_variable_callback(input_goods)
                            ),
                        "maintenance_requirements", ZERO_OR_ONE,
                            good_definition_manager.expect_good_definition_decimal_map(
                                move_variable_callback(maintenance_requirements)
                            ),
                        "output_good", ONE_EXACTLY,
                            good_definition_manager.expect_good_definition_identifier(
                                assign_variable_callback_pointer(output_good)
                            ),
                        "value", ONE_EXACTLY,
                            expect_fixed_point(assign_variable_callback(output_value))
                    )(process_node);

                    if (!ret) {
                        return false;
                    }

                    return production_type_manager.add_production_type(
                        game_rules_manager,
                        pop_manager.get_pop_types(),
                        process_identifier,
                        std::nullopt,
                        memory::vector<Job> {},
                        ProductionType::template_type_t::PROCESS,
                        workforce,
                        std::move(input_goods),
                        output_good,
                        output_value,
                        memory::vector<ProductionType::bonus_t> {},
                        std::move(maintenance_requirements),
                        false, false, false
                    );
                }
            )(root);
        }	};
}
