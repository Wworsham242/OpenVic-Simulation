#include "openvic-simulation/core/random/RandomGenerator.hpp"
#include "openvic-simulation/country/CountryDefinition.hpp"
#include "openvic-simulation/country/CountryInstance.hpp"
#include "openvic-simulation/country/CountryInstanceDeps.hpp"
#include "openvic-simulation/country/SharedCountryValues.hpp"
#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/diplomacy/CountryRelation.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerDeps.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperationDeps.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/environment/ProvinceEnvironmentalState.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/ProvinceInstanceDeps.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/population/PopDeps.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/population/PopManager.hpp"
#include "openvic-simulation/population/PopValuesFromProvince.hpp"
#include "openvic-simulation/population/Religion.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

#include <array>
#include <optional>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
    struct TestProvince : ProvinceInstance {
        using ProvinceInstance::ProvinceInstance;

        void aggregate_pops_for_test() {
            clear_pops_aggregate();
            for (Pop const& pop : get_pops()) {
                add_pops_aggregate(pop);
            }
            normalise_pops_aggregate();
        }
    };

    struct FoodNeedsFixture {
        GameRulesManager rules;
        GoodDefinitionManager definitions;
        GoodDefinition const* crop = nullptr;
        std::optional<GoodInstanceManager> goods;

        Date date;
        ThreadPool threads { date };
        DefineManager defines;
        ModifierManager modifiers;
        BuildingTypeManager buildings;
        UnitTypeManager units;
        CountryRelationManager relations;

        PopManager pop_manager;

        PopsAggregateDeps aggregates {
            {},
            {},
            pop_type_index_t { 1 },
            {},
            strata_index_t { 1 }
        };

        GraphicalCultureType graphics {
            "test",
            graphical_culture_index_t { 0 }
        };

        CultureGroup culture_group {
            "test",
            "test",
            graphics,
            false,
            nullptr
        };

        Culture culture {
            "test",
            colour_t { 0x12, 0x34, 0x56 },
            culture_group,
            {},
            {},
            0,
            nullptr
        };

        ReligionGroup religion_group { "test" };

        Religion religion {
            "test",
            colour_t { 0x12, 0x34, 0x56 },
            religion_group,
            1,
            false
        };

        Strata strata {
            "workers",
            strata_index_t { 0 }
        };

        std::optional<PopType> workers;

        CountryDefinition country_definition {
            "TST",
            colour_t { 0x12, 0x34, 0x56 },
            country_index_t { 1 },
            graphics,
            IdentifierRegistry<CountryParty> { "parties" },
            {},
            false,
            {},
            colour_t { 0x12, 0x34, 0x56 },
            colour_t { 0x12, 0x34, 0x56 },
            colour_t { 0x12, 0x34, 0x56 }
        };

        std::optional<MarketInstance> market;
        std::optional<SharedCountryValues> shared;
        std::optional<CountryInstance> country;

        ProvinceDefinition province_definition {
            "farm",
            colour_t { 0x12, 0x34, 0x56 },
            province_index_t { 0 }
        };

        std::optional<ProductionType> farm_production;
        ProductionTypeManager production_type_manager;
        std::optional<TestProvince> province;
        std::optional<PopValuesFromProvince> pop_values;

        explicit FoodNeedsFixture(
            fixed_point_t water_availability = fixed_point_t::_1
        ) {
            // Match the 004A1 native fixture. With no controller set,
            // country_to_report_economy remains null and avoids fabricating
            // country-index market buffers for this causal proof.
            rules.set_country_to_report_economy(
                country_to_report_economy_t::Controller
            );

            // Minimal deterministic POP economic defines required by
            // the native needs calculation. The default-constructed
            // DefineManager intentionally leaves loaded-game defines at 0.
            defines.configure_pops_economic_test_values(
                fixed_point_t::_1,
                fixed_point_t::_1
            );

            REQUIRE(definitions.add_good_category("agriculture", 1));

            auto* category =
                definitions.get_good_category_by_identifier("agriculture");
            REQUIRE(category != nullptr);

            REQUIRE(definitions.add_good_definition(
                "crop",
                colour_rgb_t {},
                *const_cast<GoodCategory*>(category),
                1,
                true,
                true,
                false,
                false
            ));

            definitions.lock_good_categories();
            definitions.lock_good_definitions();

            crop = definitions.get_good_definition_by_identifier("crop");
            REQUIRE(crop != nullptr);
            fixed_point_map_t<good_index_t> life_needs;
            life_needs[crop->index] = fixed_point_t { 12000 };
            workers.emplace(
                "workers",
                colour_t { 0x12, 0x34, 0x56 },
                pop_type_index_t { 0 },
                strata,
                pop_sprite_t {},
                std::move(life_needs),
                fixed_point_map_t<good_index_t> {},
                fixed_point_map_t<good_index_t> {},
                PopType::income_type_t::NO_INCOME_TYPE,
                PopType::income_type_t::NO_INCOME_TYPE,
                PopType::income_type_t::NO_INCOME_TYPE,
                PopType::rebel_units_t {},
                pop_size_t { 1000 },
                pop_size_t { 1000 },
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                false,
                true,
                0,
                0,
                0,
                0,
                nullptr,
                ConditionalWeightFactorMul {},
                ConditionalWeightFactorMul {},
                PopType::poptype_weight_map_t { create_empty },
                PopType::ideology_weight_map_t { create_empty },
                PopType::issue_weight_map_t {}
            );
            REQUIRE(modifiers.setup_modifier_effects());

            // PopValuesFromProvince reads the native strata modifier
            // cache. Populate it through the same PopManager path used
            // by normal definition initialization.
            REQUIRE(pop_manager.setup_stratas());
            REQUIRE(pop_manager.generate_modifiers(modifiers));

            REQUIRE(definitions.generate_modifiers(modifiers));
            goods.emplace(definitions, rules);
            market.emplace(
                threads,
                defines.get_country_defines(),
                *goods
            );
            shared.emplace(
                defines.get_pops_defines(),
                *goods,
                std::span<const PopType> {},
                units.get_regiment_types()
            );
            country.emplace(
                country_definition,
                *shared,
                CountryInstanceDeps {
                    {},
                    defines.get_country_defines(),
                    relations,
                    {},
                    date,
                    defines.get_diplomacy_defines(),
                    defines.get_economy_defines(),
                    {},
                    {},
                    rules,
                    goods->get_good_instances(),
                    *goods,
                    {},
                    *market,
                    defines.get_military_defines(),
                    modifiers.get_modifier_effect_cache(),
                    aggregates,
                    {},
                    {},
                    units.get_regiment_types(),
                    units.get_ship_types(),
                    {},
                    {},
                    units
                }
            );

            ResourceGatheringOperationDeps rgo_deps {
                *market,
                modifiers.get_modifier_effect_cache(),
                pop_type_index_t { 1 }
            };
            province.emplace(
                province_definition,
                ProvinceInstanceDeps {
                    buildings,
                    rules,
                    aggregates,
                    rgo_deps,
                    {}
                }
            );

            REQUIRE(province->set_owner(&*country));
            farm_production.emplace(
                rules,
                "native_food_rgo",
                std::nullopt,
                memory::vector<Job> {
                    Job {
                        workers->index,
                        Job::effect_t::THROUGHPUT,
                        1,
                        1
                    }
                },
                ProductionType::template_type_t::RGO,
                pop_size_t { 100 },
                fixed_point_map_t<GoodDefinition const*> {},
                *crop,
                8,
                memory::vector<ProductionType::bonus_t> {},
                fixed_point_map_t<GoodDefinition const*> {},
                false,
                true,
                false
            );

            REQUIRE(
                province->set_rgo_production_type_nullable(
                    {},
                    &*farm_production
                )
            );

            ArtisanalProducerDeps artisan_deps {
                defines.get_economy_defines(),
                {},
                modifiers.get_modifier_effect_cache()
            };

            PopDeps pop_deps {
                artisan_deps,
                *market,
                aggregates
            };

            struct InitialPop : PopBase {
                InitialPop(
                    PopType const& type,
                    Culture const& culture,
                    Religion const& religion
                )
                    : PopBase {
                        type,
                        culture,
                        religion,
                        pop_size_t { 100 },
                        0,
                        0,
                        nullptr
                    } {}
            };

            std::array<PopBase, 1> initial {
                InitialPop { *workers, culture, religion }
            };
            REQUIRE(province->add_pop_vec(initial, pop_deps));
            province->aggregate_pops_for_test();

            province->get_mutable_rgo().initialise_rgo_size_multiplier();

            province->set_environmental_state(
                ProvinceEnvironmentalState { water_availability }
            );
            pop_values.emplace(
                rules,
                *goods,
                modifiers.get_modifier_effect_cache(),
                production_type_manager,
                defines.get_pops_defines(),
                strata_index_t { 1 }
            );
        }

        Pop& consumer() {
            return *province->get_mutable_pops().begin();
        }

        struct CycleResult {
            fixed_point_t output = 0;
            fixed_point_t market_supply = 0;
            fixed_point_t market_traded = 0;
            fixed_point_t life_need_requested = 0;
            fixed_point_t life_needs_fulfilled = 0;
            fixed_point_t cash_before_purchase = 0;

            bool operator==(CycleResult const&) const = default;
        };

        CycleResult run_cycle() {
            Pop& pop = consumer();

            // Deliberately make household purchasing power non-binding.
            // This isolates physical food scarcity from wage/income effects.
            pop.add_event_and_decision_income(fixed_point_t { 100 });

            const fixed_point_t cash_before_purchase = pop.get_cash().get_copy_of_value();

            auto& rgo = province->get_mutable_rgo();

            memory::vector<fixed_point_t> rgo_scratch;
            rgo.rgo_tick(rgo_scratch);

            pop_values->update_pop_values_from_province(*province);
            memory::vector<char> goods_mask(
                definitions.get_good_definition_count(),
                0
            );

            std::array<
                memory::vector<fixed_point_t>,
                Pop::VECTORS_FOR_POP_TICK
            > pop_vectors;

            RandomU32 random {};
            pop.pop_tick(
                *pop_values,
                random,
                goods_mask,
                pop_vectors
            );
            fixed_point_t requested = 0;
            auto const& life_needs = pop.get_life_needs();
            auto need_it = life_needs.find(crop->index);
            if (need_it != life_needs.end()) {
                requested = need_it.value();
            }

            auto& crop_market =
                goods->get_good_instance_by_definition(*crop);

            std::array<
                memory::vector<fixed_point_t>,
                GoodMarket::VECTORS_FOR_EXECUTE_ORDERS
            > market_vectors;
            crop_market.execute_orders(
                {},
                {},
                market_vectors
            );
            return {
                rgo.get_output_quantity_yesterday(),
                crop_market.get_total_supply_yesterday(),
                crop_market.get_quantity_traded_yesterday(),
                requested,
                pop.get_life_needs_fulfilled(),
                cash_before_purchase
            };
        }
    };
}

TEST_CASE(
    "004A2 neutral food supply fulfills native POP life need",
    "[convergence][004a2][environment][agriculture][market][population]"
) {
    FoodNeedsFixture fixture { fixed_point_t::_1 };

    auto result = fixture.run_cycle();

    CHECK(result.output == fixed_point_t { 8 });
    CHECK(result.market_supply == result.output);

    // 12000 base need * 100 POP / 200000 denominator = 6.
    CHECK(result.life_need_requested == fixed_point_t { 6 });

    CHECK(result.market_supply >= result.life_need_requested);
    CHECK(result.life_needs_fulfilled == fixed_point_t::_1);
}

TEST_CASE(
    "004A2 environmental food scarcity lowers native POP life-needs fulfillment",
    "[convergence][004a2][environment][agriculture][market][population]"
) {
    FoodNeedsFixture neutral { fixed_point_t::_1 };
    FoodNeedsFixture stressed { fixed_point_t::_0_50 };

    auto normal = neutral.run_cycle();
    auto dry = stressed.run_cycle();

    CHECK(normal.output == fixed_point_t { 8 });
    CHECK(dry.output == fixed_point_t { 4 });

    CHECK(dry.output < normal.output);
    CHECK(dry.market_supply < normal.market_supply);
    CHECK(dry.market_traded < normal.market_traded);

    CHECK(normal.life_needs_fulfilled == fixed_point_t::_1);
    CHECK(dry.life_needs_fulfilled < normal.life_needs_fulfilled);
    CHECK(dry.life_needs_fulfilled > fixed_point_t::_0);
}

TEST_CASE(
    "004A2 scarcity effect is physical not purchasing-power driven",
    "[convergence][004a2][environment][market][population]"
) {
    FoodNeedsFixture neutral { fixed_point_t::_1 };
    FoodNeedsFixture stressed { fixed_point_t::_0_50 };

    auto normal = neutral.run_cycle();
    auto dry = stressed.run_cycle();

    CHECK(normal.cash_before_purchase == dry.cash_before_purchase);
    CHECK(normal.life_need_requested == dry.life_need_requested);

    CHECK(normal.market_supply >= normal.life_need_requested);
    CHECK(dry.market_supply < dry.life_need_requested);

    CHECK(dry.life_needs_fulfilled < normal.life_needs_fulfilled);
}

TEST_CASE(
    "004A2 POP desired food demand is unchanged by upstream environmental stress",
    "[convergence][004a2][environment][population]"
) {
    FoodNeedsFixture neutral { fixed_point_t::_1 };
    FoodNeedsFixture stressed { fixed_point_t::_0_50 };

    auto normal = neutral.run_cycle();
    auto dry = stressed.run_cycle();

    CHECK(normal.life_need_requested == fixed_point_t { 6 });
    CHECK(dry.life_need_requested == normal.life_need_requested);

    CHECK(dry.output < normal.output);
    CHECK(dry.life_needs_fulfilled < normal.life_needs_fulfilled);
}

TEST_CASE(
    "004A2 recovered environmental state restores food fulfillment",
    "[convergence][004a2][environment][agriculture][market][population]"
) {
    FoodNeedsFixture fixture { fixed_point_t::_0_50 };

    auto dry = fixture.run_cycle();

    REQUIRE(dry.life_needs_fulfilled < fixed_point_t::_1);

    fixture.province->set_environmental_state(
        ProvinceEnvironmentalState {}
    );

    auto recovered = fixture.run_cycle();

    CHECK(dry.output == fixed_point_t { 4 });
    CHECK(recovered.output == fixed_point_t { 8 });
    CHECK(recovered.market_supply == fixed_point_t { 8 });
    CHECK(recovered.life_need_requested == fixed_point_t { 6 });
    CHECK(recovered.life_needs_fulfilled == fixed_point_t::_1);
}

TEST_CASE(
    "004A2 deterministic scarcity produces deterministic POP welfare",
    "[convergence][004a2][determinism][population]"
) {
    FoodNeedsFixture first { fixed_point_t::_0_50 };
    FoodNeedsFixture second { fixed_point_t::_0_50 };

    auto first_result = first.run_cycle();
    auto second_result = second.run_cycle();

    CHECK(first_result == second_result);
}
