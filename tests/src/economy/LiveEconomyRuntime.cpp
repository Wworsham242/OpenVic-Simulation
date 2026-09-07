#include "openvic-simulation/economy/LiveEconomyRuntime.hpp"

#include <array>
#include <optional>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/LiveEconomyScenario.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"
#include "openvic-simulation/defines/Define.hpp"
#include "openvic-simulation/economy/production/ArtisanalProducerDeps.hpp"
#include "openvic-simulation/economy/production/ResourceGatheringOperationDeps.hpp"
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/ProvinceInstanceDeps.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/population/Pop.hpp"
#include "openvic-simulation/population/PopDeps.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/population/PopsAggregateDeps.hpp"
#include "openvic-simulation/population/Religion.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	struct LiveEconomyFixture {
		GoodDefinitionManager definitions;
		GameRulesManager rules;

		GoodCategory const* category = nullptr;
		GoodDefinition const* feedstock = nullptr;
		GoodDefinition const* intermediate = nullptr;
		GoodDefinition const* final_good = nullptr;

		std::optional<ProductionType> upstream_process;
		std::optional<ProductionType> downstream_process;

		LiveEconomyFixture(
			memory::vector<Job> jobs = {}, pop_size_t workforce = pop_size_t { 0 },
			ProductionType::template_type_t type = ProductionType::template_type_t::FACTORY
		) {
			REQUIRE(definitions.add_good_category("live", 3));
			category = definitions.get_good_category_by_identifier("live");
			REQUIRE(category != nullptr);

			REQUIRE(definitions.add_good_definition(
				"feedstock", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
				fixed_point_t::_1, true, true, false, false
			));
			REQUIRE(definitions.add_good_definition(
				"intermediate", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
				fixed_point_t::_1, true, true, false, false
			));
			REQUIRE(definitions.add_good_definition(
				"final", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
				fixed_point_t::_1, true, true, false, false
			));

			feedstock = definitions.get_good_definition_by_identifier("feedstock");
			intermediate = definitions.get_good_definition_by_identifier("intermediate");
			final_good = definitions.get_good_definition_by_identifier("final");

			REQUIRE(feedstock != nullptr);
			REQUIRE(intermediate != nullptr);
			REQUIRE(final_good != nullptr);

			fixed_point_map_t<GoodDefinition const*> upstream_inputs;
			upstream_inputs.emplace(feedstock, fixed_point_t::_1);

			upstream_process.emplace(
				rules,
				"live_upstream",
				std::nullopt,
				std::move(jobs),
				type,
				workforce,
				std::move(upstream_inputs),
				*intermediate,
				fixed_point_t::_1,
				memory::vector<ProductionType::bonus_t> {},
				fixed_point_map_t<GoodDefinition const*> {},
				false, false, false
			);

			fixed_point_map_t<GoodDefinition const*> downstream_inputs;
			downstream_inputs.emplace(intermediate, fixed_point_t(2));

			downstream_process.emplace(
				rules,
				"live_downstream",
				std::nullopt,
				memory::vector<Job> {},
				ProductionType::template_type_t::FACTORY,
				pop_size_t { 0 },
				std::move(downstream_inputs),
				*final_good,
				fixed_point_t::_1,
				memory::vector<ProductionType::bonus_t> {},
				fixed_point_map_t<GoodDefinition const*> {},
				false, false, false
			);

			// Runtime instances consume immutable, index-stable good definitions.
			// This fixture is its own definition loader, so it must finalize the
			// registries before constructing GoodInstanceManager.
			definitions.lock_good_categories();
			definitions.lock_good_definitions();
		}

		LiveEconomyScenarioDefinition make_scenario() const {
			return LiveEconomyScenarioDefinition {
				.upstream_process = &*upstream_process,
				.downstream_process = &*downstream_process,
				.upstream_capacity = fixed_point_t(4),
				.upstream_utilization = fixed_point_t::_1,
				.downstream_capacity = fixed_point_t(4),
				.downstream_utilization = fixed_point_t::_1,
				.source_inflow_good = feedstock,
				.source_inflow_per_daily_tick = fixed_point_t(4),
				.source_node = market_node_index_t { 0 },
				.destination_node = market_node_index_t { 1 },
				.corridor_legs = {
					TransportLeg {
						.nominal_capacity = fixed_point_t(4),
						.availability_fraction = fixed_point_t::_1,
						.open = true
					}
				}
			};
		}
	};

	void execute_intermediate_market(GoodInstance& market) {
		TypedSpan<country_index_t, fixed_point_t> map_0 {};
		TypedSpan<country_index_t, fixed_point_t> map_1 {};
		std::array<
			memory::vector<fixed_point_t>,
			GoodMarket::VECTORS_FOR_EXECUTE_ORDERS
		> vectors;

		market.execute_orders(map_0, map_1, vectors);
	}
}

TEST_CASE(
	"Live economy runtime advances recurring production through a real GoodMarket",
	"[economy][live-runtime]"
) {
	LiveEconomyFixture fixture;

	GoodInstanceManager good_instances {
		fixture.definitions,
		fixture.rules
	};

	LiveEconomyScenarioDefinition scenario = fixture.make_scenario();
	REQUIRE(scenario.is_valid());

	LiveEconomyRuntime runtime {
		fixture.rules,
		good_instances,
		scenario
	};

	GoodInstance& intermediate_market =
		good_instances.get_good_instance_by_definition(*fixture.intermediate);

	LiveEconomyStatus initial = runtime.get_status();
	REQUIRE(initial.configured);
	CHECK(initial.completed_daily_ticks == 0);

	runtime.pre_market_daily_tick();
	execute_intermediate_market(intermediate_market);
	runtime.post_market_daily_tick();

	LiveEconomyStatus first = runtime.get_status();

	CHECK(first.completed_daily_ticks == 1);
	CHECK(first.upstream_output == fixed_point_t(4));
	CHECK(first.corridor_capacity == fixed_point_t(4));
	CHECK(first.downstream_desired_output == fixed_point_t(4));
	CHECK(first.downstream_output == fixed_point_t(2));
	CHECK(first.downstream_input_limited);
	CHECK(first.final_inventory == fixed_point_t(2));
	CHECK(first.intermediate_quantity_traded_yesterday == fixed_point_t(4));
	CHECK(first.intermediate_price > 0);

	runtime.pre_market_daily_tick();
	execute_intermediate_market(intermediate_market);
	runtime.post_market_daily_tick();

	LiveEconomyStatus second = runtime.get_status();

	CHECK(second.completed_daily_ticks == 2);
	CHECK(second.upstream_output == fixed_point_t(4));
	CHECK(second.downstream_output == fixed_point_t(2));
	CHECK(second.final_inventory == fixed_point_t(4));
	CHECK(second.intermediate_quantity_traded_yesterday == fixed_point_t(4));
}

TEST_CASE(
	"Live economy status exposes scenario-owned corridor and market state",
	"[economy][live-runtime][status]"
) {
	LiveEconomyFixture fixture;

	GoodInstanceManager good_instances {
		fixture.definitions,
		fixture.rules
	};

	LiveEconomyScenarioDefinition scenario = fixture.make_scenario();
	REQUIRE(scenario.is_valid());

	LiveEconomyRuntime runtime {
		fixture.rules,
		good_instances,
		scenario
	};

	GoodInstance& intermediate_market =
		good_instances.get_good_instance_by_definition(*fixture.intermediate);

	runtime.pre_market_daily_tick();
	execute_intermediate_market(intermediate_market);
	runtime.post_market_daily_tick();

	LiveEconomyStatus status = runtime.get_status();

	CHECK(status.configured);
	CHECK(status.corridor_capacity == fixed_point_t(4));
	CHECK(status.deliverable_intermediate >= fixed_point_t::_0);
	CHECK(status.intermediate_supply_yesterday == fixed_point_t(4));
	CHECK(status.intermediate_demand_yesterday >= fixed_point_t(4));
	CHECK(status.intermediate_quantity_traded_yesterday == fixed_point_t(4));
}

namespace {
	// Real POP dependencies, with empty political/needs definitions: none are
	// consulted by employment allocation. No mock employment counters.
	struct WorkforcePopFixture {
		Date date;
		ThreadPool threads { date };
		DefineManager defines;
		ModifierManager modifier_manager;
		BuildingTypeManager buildings;
		PopsAggregateDeps aggregates { {}, {}, pop_type_index_t { 2 }, {}, strata_index_t { 1 } };
		MarketInstance market;
		ResourceGatheringOperationDeps rgo_deps {
			market, modifier_manager.get_modifier_effect_cache(), pop_type_index_t { 2 }
		};
		ArtisanalProducerDeps artisan_deps {
			defines.get_economy_defines(), {}, modifier_manager.get_modifier_effect_cache()
		};
		PopDeps pop_deps { artisan_deps, market, aggregates };
		ProvinceDefinition definition { "workplace", colour_t { 0x12, 0x34, 0x56 }, province_index_t { 0 } };
		ProvinceInstance province;
		Strata strata { "workers", strata_index_t { 0 } };
		GraphicalCultureType graphics { "test", graphical_culture_index_t { 0 } };
		CultureGroup group { "test", "test", graphics, false, nullptr };
		Culture culture { "test", colour_t { 0x12, 0x34, 0x56 }, group, {}, {}, 0, nullptr };
		ReligionGroup religion_group { "test" };
		Religion religion { "test", colour_t { 0x12, 0x34, 0x56 }, religion_group, 1, false };
		PopType eligible = make_type("eligible", pop_type_index_t { 0 });
		PopType ineligible = make_type("ineligible", pop_type_index_t { 1 });

		PopType make_type(std::string_view name, pop_type_index_t index) {
			return PopType {
				name, colour_t { 0x12, 0x34, 0x56 }, index, strata, pop_sprite_t {}, {}, {}, {},
				PopType::income_type_t::NO_INCOME_TYPE,
				PopType::income_type_t::NO_INCOME_TYPE,
				PopType::income_type_t::NO_INCOME_TYPE,
				{}, pop_size_t { 1000 }, pop_size_t { 1000 },
				false, false, false, false, false, false,
				false, false, false, false, false, true,
				0, 0, 0, 0, nullptr, {}, {},
				PopType::poptype_weight_map_t { create_empty },
				PopType::ideology_weight_map_t { create_empty }, {}
			};
		}

		WorkforcePopFixture(GameRulesManager const& rules, GoodInstanceManager& goods)
			: market { threads, defines.get_country_defines(), goods },
			province { definition, ProvinceInstanceDeps { buildings, rules, aggregates, rgo_deps, {} } } {}

		Pop make_pop(PopType const& type, int size, size_t id) {
			struct InitialPop : PopBase {
				InitialPop(PopType const& type, Culture const& culture, Religion const& religion, int size)
					: PopBase { type, culture, religion, pop_size_t { size }, 0, 0, nullptr } {}
			};
			return Pop { province, InitialPop { type, culture, religion, size },
				pop_deps, pop_id_in_province_t { id } };
		}
	};

	memory::vector<Job> workforce_jobs() {
		// Duplicate eligibility must never allocate the same POP twice.
		return {
			Job { pop_type_index_t { 0 }, Job::effect_t::THROUGHPUT, 1, 1 },
			Job { pop_type_index_t { 0 }, Job::effect_t::OUTPUT, 1, 1 }
		};
	}
}

TEST_CASE("Native POP allocation propagates labor availability through the live market",
	"[economy][live-runtime][native-workforce]") {
	for (int const already_employed : { 0, 20 }) {
		LiveEconomyFixture fixture {
			workforce_jobs(), pop_size_t { 10 }, ProductionType::template_type_t::PROCESS
		};
		GoodInstanceManager goods { fixture.definitions, fixture.rules };
		WorkforcePopFixture population { fixture.rules, goods };
		std::array pops {
			population.make_pop(population.ineligible, 100, 1),
			population.make_pop(population.eligible, 40, 2)
		};
		if (already_employed > 0) {
			pops[1].hire(pop_size_t { already_employed });
		}
		int const available = 40 - already_employed;
		REQUIRE(pops[1].get_unemployed() == pop_size_t { available });
		auto scenario = fixture.make_scenario();
		LiveEconomyRuntime runtime { fixture.rules, goods, scenario };
		runtime.pre_market_daily_tick(std::span<Pop> { pops });
		CHECK(pops[1].get_unemployed() == pop_size_t { 0 });
		CHECK(pops[0].get_unemployed() == pop_size_t { 100 });
		auto& market = goods.get_good_instance_by_definition(*fixture.intermediate);
		execute_intermediate_market(market);
		runtime.post_market_daily_tick();
		auto status = runtime.get_status();
		CHECK(status.upstream_output == fixed_point_t { available / 10 });
		CHECK(status.intermediate_supply_yesterday == fixed_point_t { available / 10 });
		CHECK(status.intermediate_quantity_traded_yesterday == fixed_point_t { available / 10 });
		CHECK(status.downstream_output == fixed_point_t { available / 20 });
		CHECK(status.downstream_input_limited);
		CHECK(status.intermediate_upstream_inventory == fixed_point_t::_0);
		CHECK(status.intermediate_downstream_inventory == fixed_point_t::_0);
	}
}

TEST_CASE("Native workforce allocation caps hires and respects existing employment",
	"[economy][native-workforce]") {
	LiveEconomyFixture fixture {
		workforce_jobs(), pop_size_t { 10 }, ProductionType::template_type_t::PROCESS
	};
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	WorkforcePopFixture population { fixture.rules, goods };
	std::array pops {
		population.make_pop(population.eligible, 15, 1),
		population.make_pop(population.eligible, 50, 2)
	};
	AggregateProducer first { "first", *fixture.upstream_process, 4, 1 };
	CHECK(allocate_producer_workforce(first, pops) == fixed_point_t { 40 });
	CHECK(first.get_available_workforce() == fixed_point_t { 40 });
	CHECK(pops[0].get_unemployed() == pop_size_t { 0 });
	CHECK(pops[1].get_unemployed() == pop_size_t { 25 });
	AggregateProducer second { "second", *fixture.upstream_process, 4, 1 };
	CHECK(allocate_producer_workforce(second, pops) == fixed_point_t { 25 });
	CHECK(pops[1].get_unemployed() == pop_size_t { 0 });
	AggregateProducer third { "third", *fixture.upstream_process, 4, 1 };
	CHECK(allocate_producer_workforce(third, pops) == fixed_point_t::_0);
	CHECK(third.calculate_desired_output() == fixed_point_t::_0);
	CHECK(allocate_producer_workforce(first, {}) == fixed_point_t::_0);
	CHECK(first.calculate_desired_output() == fixed_point_t::_0);
}
