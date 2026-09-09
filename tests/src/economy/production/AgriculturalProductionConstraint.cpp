#include "openvic-simulation/economy/production/AgriculturalProductionConstraint.hpp"

#include <array>

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
#include "openvic-simulation/economy/trading/MarketInstance.hpp"
#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/ProvinceInstance.hpp"
#include "openvic-simulation/map/ProvinceInstanceDeps.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/modifier/ModifierManager.hpp"
#include "openvic-simulation/population/Culture.hpp"
#include "openvic-simulation/population/PopDeps.hpp"
#include "openvic-simulation/population/PopType.hpp"
#include "openvic-simulation/population/Religion.hpp"
#include "openvic-simulation/utility/ThreadPool.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	// Exercise native POP aggregation without unrelated military/politics updates.
	struct TestProvince : ProvinceInstance {
		using ProvinceInstance::ProvinceInstance;
		void aggregate_workers() {
			clear_pops_aggregate();
			for (Pop const& pop : get_pops()) { add_pops_aggregate(pop); }
			normalise_pops_aggregate();
		}
	};

	struct FarmFixture {
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
		PopsAggregateDeps aggregates { {}, {}, pop_type_index_t { 1 }, {}, strata_index_t { 1 } };
		GraphicalCultureType graphics { "test", graphical_culture_index_t { 0 } };
		CultureGroup group { "test", "test", graphics, false, nullptr };
		Culture culture { "test", colour_t { 0x12, 0x34, 0x56 }, group, {}, {}, 0, nullptr };
		ReligionGroup religion_group { "test" };
		Religion religion { "test", colour_t { 0x12, 0x34, 0x56 }, religion_group, 1, false };
		Strata strata { "workers", strata_index_t { 0 } };
		PopType workers {
			"workers", colour_t { 0x12, 0x34, 0x56 }, pop_type_index_t { 0 }, strata, pop_sprite_t {}, {}, {}, {},
			PopType::income_type_t::NO_INCOME_TYPE, PopType::income_type_t::NO_INCOME_TYPE,
			PopType::income_type_t::NO_INCOME_TYPE, {}, pop_size_t { 1000 }, pop_size_t { 1000 },
			false, false, false, false, false, false, false, false, false, false, false, true,
			0, 0, 0, 0, nullptr, {}, {}, PopType::poptype_weight_map_t { create_empty },
			PopType::ideology_weight_map_t { create_empty }, {}
		};
		CountryDefinition country_definition {
			"TST", colour_t { 0x12, 0x34, 0x56 }, country_index_t { 1 }, graphics, IdentifierRegistry<CountryParty> { "parties" },
			{}, false, {}, colour_t { 0x12, 0x34, 0x56 }, colour_t { 0x12, 0x34, 0x56 }, colour_t { 0x12, 0x34, 0x56 }
		};
		std::optional<MarketInstance> market;
		std::optional<SharedCountryValues> shared;
		std::optional<CountryInstance> country;
		ProvinceDefinition province_definition { "farm", colour_t { 0x12, 0x34, 0x56 }, province_index_t { 0 } };
		std::optional<ProductionType> production;
		std::optional<TestProvince> province;

		explicit FarmFixture(bool farm = true, bool mine = false) {
			// Ordinary controller-reporting rule: this owned, uncontrolled province
			// uses the native unaffiliated market path (no fabricated country arrays).
			rules.set_country_to_report_economy(country_to_report_economy_t::Controller);
			REQUIRE(definitions.add_good_category("agriculture", 1));
			auto* category = definitions.get_good_category_by_identifier("agriculture");
			REQUIRE(category != nullptr);
			REQUIRE(definitions.add_good_definition(
				"crop", colour_rgb_t {}, *const_cast<GoodCategory*>(category), 1, true, true, false, false
			));
			definitions.lock_good_categories();
			definitions.lock_good_definitions();
			crop = definitions.get_good_definition_by_identifier("crop");
			REQUIRE(crop != nullptr);
			REQUIRE(modifiers.setup_modifier_effects());
			REQUIRE(definitions.generate_modifiers(modifiers));
			goods.emplace(definitions, rules);
			market.emplace(threads, defines.get_country_defines(), *goods);
			shared.emplace(defines.get_pops_defines(), *goods, std::span<const PopType> {}, units.get_regiment_types());
			country.emplace(country_definition, *shared, CountryInstanceDeps {
				{}, defines.get_country_defines(), relations, {}, date, defines.get_diplomacy_defines(),
				defines.get_economy_defines(), {}, {}, rules, goods->get_good_instances(), *goods, {}, *market,
				defines.get_military_defines(), modifiers.get_modifier_effect_cache(), aggregates,
				{}, {}, units.get_regiment_types(), units.get_ship_types(), {}, {}, units
			});
			ResourceGatheringOperationDeps rgo_deps { *market, modifiers.get_modifier_effect_cache(), pop_type_index_t { 1 } };
			province.emplace(province_definition, ProvinceInstanceDeps { buildings, rules, aggregates, rgo_deps, {} });
			REQUIRE(province->set_owner(&*country));
			production.emplace(rules, "native_rgo", std::nullopt,
				memory::vector<Job> { Job { workers.index, Job::effect_t::THROUGHPUT, 1, 1 } },
				ProductionType::template_type_t::RGO, pop_size_t { 100 }, fixed_point_map_t<GoodDefinition const*> {},
				*crop, 8, memory::vector<ProductionType::bonus_t> {}, fixed_point_map_t<GoodDefinition const*> {},
				false, farm, mine);
			REQUIRE(province->set_rgo_production_type_nullable({}, &*production));
			ArtisanalProducerDeps artisan_deps { defines.get_economy_defines(), {}, modifiers.get_modifier_effect_cache() };
			PopDeps pop_deps { artisan_deps, *market, aggregates };
			struct InitialPop : PopBase {
				InitialPop(PopType const& type, Culture const& culture, Religion const& religion)
					: PopBase { type, culture, religion, pop_size_t { 100 }, 0, 0, nullptr } {}
			};
			std::array<PopBase, 1> initial { InitialPop { workers, culture, religion } };
			REQUIRE(province->add_pop_vec(initial, pop_deps));
			province->aggregate_workers();
			province->get_mutable_rgo().initialise_rgo_size_multiplier();
		}

		struct Result {
			fixed_point_t output, supply, traded, bought, revenue;
			pop_size_t employed;
			std::optional<AgriculturalProductionConstraintResult> facts;
			bool operator==(Result const&) const = default;
		};

		Result run(bool allocate_workers = true) {
			auto& rgo = province->get_mutable_rgo();
			memory::vector<fixed_point_t> scratch;
			if (allocate_workers) { rgo.rgo_tick(scratch); }
			else { rgo.production_cycle(scratch); }
			auto& good = goods->get_good_instance_by_definition(*crop);
			fixed_point_t bought = 0;
			good.add_buy_up_to_order({ std::nullopt, 100, 1000, &bought,
				[](void* actor, BuyResult const& result) { *static_cast<fixed_point_t*>(actor) = result.quantity_bought; } });
			std::array<memory::vector<fixed_point_t>, GoodMarket::VECTORS_FOR_EXECUTE_ORDERS> vectors;
			good.execute_orders({}, {}, vectors);
			return { rgo.get_output_quantity_yesterday(), good.get_total_supply_yesterday(),
				good.get_quantity_traded_yesterday(), bought, rgo.get_revenue_yesterday(),
				rgo.get_total_employees_count_cache(), rgo.get_agricultural_constraint_yesterday() };
		}
	};
}

TEST_CASE("004A1 neutral environment preserves native farm baseline", "[convergence][004a1][economy][rgo]") {
	FarmFixture fixture;
	auto result = fixture.run();
	REQUIRE(result.facts.has_value());
	CHECK(fixture.province->get_environmental_state() == ProvinceEnvironmentalState {});
	CHECK(result.facts->yield_factor == fixed_point_t::_1);
	// Legacy equation: base 8 * size 1 * workforce throughput 1 * output 1.
	CHECK(result.output == fixed_point_t { 8 });
	CHECK(result.output == result.facts->unconstrained_output);
	CHECK(result.output == result.facts->constrained_output);
	CHECK(result.employed == pop_size_t { 100 });
}

TEST_CASE("004A1 water stress reduces physical farm output only", "[convergence][004a1][economy][rgo]") {
	FarmFixture fixture;
	auto normal = fixture.run();
	// Keep the same province, production type, owner, modifiers and hired POPs.
	fixture.province->set_environmental_state(ProvinceEnvironmentalState { fixed_point_t::_0_50 });
	auto dry = fixture.run(false);
	REQUIRE(dry.facts.has_value());
	CHECK(dry.facts->environment == fixture.province->get_environmental_state());
	CHECK(dry.facts->yield_factor == fixed_point_t::_0_50);
	CHECK(dry.facts->unconstrained_output == normal.output);
	CHECK(dry.output == fixed_point_t { 4 });
	CHECK(dry.output < normal.output);
	CHECK(dry.employed == normal.employed);
	fixture.province->set_environmental_state(ProvinceEnvironmentalState {});
	CHECK(fixture.run(false).output == normal.output);
}

TEST_CASE("004A1 reduced farm quantity reaches native market and buyer", "[convergence][004a1][economy][rgo][GoodMarket]") {
	FarmFixture neutral;
	FarmFixture stressed;
	stressed.province->set_environmental_state(ProvinceEnvironmentalState { fixed_point_t::_0_50 });
	auto normal = neutral.run();
	auto dry = stressed.run();
	CHECK(normal.supply == fixed_point_t { 8 });
	CHECK(normal.traded == normal.supply);
	CHECK(dry.supply == dry.output);
	CHECK(dry.traded == dry.output);
	CHECK(dry.bought == dry.output);
	CHECK(dry.supply < normal.supply);
	CHECK(dry.traded < normal.traded);
	CHECK(dry.revenue > fixed_point_t::_0);
	CHECK(dry.revenue < normal.revenue);
}

TEST_CASE("004A1 mines and unclassified RGOs ignore agricultural stress", "[convergence][004a1][economy][rgo]") {
	for (bool mine : { false, true }) {
		FarmFixture neutral { false, mine };
		FarmFixture stressed { false, mine };
		stressed.province->set_environmental_state(ProvinceEnvironmentalState { 0 });
		auto normal = neutral.run();
		auto dry = stressed.run();
		CHECK(normal.output == fixed_point_t { 8 });
		CHECK(dry == normal);
		CHECK_FALSE(dry.facts.has_value());
	}
}

TEST_CASE("004A1 environmental bounds cannot create negative or overflowing output", "[convergence][004a1][economy][rgo]") {
	for (fixed_point_t water : { fixed_point_t::min, fixed_point_t { -1 }, fixed_point_t::_0,
		fixed_point_t::epsilon, fixed_point_t::_0_50, fixed_point_t::_1, fixed_point_t { 2 }, fixed_point_t::max }) {
		ProvinceEnvironmentalState environment { water };
		for (fixed_point_t baseline : { fixed_point_t::min, fixed_point_t::_0, fixed_point_t::epsilon,
			fixed_point_t { 8 }, fixed_point_t::usable_max, fixed_point_t::max }) {
			auto result = constrain_agricultural_production(environment, baseline);
			CHECK(result.yield_factor >= fixed_point_t::_0);
			CHECK(result.yield_factor <= fixed_point_t::_1);
			CHECK(result.constrained_output >= fixed_point_t::_0);
			CHECK(result.constrained_output <= std::max(baseline, fixed_point_t::_0));
		}
		FarmFixture fixture;
		fixture.province->set_environmental_state(environment);
		auto result = fixture.run();
		CHECK(result.output >= fixed_point_t::_0);
		CHECK(result.output <= fixed_point_t { 8 });
		CHECK(result.supply == result.output);
	}
	CHECK(ProvinceEnvironmentalState { fixed_point_t::min }.get_water_availability() == fixed_point_t::_0);
	CHECK(ProvinceEnvironmentalState { fixed_point_t::max }.get_water_availability() == fixed_point_t::_1);
}

TEST_CASE("004A1 identical environmental state gives identical native outcomes", "[convergence][004a1][economy][rgo][determinism]") {
	for (fixed_point_t water : { fixed_point_t::_0, fixed_point_t::_0_50, fixed_point_t::_1 }) {
		FarmFixture first;
		FarmFixture second;
		first.province->set_environmental_state(ProvinceEnvironmentalState { water });
		second.province->set_environmental_state(ProvinceEnvironmentalState { water });
		CHECK(first.run() == second.run());
	}
}

TEST_CASE("004A1 causal facts are snapshots and clear for inactive RGOs", "[convergence][004a1][economy][rgo]") {
	FarmFixture fixture;
	auto normal = fixture.run();
	fixture.province->set_environmental_state(ProvinceEnvironmentalState { 0 });
	auto dry = fixture.run(false);
	REQUIRE(normal.facts.has_value());
	REQUIRE(dry.facts.has_value());
	CHECK(normal.facts->environment == ProvinceEnvironmentalState {});
	CHECK(dry.facts->yield_factor == fixed_point_t::_0);
	CHECK(dry.facts->unconstrained_output == normal.output);
	CHECK(dry.output == fixed_point_t::_0);
	fixture.province->get_mutable_rgo().set_production_type_nullable(nullptr);
	CHECK_FALSE(fixture.run(false).facts.has_value());
}
