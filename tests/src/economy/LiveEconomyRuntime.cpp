#include "openvic-simulation/economy/LiveEconomyRuntime.hpp"
#include "openvic-simulation/core/simulation/SimulationTimeline.hpp"

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
#include "openvic-simulation/map/MapDefinition.hpp"
#include "openvic-simulation/map/MapInstance.hpp"
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
		GoodDefinition const* feedstock_b = nullptr;
		GoodDefinition const* intermediate = nullptr;
		GoodDefinition const* final_good = nullptr;

		std::optional<ProductionType> upstream_process;
		std::optional<ProductionType> downstream_process;

		LiveEconomyFixture(
			memory::vector<Job> jobs = {},
			pop_size_t workforce = pop_size_t { 0 },
			ProductionType::template_type_t type =
				ProductionType::template_type_t::FACTORY,
			bool include_secondary_feedstock = false
		) {
			REQUIRE(definitions.add_good_category("live", 4));
			category = definitions.get_good_category_by_identifier("live");
			REQUIRE(category != nullptr);

			REQUIRE(definitions.add_good_definition(
				"feedstock", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
				fixed_point_t::_1, true, true, false, false
			));
			REQUIRE(definitions.add_good_definition(
				"feedstock_b", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
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
			feedstock_b = definitions.get_good_definition_by_identifier("feedstock_b");
			intermediate = definitions.get_good_definition_by_identifier("intermediate");
			final_good = definitions.get_good_definition_by_identifier("final");

			REQUIRE(feedstock != nullptr);
			REQUIRE(feedstock_b != nullptr);
			REQUIRE(intermediate != nullptr);
			REQUIRE(final_good != nullptr);

			fixed_point_map_t<GoodDefinition const*> upstream_inputs;
			upstream_inputs.emplace(feedstock, fixed_point_t::_1);
			if (include_secondary_feedstock) {
				upstream_inputs.emplace(feedstock_b, fixed_point_t { 2 });
			}

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

		Pop make_pop(PopType const& type, int size, size_t id, ProvinceInstance* location = nullptr) {
			struct InitialPop : PopBase {
				InitialPop(PopType const& type, Culture const& culture, Religion const& religion, int size)
					: PopBase { type, culture, religion, pop_size_t { size }, 0, 0, nullptr } {}
			};
			return Pop { location != nullptr ? *location : province, InitialPop { type, culture, religion, size },
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


namespace {
struct TestEmployer {
fixed_point_t assigned = 0;

static bool accepts(void*, Pop const&) {
return true;
}

static pop_size_t assign(void* self, Pop& pop, pop_size_t requested) {
auto& employer = *static_cast<TestEmployer*>(self);

pop_size_t const available = pop.get_unemployed();
pop_size_t const actual = std::min(requested, available);

if (actual <= 0) {
return pop_size_t { 0 };
}

pop.hire(actual);
employer.assigned += fixed_point_t { type_safe::get(actual) };
return actual;
}

WorkforceEmployerRequest request(
std::string_view id,
fixed_point_t labor_offer,
int workers
) {
return WorkforceEmployerRequest {
.employer_id = id,
.labor_offer = labor_offer,
.requested = fixed_point_t { workers },
.employer = this,
.accepts = &TestEmployer::accepts,
.assign = &TestEmployer::assign
};
}
};
}

TEST_CASE(
"Competing employer allocation never double employs POPs",
"[economy][native-workforce][competition]"
) {
LiveEconomyFixture fixture {
workforce_jobs(),
pop_size_t { 10 },
ProductionType::template_type_t::PROCESS
};

GoodInstanceManager goods { fixture.definitions, fixture.rules };
WorkforcePopFixture population { fixture.rules, goods };

std::array pops {
population.make_pop(population.eligible, 100, 1)
};

TestEmployer first;
TestEmployer second;

auto result = allocate_competing_employers(
{
first.request("first", fixed_point_t { 2 }, 80),
second.request("second", fixed_point_t { 1 }, 80)
},
WorkforcePool { std::span<Pop> { pops } }
);

REQUIRE(result.size() == 2);

CHECK(result[0].employer_id == "first");
CHECK(result[0].requested == fixed_point_t { 80 });
CHECK(result[0].allocated == fixed_point_t { 80 });

CHECK(result[1].employer_id == "second");
CHECK(result[1].requested == fixed_point_t { 80 });
CHECK(result[1].allocated == fixed_point_t { 20 });

CHECK(first.assigned == fixed_point_t { 80 });
CHECK(second.assigned == fixed_point_t { 20 });

CHECK(first.assigned + second.assigned == fixed_point_t { 100 });
CHECK(pops[0].get_unemployed() == pop_size_t { 0 });
}

TEST_CASE(
"Competing employer labor offer changes allocation deterministically",
"[economy][native-workforce][competition][labor-offer]"
) {
for (bool const first_has_higher_offer : { true, false }) {
LiveEconomyFixture fixture {
workforce_jobs(),
pop_size_t { 10 },
ProductionType::template_type_t::PROCESS
};

GoodInstanceManager goods { fixture.definitions, fixture.rules };
WorkforcePopFixture population { fixture.rules, goods };

std::array pops {
population.make_pop(population.eligible, 100, 1)
};

TestEmployer first;
TestEmployer second;

auto result = allocate_competing_employers(
{
first.request("first", first_has_higher_offer ? fixed_point_t { 2 } : fixed_point_t { 1 }, 80),
second.request("second", first_has_higher_offer ? fixed_point_t { 1 } : fixed_point_t { 2 }, 80)
},
WorkforcePool { std::span<Pop> { pops } }
);

REQUIRE(result.size() == 2);

if (first_has_higher_offer) {
CHECK(first.assigned == fixed_point_t { 80 });
CHECK(second.assigned == fixed_point_t { 20 });
} else {
CHECK(first.assigned == fixed_point_t { 20 });
CHECK(second.assigned == fixed_point_t { 80 });
}

CHECK(first.assigned + second.assigned == fixed_point_t { 100 });
CHECK(pops[0].get_unemployed() == pop_size_t { 0 });
}
}

TEST_CASE(
"Equal-offer competing employers use stable employer identity",
"[economy][native-workforce][competition][labor-offer][determinism]"
) {
for (bool const reverse_input_order : { false, true }) {
LiveEconomyFixture fixture {
workforce_jobs(),
pop_size_t { 10 },
ProductionType::template_type_t::PROCESS
};

GoodInstanceManager goods { fixture.definitions, fixture.rules };
WorkforcePopFixture population { fixture.rules, goods };

std::array pops {
population.make_pop(population.eligible, 100, 1)
};

TestEmployer alpha;
TestEmployer beta;

std::vector<WorkforceEmployerRequest> requests;

if (reverse_input_order) {
requests.push_back(beta.request("beta", fixed_point_t { 1 }, 80));
requests.push_back(alpha.request("alpha", fixed_point_t { 1 }, 80));
} else {
requests.push_back(alpha.request("alpha", fixed_point_t { 1 }, 80));
requests.push_back(beta.request("beta", fixed_point_t { 1 }, 80));
}

auto result = allocate_competing_employers(
std::move(requests),
WorkforcePool { std::span<Pop> { pops } }
);

REQUIRE(result.size() == 2);

CHECK(result[0].employer_id == "alpha");
CHECK(result[1].employer_id == "beta");

CHECK(alpha.assigned == fixed_point_t { 80 });
CHECK(beta.assigned == fixed_point_t { 20 });

CHECK(alpha.assigned + beta.assigned == fixed_point_t { 100 });
CHECK(pops[0].get_unemployed() == pop_size_t { 0 });
}
}
namespace {
	struct CadencedEconomyFixture {
		LiveEconomyFixture definitions;
		GoodInstanceManager goods { definitions.definitions, definitions.rules };
		LiveEconomyScenarioDefinition scenario = definitions.make_scenario();
		LiveEconomyRuntime runtime { definitions.rules, goods, scenario };
		SimulationTimeline timeline;
		std::vector<SimTime> due_times;
		uint64_t market_clearings = 0;

		void advance(int64_t ticks) {
			SimTime const previous = timeline.current_time();
			REQUIRE(timeline.advance(ticks));
			runtime.run_due_daily_cycles(previous, timeline.current_time(),
				[&](SimTime due) -> std::optional<std::span<Pop>> {
					// The preceding cycle must have finished before the next begins.
					CHECK(runtime.get_status().completed_daily_ticks == market_clearings);
					due_times.push_back(due);
					return std::nullopt;
				},
				[&]() {
					CHECK(runtime.get_status().completed_daily_ticks == market_clearings);
					CHECK(runtime.get_status().upstream_output == fixed_point_t { 4 });
					++market_clearings;
					execute_intermediate_market(goods.get_good_instance_by_definition(*definitions.intermediate));
				}
			);
			CHECK(runtime.get_status().completed_daily_ticks == market_clearings);
		}
	};
}

TEST_CASE("Economy cadence waits for the boundary and executes exactly once",
	"[economy][cadence-authority]") {
	CadencedEconomyFixture fixture;
	fixture.advance(0);
	fixture.advance(23);
	CHECK(fixture.market_clearings == 0);
	CHECK(fixture.due_times.empty());
	CHECK(fixture.runtime.get_status().completed_daily_ticks == 0);
	CHECK(fixture.runtime.get_status().upstream_output == fixed_point_t::_0);
	CHECK(fixture.runtime.get_status().final_inventory == fixed_point_t::_0);
	fixture.advance(1);
	CHECK(fixture.market_clearings == 1);
	CHECK(fixture.due_times == std::vector<SimTime> { SimTime { 24 } });
	CHECK(fixture.runtime.get_status().completed_daily_ticks == 1);
	CHECK(fixture.runtime.get_status().intermediate_quantity_traded_yesterday == fixed_point_t { 4 });
	CHECK(fixture.runtime.get_status().final_inventory == fixed_point_t { 2 });
	fixture.advance(0);
	fixture.advance(23);
	CHECK(fixture.market_clearings == 1);
	CHECK(fixture.runtime.get_status().final_inventory == fixed_point_t { 2 });
	fixture.advance(1);
	CHECK(fixture.market_clearings == 2);
	CHECK(fixture.runtime.get_status().completed_daily_ticks == 2);
	CHECK(fixture.runtime.get_status().final_inventory == fixed_point_t { 4 });
}

TEST_CASE("Economy cadence is invariant to time advance partitioning",
	"[economy][cadence-authority][determinism]") {
	CadencedEconomyFixture whole;
	whole.advance(72);
	REQUIRE(whole.market_clearings == 3);
	CHECK(whole.due_times == std::vector<SimTime> { SimTime { 24 }, SimTime { 48 }, SimTime { 72 } });
	for (auto const& steps : std::vector<std::vector<int64_t>> {
		{ 24, 24, 24 }, { 5, 19, 25, 0, 23 }, { 25, 47 }, { 1, 70, 1 }
	}) {
		CadencedEconomyFixture split;
		// Periodic dispatch must not consume exceptional scheduled work.
		REQUIRE(split.timeline.schedule_event(SimTime { 24 }, { "first", {} }).has_value());
		REQUIRE(split.timeline.schedule_event(SimTime { 24 }, { "second", {} }).has_value());
		for (int64_t step : steps) {
			split.advance(step);
		}
		CHECK(split.timeline.current_time() == whole.timeline.current_time());
		CHECK(split.due_times == whole.due_times);
		CHECK(split.market_clearings == whole.market_clearings);
		auto const expected = whole.runtime.get_status();
		auto const actual = split.runtime.get_status();
		CHECK(actual.completed_daily_ticks == expected.completed_daily_ticks);
		CHECK(actual.upstream_output == expected.upstream_output);
		CHECK(actual.downstream_output == expected.downstream_output);
		CHECK(actual.downstream_desired_output == expected.downstream_desired_output);
		CHECK(actual.downstream_input_limited == expected.downstream_input_limited);
		CHECK(actual.source_buffer_inventory == expected.source_buffer_inventory);
		CHECK(actual.source_unmet_inflow == expected.source_unmet_inflow);
		CHECK(actual.intermediate_upstream_inventory == expected.intermediate_upstream_inventory);
		CHECK(actual.intermediate_downstream_inventory == expected.intermediate_downstream_inventory);
		CHECK(actual.final_inventory == expected.final_inventory);
		CHECK(actual.final_inventory == fixed_point_t { 6 });
		CHECK(actual.deliverable_intermediate == expected.deliverable_intermediate);
		CHECK(actual.intermediate_price == expected.intermediate_price);
		CHECK(actual.intermediate_supply_yesterday == expected.intermediate_supply_yesterday);
		CHECK(actual.intermediate_demand_yesterday == expected.intermediate_demand_yesterday);
		CHECK(actual.intermediate_quantity_traded_yesterday == expected.intermediate_quantity_traded_yesterday);
		auto first = split.timeline.pop_due_event();
		auto second = split.timeline.pop_due_event();
		REQUIRE(first.has_value());
		REQUIRE(second.has_value());
		CHECK(first->payload.type_id == "first");
		CHECK(second->payload.type_id == "second");
		CHECK(first->sequence < second->sequence);
		CHECK_FALSE(split.timeline.pop_due_event().has_value());
	}
}

TEST_CASE("Cadence prepares real POP availability before allocation and clearing",
	"[economy][cadence-authority][native-workforce]") {
	for (int const already_employed : { 0, 20 }) {
		LiveEconomyFixture fixture {
			workforce_jobs(), pop_size_t { 10 }, ProductionType::template_type_t::PROCESS
		};
		GoodInstanceManager goods { fixture.definitions, fixture.rules };
		WorkforcePopFixture population { fixture.rules, goods };
		std::array pops { population.make_pop(population.eligible, 40, 1) };
		auto scenario = fixture.make_scenario();
		LiveEconomyRuntime runtime { fixture.rules, goods, scenario };
		SimulationTimeline timeline;
		uint64_t prepared = 0;
		uint64_t cleared = 0;
		auto prepare = [&](SimTime due) -> std::optional<std::span<Pop>> {
			CHECK(due == SimTime { 24 });
			CHECK(cleared == 0);
			++prepared;
			// Fresh real POP state starts unemployed. Existing employers take
			// their share before this cycle's workforce allocation runs.
			if (already_employed > 0) {
				pops[0].hire(pop_size_t { already_employed });
			}
			return std::span<Pop> { pops };
		};
		auto clear = [&]() {
			CHECK(prepared == 1);
			CHECK(pops[0].get_unemployed() == pop_size_t { 0 });
			CHECK(runtime.get_status().completed_daily_ticks == 0);
			++cleared;
			execute_intermediate_market(goods.get_good_instance_by_definition(*fixture.intermediate));
		};
		REQUIRE(timeline.advance(23));
		runtime.run_due_daily_cycles(SimTime { 0 }, timeline.current_time(), prepare, clear);
		CHECK(prepared == 0);
		CHECK(cleared == 0);
		CHECK(pops[0].get_unemployed() == pop_size_t { 40 });
		REQUIRE(timeline.advance(1));
		runtime.run_due_daily_cycles(SimTime { 23 }, timeline.current_time(), prepare, clear);
		CHECK(prepared == 1);
		CHECK(cleared == 1);
		auto const status = runtime.get_status();
		int const available = 40 - already_employed;
		CHECK(status.completed_daily_ticks == 1);
		CHECK(status.upstream_output == fixed_point_t { available / 10 });
		CHECK(status.intermediate_supply_yesterday == fixed_point_t { available / 10 });
		CHECK(status.intermediate_quantity_traded_yesterday == fixed_point_t { available / 10 });
		CHECK(status.downstream_output == fixed_point_t { available / 20 });
	}
}

namespace {
	struct ProvenanceRun {
		LiveEconomyStatus status;
		std::optional<LiveEconomyCycleProvenance> provenance;
		pop_size_t unemployed;
		uint64_t clearings;
		bool operator==(ProvenanceRun const&) const = default;
	};

	ProvenanceRun run_provenance_case(
		int workers, int transport_capacity, fixed_point_t source_availability, bool record = true
	) {
		LiveEconomyFixture fixture {
			workforce_jobs(), pop_size_t { 10 }, ProductionType::template_type_t::PROCESS
		};
		GoodInstanceManager goods { fixture.definitions, fixture.rules };
		WorkforcePopFixture population { fixture.rules, goods };
		std::array pops { population.make_pop(population.eligible, 40, 1) };
		if (workers < 40) {
			pops[0].hire(pop_size_t { 40 - workers });
		}
		auto scenario = fixture.make_scenario();
		scenario.corridor_legs[0].nominal_capacity = fixed_point_t { transport_capacity };
		LiveEconomyRuntime runtime { fixture.rules, goods, scenario, record };
		REQUIRE(runtime.set_source_resource_availability(source_availability));
		CHECK_FALSE(runtime.get_latest_provenance().has_value());
		uint64_t clearings = 0;
		SimulationTimeline timeline;
		REQUIRE(timeline.advance(24));
		runtime.run_due_daily_cycles(SimTime { 0 }, timeline.current_time(),
			[&](SimTime) -> std::optional<std::span<Pop>> { return std::span<Pop> { pops }; },
			[&]() {
				// A partial cycle must not escape through the completed-cycle API.
				CHECK_FALSE(runtime.get_latest_provenance().has_value());
				++clearings;
				execute_intermediate_market(goods.get_good_instance_by_definition(*fixture.intermediate));
			}
		);
		return { runtime.get_status(), runtime.get_latest_provenance(), pops[0].get_unemployed(), clearings };
	}
}

TEST_CASE("Provenance distinguishes labor shortage from an equally costly delivery shortage",
	"[economy][provenance][native-workforce]") {
	auto full = run_provenance_case(40, 4, fixed_point_t::_1);
	auto labor = run_provenance_case(20, 4, fixed_point_t::_1);
	auto logistics = run_provenance_case(40, 2, fixed_point_t::_1);
	REQUIRE(full.provenance.has_value());
	REQUIRE(labor.provenance.has_value());
	REQUIRE(logistics.provenance.has_value());
	auto const& f = *full.provenance;
	auto const& l = *labor.provenance;
	auto const& t = *logistics.provenance;
	CHECK(f.due_time == std::optional<SimTime> { SimTime { 24 } });
	CHECK(f.cycle == 1);
	CHECK(f.upstream_process_id == "live_upstream");
	CHECK(f.intermediate_good_id == "intermediate");
	REQUIRE(f.workforce.has_value());
	REQUIRE(l.workforce.has_value());
	CHECK(f.workforce->requested == fixed_point_t { 40 });
	CHECK(f.workforce->allocated == fixed_point_t { 40 });
	CHECK(l.workforce->allocated == fixed_point_t { 20 });
	CHECK_FALSE(f.upstream.labor_limited);
	CHECK(f.upstream.installed_ceiling_active);
	CHECK(l.upstream.labor_limited);
	CHECK_FALSE(l.upstream.installed_ceiling_active);
	CHECK(l.upstream.potential_output == fixed_point_t { 4 });
	CHECK(l.upstream.desired_output == fixed_point_t { 2 });
	CHECK(l.upstream.actual_output == fixed_point_t { 2 });
	CHECK(l.upstream.labor_supported_capacity == fixed_point_t { 2 });
	CHECK_FALSE(l.upstream.input_limited);
	CHECK_FALSE(l.logistics.is_delivery_limited());
	CHECK(l.logistics.physical_supply == fixed_point_t { 2 });
	CHECK(l.logistics.calculate_deliverable_quantity() == fixed_point_t { 2 });
	CHECK_FALSE(t.upstream.labor_limited);
	CHECK(t.upstream.actual_output == fixed_point_t { 4 });
	CHECK(t.logistics.physical_supply == fixed_point_t { 4 });
	CHECK(t.logistics.calculate_deliverable_quantity() == fixed_point_t { 2 });
	CHECK(t.logistics.is_delivery_limited());
	CHECK(l.downstream.actual_output == t.downstream.actual_output);
	CHECK(l.downstream.actual_output == fixed_point_t { 1 });
	CHECK(l.downstream.input_limited);
	CHECK(t.downstream.input_limited);
	CHECK(f.downstream.actual_output == fixed_point_t { 2 });
	CHECK(f.upstream_market.output_offered == fixed_point_t { 4 });
	CHECK(f.upstream_market.output_sold == fixed_point_t { 4 });
	CHECK(l.upstream_market.output_offered == fixed_point_t { 2 });
	CHECK(l.upstream_market.output_sold == fixed_point_t { 2 });
	CHECK(t.upstream_market.output_offered == fixed_point_t { 4 });
	CHECK(t.upstream_market.output_sold == fixed_point_t { 2 });
	CHECK(t.downstream_market.input_requested == fixed_point_t { 8 });
	CHECK(t.downstream_market.input_ordered == fixed_point_t { 2 });
	CHECK(t.downstream_market.input_bought == fixed_point_t { 2 });
	CHECK_FALSE(t.downstream_market.transaction_limited());
	CHECK_FALSE(l.downstream_market.transaction_limited());
}

TEST_CASE("Provenance records resource shortfall and simultaneous production ceilings",
	"[economy][provenance][resources]") {
	auto resource = run_provenance_case(40, 4, fixed_point_t::_0_50);
	REQUIRE(resource.provenance.has_value());
	auto const& p = *resource.provenance;
	CHECK(p.resource.requested == fixed_point_t { 4 });
	CHECK(p.resource.nominal_supply == fixed_point_t { 4 });
	CHECK(p.resource.physical_supply == fixed_point_t { 2 });
	CHECK(p.resource.accessible_supply == fixed_point_t { 2 });
	CHECK(p.resource.delivered == fixed_point_t { 2 });
	CHECK(p.resource.buffer_draw == fixed_point_t::_0);
	CHECK(p.resource.unmet == fixed_point_t { 2 });
	CHECK(p.resource.source_availability_limited);
	CHECK_FALSE(p.resource.source_access_limited);
	CHECK(p.upstream.input_limited);
	CHECK(p.upstream.input_below_potential);
	CHECK_FALSE(p.upstream.labor_limited);
	CHECK(p.upstream.actual_output == fixed_point_t { 2 });
	CHECK(p.downstream.input_limited);
	CHECK(p.downstream.actual_output == fixed_point_t { 1 });
	auto combined = run_provenance_case(20, 4, fixed_point_t::_0_50);
	REQUIRE(combined.provenance.has_value());
	CHECK(combined.provenance->upstream.labor_limited);
	CHECK(combined.provenance->upstream.input_below_potential);
	CHECK(combined.provenance->upstream.input_supported_output == std::optional<fixed_point_t> { 2 });
	// Equal labor/input ceilings are both retained, although the input pass
	// did not further reduce the already labor-limited desired output.
	CHECK_FALSE(combined.provenance->upstream.input_limited);
}

TEST_CASE("Provenance recording is observational and deterministic",
	"[economy][provenance][determinism]") {
	for (int workers : { 20, 40 }) {
		for (int transport : { 2, 4 }) {
			for (fixed_point_t availability : { fixed_point_t::_0_50, fixed_point_t::_1 }) {
				auto recorded = run_provenance_case(workers, transport, availability);
				auto unrecorded = run_provenance_case(workers, transport, availability, false);
				auto repeated = run_provenance_case(workers, transport, availability);
				CHECK(recorded == repeated);
				CHECK(recorded.status == unrecorded.status); // Includes price, inventories and trades.
				CHECK(recorded.unemployed == unrecorded.unemployed);
				CHECK(recorded.unemployed == pop_size_t { 0 });
				CHECK(recorded.clearings == unrecorded.clearings);
				CHECK(recorded.clearings == 1);
				CHECK(recorded.status.completed_daily_ticks == 1);
				CHECK_FALSE(unrecorded.provenance.has_value());
			}
		}
	}
}

TEST_CASE("Provenance retains only the completed cycle and respects cadence partitions",
	"[economy][provenance][cadence-authority]") {
	CadencedEconomyFixture whole;
	CadencedEconomyFixture split;
	whole.advance(72);
	split.advance(23);
	CHECK_FALSE(split.runtime.get_latest_provenance().has_value());
	split.advance(1);
	auto first = split.runtime.get_latest_provenance();
	REQUIRE(first.has_value());
	CHECK(first->cycle == 1);
	split.advance(25);
	split.advance(23);
	CHECK(split.runtime.get_latest_provenance() == whole.runtime.get_latest_provenance());
	REQUIRE(split.runtime.get_latest_provenance().has_value());
	CHECK(split.runtime.get_latest_provenance()->cycle == 3);
	CHECK(split.runtime.get_latest_provenance()->due_time == std::optional<SimTime> { SimTime { 72 } });
	CHECK_FALSE(split.runtime.get_latest_provenance()->workforce.has_value());
	CHECK_FALSE(split.runtime.get_latest_provenance()->upstream.workforce_enabled);
	CHECK(first->cycle == 1); // Earlier value copies remain immutable.
	auto latest = split.runtime.get_latest_provenance();
	split.runtime.pre_market_daily_tick();
	CHECK(split.runtime.get_latest_provenance() == latest);
	execute_intermediate_market(split.goods.get_good_instance_by_definition(*split.definitions.intermediate));
	split.runtime.post_market_daily_tick();
	CHECK(split.runtime.get_latest_provenance()->cycle == 4);
	CHECK_FALSE(split.runtime.get_latest_provenance()->due_time.has_value());
}

namespace {
	struct BoundWorldFixture {
		LiveEconomyFixture economy { workforce_jobs(), pop_size_t { 10 }, ProductionType::template_type_t::PROCESS };
		GoodInstanceManager goods { economy.definitions, economy.rules };
		WorkforcePopFixture population { economy.rules, goods };
		MapDefinition definition;
		std::unique_ptr<MapInstance> map;
		LiveEconomyScenarioDefinition scenario = economy.make_scenario();
		LiveEconomyRuntime runtime { economy.rules, goods, scenario };
		SimulationTimeline timeline;
		ProductiveSiteBinding binding { "1", "plant", "live_upstream", LaborPoolScope::ProvinceLocal };
		uint64_t clearings = 0;

		BoundWorldFixture() {
			// province_building_types stores references into the building registry,
                 // so reserve before inserting multiple manually-created types.
                 population.buildings.reserve_more_building_types(2);

                 BuildingType::building_type_args_t primary_args;
                 primary_args.type = "productive_site";
                 primary_args.in_province = true;
                 primary_args.capacity_per_level = 2;
                 primary_args.max_level = building_level_t { 5 };
                 primary_args.production_type = &*economy.upstream_process;

                 BuildingType::building_type_args_t secondary_args;
                 secondary_args.type = "productive_site";
                 secondary_args.in_province = true;
                 secondary_args.capacity_per_level = 2;
                 secondary_args.max_level = building_level_t { 5 };
                 secondary_args.production_type = &*economy.upstream_process;

                 REQUIRE(
                         population.buildings.add_building_type(
                                 "plant",
                                 primary_args
                         )
                 );

                 REQUIRE(
                         population.buildings.add_building_type(
                                 "plant_b",
                                 secondary_args
                         )
                 );

                 population.buildings.lock_building_types();
			REQUIRE(definition.set_max_provinces(province_index_t { 2 }));
			REQUIRE(definition.add_province_definition("1", colour_t { 1, 2, 3 }));
			REQUIRE(definition.add_province_definition("2", colour_t { 4, 5, 6 }));
			definition.lock_province_definitions();
			map = std::make_unique<MapInstance>(definition, ProvinceInstanceDeps {
				population.buildings, economy.rules, population.aggregates, population.rgo_deps, {}
			}, population.threads);
			plant().set_level(building_level_t { 2 });
			replace_population("1", 40);
			replace_population("2", 100);
			REQUIRE(runtime.bind_upstream_site(binding, *map));
		}

		ProvinceInstance& province(std::string_view id) {
			auto* result = map->get_province_instance_by_identifier(id);
			REQUIRE(result != nullptr);
			return *result;
		}
		BuildingInstance& plant() {
			auto* result = province("1").get_mutable_building_by_identifier("plant");
			REQUIRE(result != nullptr);
			return *result;
		}

         BuildingInstance& plant_b() {
                 auto* result =
                         province("1").get_mutable_building_by_identifier("plant_b");
                 REQUIRE(result != nullptr);
                 return *result;
         }
		void replace_population(std::string_view id, int size) {
			auto& location = province(id);
			location.get_mutable_pops().clear();
			location.get_mutable_pops().emplace(population.make_pop(population.eligible, size, 1, &location));
		}
		void cycle() {
                 auto previous = timeline.current_time();
                 REQUIRE(timeline.advance(24));

                 // Match InstanceManager: one prepared POP pool, one province-wide
                 // allocation authority containing RGO + every bound productive site.
                 map->prepare_employment_phase();

                 auto employer_requests =
                         runtime.prepare_upstream_employer_requests(*map);

                 auto allocations =
                         map->allocate_employment_phase(
                                 std::move(employer_requests)
                         );

                 runtime.apply_upstream_employer_allocations(
                         allocations
                 );

                 map->finish_rgo_production();

                 runtime.run_due_daily_cycles(
                         previous,
                         timeline.current_time(),
                         [&](SimTime) -> std::optional<WorkforcePool> {
                                 // Workers have already been committed through Pop::hire().
                                 // A second pool here would constitute a second hiring pass.
                                 return std::nullopt;
                         },
                         [&]() {
                                 ++clearings;
                                 execute_intermediate_market(
                                         goods.get_good_instance_by_definition(
                                                 *economy.intermediate
                                         )
                                 );
                         }
                 );
         }
	};
}

TEST_CASE("World productive site derives capacity and hires from its province",
	"[economy][productive-site][world]") {
	BoundWorldFixture world;
	world.cycle();
	auto p = world.runtime.get_latest_provenance();
	REQUIRE(p.has_value());
	CHECK(p->productive_site == std::optional<ProductiveSiteBinding> { world.binding });
	CHECK(p->due_time == std::optional<SimTime> { SimTime { 24 } });
	CHECK(world.plant().get_level() == building_level_t { 2 });
	CHECK(p->upstream.installed_capacity == fixed_point_t { 4 });
	REQUIRE(p->workforce.has_value());
	CHECK(p->workforce->allocated == fixed_point_t { 40 });
	CHECK(p->upstream.actual_output == fixed_point_t { 4 });
	CHECK(p->upstream_market.output_sold == fixed_point_t { 4 });
	CHECK(p->downstream_market.input_bought == fixed_point_t { 4 });
	CHECK(p->downstream.actual_output == fixed_point_t { 2 });
	CHECK(world.province("1").get_mutable_pops().begin()->get_unemployed() == pop_size_t { 0 });
	CHECK(world.clearings == 1);
}

TEST_CASE("World building level changes the bound producer on the next cadence",
	"[economy][productive-site][capacity]") {
	BoundWorldFixture world;
	world.cycle();
	CHECK(world.runtime.get_status().upstream_output == fixed_point_t { 4 });
	world.plant().set_level(building_level_t { 1 });
	// Replace the day's POP availability in the world, not in the producer.
	world.replace_population("1", 40);
	world.cycle();
	auto p = world.runtime.get_latest_provenance();
	REQUIRE(p.has_value());
	CHECK(p->upstream.installed_capacity == fixed_point_t { 2 });
	REQUIRE(p->workforce.has_value());
	CHECK(p->workforce->requested == fixed_point_t { 20 });
	CHECK(p->workforce->allocated == fixed_point_t { 20 });
	CHECK(p->upstream.actual_output == fixed_point_t { 2 });
	CHECK(p->upstream_market.output_sold == fixed_point_t { 2 });
	CHECK(p->downstream.actual_output == fixed_point_t { 1 });
	CHECK(world.clearings == 2);
}

TEST_CASE("Reduced province population limits output without borrowing unrelated workers",
	"[economy][productive-site][labor][identity]") {
	BoundWorldFixture world;
	world.cycle();
	world.replace_population("1", 20);
	world.cycle();
	auto p = world.runtime.get_latest_provenance();
	REQUIRE(p.has_value());
	CHECK(p->upstream.installed_capacity == fixed_point_t { 4 });
	REQUIRE(p->workforce.has_value());
	CHECK(p->workforce->allocated == fixed_point_t { 20 });
	CHECK(p->upstream.labor_limited);
	CHECK(p->upstream.actual_output == fixed_point_t { 2 });
	CHECK(p->upstream_market.output_sold == fixed_point_t { 2 });
	CHECK(p->downstream.actual_output == fixed_point_t { 1 });
	CHECK(world.province("2").get_mutable_pops().begin()->get_unemployed() == pop_size_t { 100 });
}

TEST_CASE(
"Site binding validates world identity and labor scope",
"[economy][productive-site][validation]"
) {
BoundWorldFixture world;

auto invalid = world.binding;
invalid.province_id = "missing";
CHECK_FALSE(world.runtime.bind_upstream_site(invalid, *world.map));

invalid = world.binding;
invalid.building_id = "missing";
CHECK_FALSE(world.runtime.bind_upstream_site(invalid, *world.map));

invalid = world.binding;
invalid.production_type_id = "live_downstream";
CHECK_FALSE(world.runtime.bind_upstream_site(invalid, *world.map));
CHECK_FALSE(
world.binding.resolve(
*world.map,
*world.economy.downstream_process
).has_value()
);

invalid = world.binding;
invalid.labor_scope = static_cast<LaborPoolScope>(99);
CHECK_FALSE(world.runtime.bind_upstream_site(invalid, *world.map));
}

TEST_CASE(
"World bound producer consumes a shared workforce assignment without rehiring",
"[economy][productive-site][world][competition][b3]"
) {
BoundWorldFixture world;

// Give the real bound province a finite labor supply large enough for
// both employers to request workers but not both to receive everything.
world.replace_population("1", 60);

auto workforce = world.runtime.prepare_upstream_site(*world.map);
REQUIRE(workforce.has_value());

AggregateProducer& producer =
world.runtime.get_upstream_producer_for_workforce_allocation();

TestEmployer competing_employer;

WorkforceEmployerRequest producer_request =
make_producer_workforce_request(
producer,
"site:1:plant",
2
);

WorkforceEmployerRequest competing_request =
competing_employer.request(
"other:1",
1,
40
);

fixed_point_t const requested = producer_request.requested;

auto allocations = allocate_competing_employers(
{
producer_request,
competing_request
},
*workforce
);

REQUIRE(allocations.size() == 2);

fixed_point_t producer_allocated = fixed_point_t::_0;

for (WorkforceEmployerAllocation const& allocation : allocations) {
if (allocation.employer_id == "site:1:plant") {
producer_allocated = allocation.allocated;
}
}

// Building level 2 × capacity-per-level 2 × base workforce 10.
CHECK(requested == fixed_point_t { 40 });
CHECK(producer_allocated == fixed_point_t { 40 });

// Only the remaining 20 workers can go to the competing employer.
CHECK(competing_employer.assigned == fixed_point_t { 20 });

auto& pop = *world.province("1").get_mutable_pops().begin();

CHECK(pop.get_unemployed() == pop_size_t { 0 });

world.runtime.set_preallocated_upstream_workforce(
WorkforceAllocationResult {
.requested = requested,
.allocated = producer_allocated
}
);

SimTime const previous = world.timeline.current_time();
REQUIRE(world.timeline.advance(24));

world.runtime.run_due_daily_cycles(
previous,
world.timeline.current_time(),

// Returning no workforce is deliberate: the authoritative shared
// allocator already hired the workers. The runtime must consume that
// assignment rather than hire them again.
[&](SimTime) -> std::optional<WorkforcePool> {
return std::nullopt;
},

[&]() {
++world.clearings;
execute_intermediate_market(
world.goods.get_good_instance_by_definition(
*world.economy.intermediate
)
);
}
);

auto provenance = world.runtime.get_latest_provenance();
REQUIRE(provenance.has_value());
REQUIRE(provenance->workforce.has_value());

CHECK(provenance->workforce->requested == fixed_point_t { 40 });
CHECK(provenance->workforce->allocated == fixed_point_t { 40 });

// The producer retained the shared assignment and produced at full
// installed capacity without a second Pop::hire pass.
CHECK(provenance->upstream.installed_capacity == fixed_point_t { 4 });
CHECK(provenance->upstream.actual_output == fixed_point_t { 4 });
CHECK(provenance->upstream_market.output_sold == fixed_point_t { 4 });
CHECK(provenance->downstream.actual_output == fixed_point_t { 2 });

CHECK(pop.get_unemployed() == pop_size_t { 0 });
CHECK(world.clearings == 1);
}

TEST_CASE(
"Realized producer economics change the next labor allocation",
"[economy][productive-site][labor-offer][feedback][b4b]"
) {
BoundWorldFixture world;

// First establish a completed profitable production cycle using real
// production, inventory, market execution, and authoritative workforce.
world.replace_population("1", 40);
world.cycle();

fixed_point_t const high_offer =
world.runtime.get_upstream_labor_offer();

REQUIRE(high_offer > fixed_point_t::_0);

auto high_provenance = world.runtime.get_latest_provenance();
REQUIRE(high_provenance.has_value());
REQUIRE(high_provenance->workforce.has_value());
CHECK(high_provenance->workforce->allocated == fixed_point_t { 40 });
CHECK(high_provenance->upstream_market.output_sold == fixed_point_t { 4 });
CHECK(high_provenance->upstream_market.money_received > fixed_point_t::_0);

// Remove the real upstream resource. The producer still receives workers
// for this completed cycle, but cannot create saleable output. Therefore
// its observed economic capacity per worker falls to zero.
REQUIRE(
world.runtime.set_source_resource_availability(fixed_point_t::_0)
);

world.replace_population("1", 40);
world.cycle();

fixed_point_t const low_offer =
world.runtime.get_upstream_labor_offer();

auto low_provenance = world.runtime.get_latest_provenance();
REQUIRE(low_provenance.has_value());
REQUIRE(low_provenance->workforce.has_value());

CHECK(low_provenance->workforce->allocated == fixed_point_t { 40 });
CHECK(low_provenance->upstream.actual_output == fixed_point_t::_0);
CHECK(low_provenance->upstream_market.output_sold == fixed_point_t::_0);
CHECK(low_provenance->upstream_market.money_received == fixed_point_t::_0);
CHECK(low_offer == fixed_point_t::_0);
CHECK(high_offer > low_offer);

// Choose one competing offer strictly between the producer's observed
// high and low economic offers. Nothing about the competitor changes.
fixed_point_t const competing_offer =
high_offer / fixed_point_t { 2 };

REQUIRE(competing_offer > low_offer);
REQUIRE(high_offer > competing_offer);

// ------------------------------------------------------------
// Allocation using the producer's HIGH realized economic offer.
// Producer should be first and receive its entire 40-worker demand.
// ------------------------------------------------------------

world.replace_population("1", 60);

auto high_workforce =
world.runtime.prepare_upstream_site(*world.map);

REQUIRE(high_workforce.has_value());

AggregateProducer& producer =
world.runtime.get_upstream_producer_for_workforce_allocation();

TestEmployer high_competitor;

WorkforceEmployerRequest high_producer_request =
make_producer_workforce_request(
producer,
"site:1:plant",
high_offer
);

WorkforceEmployerRequest high_competing_request =
high_competitor.request(
"other:1",
competing_offer,
40
);

auto high_allocations = allocate_competing_employers(
{
high_producer_request,
high_competing_request
},
*high_workforce
);

REQUIRE(high_allocations.size() == 2);

fixed_point_t high_producer_allocated = fixed_point_t::_0;

for (WorkforceEmployerAllocation const& allocation : high_allocations) {
if (allocation.employer_id == "site:1:plant") {
high_producer_allocated = allocation.allocated;
}
}

CHECK(high_producer_allocated == fixed_point_t { 40 });
CHECK(high_competitor.assigned == fixed_point_t { 20 });
CHECK(
world.province("1").get_mutable_pops().begin()->get_unemployed()
== pop_size_t { 0 }
);

// ------------------------------------------------------------
// Allocation using the producer's LOW realized economic offer.
// The unchanged competitor is now preferred, so it receives 40
// and only the remaining 20 can reach the producer.
// ------------------------------------------------------------

world.replace_population("1", 60);

auto low_workforce =
world.runtime.prepare_upstream_site(*world.map);

REQUIRE(low_workforce.has_value());

TestEmployer low_competitor;

WorkforceEmployerRequest low_producer_request =
make_producer_workforce_request(
producer,
"site:1:plant",
low_offer
);

WorkforceEmployerRequest low_competing_request =
low_competitor.request(
"other:1",
competing_offer,
40
);

auto low_allocations = allocate_competing_employers(
{
low_producer_request,
low_competing_request
},
*low_workforce
);

REQUIRE(low_allocations.size() == 2);

fixed_point_t low_producer_allocated = fixed_point_t::_0;

for (WorkforceEmployerAllocation const& allocation : low_allocations) {
if (allocation.employer_id == "site:1:plant") {
low_producer_allocated = allocation.allocated;
}
}

CHECK(low_competitor.assigned == fixed_point_t { 40 });
CHECK(low_producer_allocated == fixed_point_t { 20 });

// Same finite authoritative POP supply in both allocations.
// No employer created workers and no worker was hired twice.
CHECK(
world.province("1").get_mutable_pops().begin()->get_unemployed()
== pop_size_t { 0 }
);
CHECK(
low_competitor.assigned + low_producer_allocated
== fixed_point_t { 60 }
);
}


TEST_CASE(
"Map employment authority allocates RGO and multiple external employers deterministically",
"[economy][employment-authority][multi-employer][determinism]"
) {
auto run = [](bool reverse_external_order) {
BoundWorldFixture world;

// One authoritative province-local labor pool.
world.replace_population("1", 60);

// Employment preparation resets all POP jobs and prepares the RGO before
// the province-wide allocation authority runs.
world.map->prepare_employment_phase();

TestEmployer employer_a;
TestEmployer employer_b;

// Both external employers request more labor than can be jointly satisfied.
// Distinct offers make the expected ranking explicit.
WorkforceEmployerRequest request_a =
employer_a.request(
"site:1:a",
fixed_point_t { 3 },
40
);

WorkforceEmployerRequest request_b =
employer_b.request(
"site:1:b",
fixed_point_t { 2 },
40
);

std::vector<WorkforceEmployerRequest> external_requests;

if (reverse_external_order) {
external_requests = { request_b, request_a };
} else {
external_requests = { request_a, request_b };
}

auto allocations = world.map->allocate_employment_phase(
{
ProvinceWorkforceEmployerRequests {
.province_id = "1",
.employers = std::move(external_requests)
}
}
);

fixed_point_t allocated_a = fixed_point_t::_0;
fixed_point_t allocated_b = fixed_point_t::_0;

for (WorkforceEmployerAllocation const& allocation : allocations) {
if (allocation.employer_id == "site:1:a") {
allocated_a = allocation.allocated;
} else if (allocation.employer_id == "site:1:b") {
allocated_b = allocation.allocated;
}
}

auto& pop = *world.province("1").get_mutable_pops().begin();

struct Result final {
fixed_point_t allocated_a;
fixed_point_t allocated_b;
fixed_point_t assigned_a;
fixed_point_t assigned_b;
pop_size_t unemployed;

bool operator==(Result const&) const = default;
};

return Result {
.allocated_a = allocated_a,
.allocated_b = allocated_b,
.assigned_a = employer_a.assigned,
.assigned_b = employer_b.assigned,
.unemployed = pop.get_unemployed()
};
};

auto forward = run(false);
auto reverse = run(true);

// Employer A has the stronger offer and receives its entire request.
// Employer B receives only the remaining 20 workers.
// The RGO participates in the same authority but cannot create extra labor.
CHECK(forward.allocated_a == fixed_point_t { 40 });
CHECK(forward.assigned_a == fixed_point_t { 40 });

CHECK(forward.allocated_b == fixed_point_t { 20 });
CHECK(forward.assigned_b == fixed_point_t { 20 });

// All 60 real workers are exhausted exactly once.
CHECK(forward.unemployed == pop_size_t { 0 });
CHECK(
forward.assigned_a + forward.assigned_b
== fixed_point_t { 60 }
);

// Collection order is not authoritative; labor offer + employer identity are.
CHECK(reverse == forward);
}


TEST_CASE(
"Real bound productive sites share one province workforce and market deterministically",
"[economy][productive-site][world][multi-site][employment-authority][determinism]"
) {
        struct Result final {
                fixed_point_t primary_allocated = fixed_point_t::_0;
                fixed_point_t secondary_allocated = fixed_point_t::_0;
                fixed_point_t primary_output = fixed_point_t::_0;
                fixed_point_t secondary_output = fixed_point_t::_0;
                fixed_point_t primary_sold = fixed_point_t::_0;
                fixed_point_t secondary_sold = fixed_point_t::_0;
                fixed_point_t total_upstream_output = fixed_point_t::_0;
                pop_size_t unemployed = 0;
                uint64_t clearings = 0;

                bool operator==(Result const&) const = default;
        };

        auto run = [](bool bind_secondary_first) {
                BoundWorldFixture world;

                world.plant().set_level(building_level_t { 2 });
                world.plant_b().set_level(building_level_t { 2 });
                world.replace_population("1", 60);

                ProductiveSiteBinding secondary {
                        "1",
                        "plant_b",
                        "live_upstream",
                        LaborPoolScope::ProvinceLocal
                };

                // The primary was bound by the fixture constructor. To genuinely
                // test opposite binding-call order, rebuild the runtime against the
                // same world when requested.
                if (bind_secondary_first) {
                        LiveEconomyRuntime reordered_runtime {
                                world.economy.rules,
                                world.goods,
                                world.scenario
                        };

                        REQUIRE(
                                reordered_runtime.bind_additional_upstream_site(
                                        secondary,
                                        *world.map
                                )
                        );

                        REQUIRE(
                                reordered_runtime.bind_upstream_site(
                                        world.binding,
                                        *world.map
                                )
                        );

                        world.map->prepare_employment_phase();

                        auto requests =
                                reordered_runtime.prepare_upstream_employer_requests(
                                        *world.map
                                );

                        auto allocations =
                                world.map->allocate_employment_phase(
                                        std::move(requests)
                                );

                        reordered_runtime.apply_upstream_employer_allocations(
                                allocations
                        );

                        world.map->finish_rgo_production();

                        SimulationTimeline timeline;
                        REQUIRE(timeline.advance(24));

                        uint64_t clearings = 0;

                        reordered_runtime.run_due_daily_cycles(
                                SimTime { 0 },
                                timeline.current_time(),
                                [&](SimTime) -> std::optional<WorkforcePool> {
                                        return std::nullopt;
                                },
                                [&]() {
                                        ++clearings;
                                        execute_intermediate_market(
                                                world.goods.get_good_instance_by_definition(
                                                        *world.economy.intermediate
                                                )
                                        );
                                }
                        );

                        auto provenance =
                                reordered_runtime.get_latest_provenance();

                        REQUIRE(provenance.has_value());
                        REQUIRE(provenance->workforce.has_value());

                        auto secondary_workforce =
                                reordered_runtime
                                        .get_additional_upstream_previous_workforce(0);

                        auto secondary_production =
                                reordered_runtime
                                        .get_additional_upstream_last_production(0);

                        auto secondary_market =
                                reordered_runtime
                                        .get_additional_upstream_last_market(0);

                        REQUIRE(secondary_workforce.has_value());
                        REQUIRE(secondary_production != nullptr);
                        REQUIRE(secondary_market != nullptr);

                        return Result {
                                .primary_allocated =
                                        provenance->workforce->allocated,
                                .secondary_allocated =
                                        secondary_workforce->allocated,
                                .primary_output =
                                        provenance->upstream.actual_output,
                                .secondary_output =
                                        secondary_production->actual_output,
                                .primary_sold =
                                        provenance->upstream_market.output_sold,
                                .secondary_sold =
                                        secondary_market->output_sold,
                                .total_upstream_output =
                                        reordered_runtime
                                                .get_status()
                                                .upstream_output,
                                .unemployed =
                                        world.province("1")
                                                .get_mutable_pops()
                                                .begin()
                                                ->get_unemployed(),
                                .clearings = clearings
                        };
                }

                REQUIRE(
                        world.runtime.bind_additional_upstream_site(
                                secondary,
                                *world.map
                        )
                );

                world.cycle();

                auto provenance = world.runtime.get_latest_provenance();

                REQUIRE(provenance.has_value());
                REQUIRE(provenance->workforce.has_value());

                auto secondary_workforce =
                        world.runtime
                                .get_additional_upstream_previous_workforce(0);

                auto secondary_production =
                        world.runtime
                                .get_additional_upstream_last_production(0);

                auto secondary_market =
                        world.runtime
                                .get_additional_upstream_last_market(0);

                REQUIRE(secondary_workforce.has_value());
                REQUIRE(secondary_production != nullptr);
                REQUIRE(secondary_market != nullptr);

                return Result {
                        .primary_allocated =
                                provenance->workforce->allocated,
                        .secondary_allocated =
                                secondary_workforce->allocated,
                        .primary_output =
                                provenance->upstream.actual_output,
                        .secondary_output =
                                secondary_production->actual_output,
                        .primary_sold =
                                provenance->upstream_market.output_sold,
                        .secondary_sold =
                                secondary_market->output_sold,
                        .total_upstream_output =
                                world.runtime.get_status().upstream_output,
                        .unemployed =
                                world.province("1")
                                        .get_mutable_pops()
                                        .begin()
                                        ->get_unemployed(),
                        .clearings = world.clearings
                };
        };

        Result const primary_first = run(false);
        Result const secondary_first = run(true);

        // Two distinct real BuildingInstances consume the one 60-worker POP pool.
        CHECK(primary_first.primary_allocated == fixed_point_t { 40 });
        CHECK(primary_first.secondary_allocated == fixed_point_t { 20 });
        CHECK(
                primary_first.primary_allocated +
                primary_first.secondary_allocated
                == fixed_point_t { 60 }
        );
        CHECK(primary_first.unemployed == pop_size_t { 0 });

        // Each producer can only produce from the workforce assigned to itself.
        CHECK(primary_first.primary_output > fixed_point_t::_0);
        CHECK(primary_first.secondary_output > fixed_point_t::_0);

        // The finite source inflow is conserved rather than duplicated.
        CHECK(
                primary_first.total_upstream_output
                == fixed_point_t { 4 }
        );

        // Both bridges participate independently in the same GoodMarket clear.
        CHECK(primary_first.primary_sold > fixed_point_t::_0);
        CHECK(primary_first.secondary_sold > fixed_point_t::_0);
        CHECK(
                primary_first.primary_sold +
                primary_first.secondary_sold
                == primary_first.total_upstream_output
        );
        CHECK(primary_first.clearings == 1);

        // Binding-call order does not change authoritative outcomes.
        CHECK(secondary_first == primary_first);
}

TEST_CASE(
"Site-specific physical input access isolates one productive-site disruption",
"[economy][productive-site][logistics][site-access][determinism][b6]"
) {
        struct Result final {
                fixed_point_t primary_allocated = fixed_point_t::_0;
                fixed_point_t secondary_allocated = fixed_point_t::_0;
                fixed_point_t primary_output = fixed_point_t::_0;
                fixed_point_t secondary_output = fixed_point_t::_0;
                fixed_point_t total_output = fixed_point_t::_0;
                fixed_point_t market_supply = fixed_point_t::_0;
                fixed_point_t market_traded = fixed_point_t::_0;
                fixed_point_t source_unmet = fixed_point_t::_0;
                pop_size_t unemployed = 0;

                bool operator==(Result const&) const = default;
        };

        auto run = [](bool disrupt_primary, bool reverse_route_order) {
                BoundWorldFixture world;

                world.plant().set_level(building_level_t { 2 });
                world.plant_b().set_level(building_level_t { 2 });
                world.replace_population("1", 80);

                LiveEconomyScenarioDefinition scenario =
                        world.economy.make_scenario();

                scenario.source_inflow_per_daily_tick =
                        fixed_point_t { 8 };

                REQUIRE(!scenario.corridor_legs.empty());
                scenario.corridor_legs[0].nominal_capacity =
                        fixed_point_t { 8 };

                LiveEconomyRuntime runtime {
                        world.economy.rules,
                        world.goods,
                        scenario
                };

                ProductiveSiteBinding secondary {
                        "1",
                        "plant_b",
                        "live_upstream",
                        LaborPoolScope::ProvinceLocal
                };

                REQUIRE(
                        runtime.bind_upstream_site(
                                world.binding,
                                *world.map
                        )
                );

                REQUIRE(
                        runtime.bind_additional_upstream_site(
                                secondary,
                                *world.map
                        )
                );

                REQUIRE(
                        runtime.configure_logistics_graph(
                                {
                                        LogisticsGraphEdge {
                                                .edge_id = "feedstock_to_primary",
                                                .source = market_node_index_t { 0 },
                                                .destination = market_node_index_t { 10 },
                                                .leg = TransportLeg {
                                                        .nominal_capacity =
                                                                fixed_point_t { 8 },
                                                        .availability_fraction =
                                                                fixed_point_t::_1,
                                                        .open = true
                                                }
                                        },
                                        LogisticsGraphEdge {
                                                .edge_id = "feedstock_to_secondary",
                                                .source = market_node_index_t { 0 },
                                                .destination = market_node_index_t { 20 },
                                                .leg = TransportLeg {
                                                        .nominal_capacity =
                                                                fixed_point_t { 8 },
                                                        .availability_fraction =
                                                                fixed_point_t::_1,
                                                        .open = true
                                                }
                                        }
                                }
                        )
                );

                ProductiveSiteResourceRoute primary_route {
                        .employer_id = "site:1:plant",
                        .source_node = market_node_index_t { 0 },
                        .destination_node = market_node_index_t { 10 }
                };

                ProductiveSiteResourceRoute secondary_route {
                        .employer_id = "site:1:plant_b",
                        .source_node = market_node_index_t { 0 },
                        .destination_node = market_node_index_t { 20 }
                };

                std::vector<ProductiveSiteResourceRoute> routes =
                        reverse_route_order
                                ? std::vector<ProductiveSiteResourceRoute> {
                                        secondary_route,
                                        primary_route
                                }
                                : std::vector<ProductiveSiteResourceRoute> {
                                        primary_route,
                                        secondary_route
                                };

                REQUIRE(
                        runtime.configure_upstream_site_resource_routes(
                                std::move(routes)
                        )
                );

                if (disrupt_primary) {
                        REQUIRE(
                                runtime.set_logistics_graph_edge_open(
                                        "feedstock_to_primary",
                                        false
                                )
                        );
                }

                world.map->prepare_employment_phase();

                auto requests =
                        runtime.prepare_upstream_employer_requests(
                                *world.map
                        );

                auto allocations =
                        world.map->allocate_employment_phase(
                                std::move(requests)
                        );

                runtime.apply_upstream_employer_allocations(
                        allocations
                );

                world.map->finish_rgo_production();

                SimulationTimeline timeline;
                REQUIRE(timeline.advance(24));

                runtime.run_due_daily_cycles(
                        SimTime { 0 },
                        timeline.current_time(),
                        [&](SimTime) -> std::optional<WorkforcePool> {
                                return std::nullopt;
                        },
                        [&]() {
                                execute_intermediate_market(
                                        world.goods
                                                .get_good_instance_by_definition(
                                                        *world.economy.intermediate
                                                )
                                );
                        }
                );

                auto provenance = runtime.get_latest_provenance();
                REQUIRE(provenance.has_value());
                REQUIRE(provenance->workforce.has_value());

                auto secondary_workforce =
                        runtime.get_additional_upstream_previous_workforce(0);

                auto secondary_production =
                        runtime.get_additional_upstream_last_production(0);

                REQUIRE(secondary_workforce.has_value());
                REQUIRE(secondary_production != nullptr);

                auto status = runtime.get_status();

                return Result {
                        .primary_allocated =
                                provenance->workforce->allocated,
                        .secondary_allocated =
                                secondary_workforce->allocated,
                        .primary_output =
                                provenance->upstream.actual_output,
                        .secondary_output =
                                secondary_production->actual_output,
                        .total_output =
                                status.upstream_output,
                        .market_supply =
                                status.intermediate_supply_yesterday,
                        .market_traded =
                                status.intermediate_quantity_traded_yesterday,
                        .source_unmet =
                                status.source_unmet_inflow,
                        .unemployed =
                                world.province("1")
                                        .get_mutable_pops()
                                        .begin()
                                        ->get_unemployed()
                };
        };

        Result const open = run(false, false);
        Result const open_reversed = run(false, true);
        Result const disrupted = run(true, false);
        Result const disrupted_reversed = run(true, true);

        CHECK(open_reversed == open);
        CHECK(disrupted_reversed == disrupted);

        CHECK(open.primary_allocated == fixed_point_t { 40 });
        CHECK(open.secondary_allocated == fixed_point_t { 40 });
        CHECK(open.primary_output == fixed_point_t { 4 });
        CHECK(open.secondary_output == fixed_point_t { 4 });
        CHECK(open.total_output == fixed_point_t { 8 });
        CHECK(open.market_supply == fixed_point_t { 8 });
        CHECK(open.market_traded == fixed_point_t { 8 });
        CHECK(open.source_unmet == fixed_point_t::_0);
        CHECK(open.unemployed == pop_size_t { 0 });

        CHECK(disrupted.primary_allocated == open.primary_allocated);
        CHECK(disrupted.secondary_allocated == open.secondary_allocated);
        CHECK(disrupted.unemployed == open.unemployed);
        CHECK(disrupted.primary_output == fixed_point_t::_0);
        CHECK(disrupted.secondary_output == open.secondary_output);

        CHECK(
                open.total_output - disrupted.total_output
                == open.primary_output
        );
        CHECK(
                open.market_supply - disrupted.market_supply
                == open.primary_output
        );
        CHECK(
                open.market_traded - disrupted.market_traded
                == open.primary_output
        );
        CHECK(disrupted.source_unmet == open.primary_output);
}

TEST_CASE(
	"Multiple physical input goods constrain one productive site independently",
	"[economy][productive-site][multi-input][logistics][inventory][b7]"
) {
	LiveEconomyFixture fixture {
		{},
		pop_size_t { 0 },
		ProductionType::template_type_t::PROCESS,
		true
	};

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

	REQUIRE(
		runtime.configure_additional_upstream_resource_input(
			*fixture.feedstock_b,
			fixed_point_t { 8 },
			market_node_index_t { 0 }
		)
	);

	CHECK(runtime.get_additional_upstream_resource_input_count() == 1);

	REQUIRE(
		runtime.configure_logistics_graph(
			{
				LogisticsGraphEdge {
					.edge_id = "primary_input_path",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 8 },
						.availability_fraction = fixed_point_t::_1,
						.open = true
					}
				},
				LogisticsGraphEdge {
					.edge_id = "secondary_input_path",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 20 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 8 },
						.availability_fraction = fixed_point_t::_1,
						.open = true
					}
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_site_resource_routes(
			{
				ProductiveSiteResourceRoute {
					.employer_id = "site:primary",
					.source_node = market_node_index_t { 0 },
					.destination_node = market_node_index_t { 10 },
					.good = fixture.feedstock
				},
				ProductiveSiteResourceRoute {
					.employer_id = "site:primary",
					.source_node = market_node_index_t { 0 },
					.destination_node = market_node_index_t { 20 },
					.good = fixture.feedstock_b
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		good_instances.get_good_instance_by_definition(
			*fixture.intermediate
		);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();

	auto first = runtime.get_latest_provenance();
	REQUIRE(first.has_value());
	CHECK(first->upstream.actual_output == fixed_point_t { 4 });
	REQUIRE(first->upstream.input_supported_output.has_value());
	CHECK(
		*first->upstream.input_supported_output ==
			fixed_point_t { 4 }
	);

	AggregateProducer& producer =
		runtime.get_upstream_producer_for_workforce_allocation();

	CHECK(
		producer.get_inventory(*fixture.feedstock) ==
			fixed_point_t::_0
	);
	CHECK(
		producer.get_inventory(*fixture.feedstock_b) ==
			fixed_point_t::_0
	);

	REQUIRE(
		runtime.set_logistics_graph_edge_open(
			"secondary_input_path",
			false
		)
	);

	cycle();

	auto blocked = runtime.get_latest_provenance();
	REQUIRE(blocked.has_value());
	CHECK(blocked->upstream.actual_output == fixed_point_t::_0);
	CHECK(blocked->upstream.input_limited);
	REQUIRE(blocked->upstream.input_supported_output.has_value());
	CHECK(
		*blocked->upstream.input_supported_output ==
			fixed_point_t::_0
	);

	CHECK(
		producer.get_inventory(*fixture.feedstock) ==
			fixed_point_t { 4 }
	);
	CHECK(
		producer.get_inventory(*fixture.feedstock_b) ==
			fixed_point_t::_0
	);

	REQUIRE(
		runtime.set_logistics_graph_edge_open(
			"secondary_input_path",
			true
		)
	);

	REQUIRE(
		runtime.set_additional_upstream_resource_availability(
			*fixture.feedstock_b,
			fixed_point_t::_1 / fixed_point_t { 2 }
		)
	);

	cycle();

	auto scarce = runtime.get_latest_provenance();
	REQUIRE(scarce.has_value());
	CHECK(scarce->upstream.actual_output == fixed_point_t { 2 });
	CHECK(scarce->upstream.input_limited);
	REQUIRE(scarce->upstream.input_supported_output.has_value());
	CHECK(
		*scarce->upstream.input_supported_output ==
			fixed_point_t { 2 }
	);

	CHECK(
		producer.get_inventory(*fixture.feedstock) ==
			fixed_point_t { 2 }
	);
	CHECK(
		producer.get_inventory(*fixture.feedstock_b) ==
			fixed_point_t::_0
	);

	CHECK(runtime.get_status().upstream_output == fixed_point_t { 2 });
}

TEST_CASE(
	"Electricity and industrial water constrain production before material demand",
	"[economy][utilities][electricity][water][b8]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();
	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t { 2 },
					.available_per_tick = fixed_point_t { 4 }
				},
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::IndustrialWater,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t { 3 }
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();

	auto first = runtime.get_latest_provenance();
	REQUIRE(first.has_value());
	CHECK(first->upstream.external_limited);
	REQUIRE(first->upstream.external_supported_output.has_value());
	CHECK(
		*first->upstream.external_supported_output ==
			fixed_point_t { 2 }
	);
	CHECK(first->upstream.desired_output == fixed_point_t { 2 });
	CHECK(first->upstream.actual_output == fixed_point_t { 2 });

	AggregateProducer& producer =
		runtime.get_upstream_producer_for_workforce_allocation();

	// Utility ceiling is applied before material shortfall is calculated,
	// so only two units of feedstock are requested/consumed this cycle.
	CHECK(
		producer.get_inventory(*fixture.feedstock) ==
			fixed_point_t::_0
	);

	REQUIRE(
		runtime.set_upstream_site_utility_availability(
			"site:primary",
			ProductiveSiteUtilityKind::Electricity,
			fixed_point_t { 8 }
		)
	);

	cycle();

	auto water_limited = runtime.get_latest_provenance();
	REQUIRE(water_limited.has_value());
	CHECK(water_limited->upstream.external_limited);
	REQUIRE(
		water_limited->upstream.external_supported_output.has_value()
	);
	CHECK(
		*water_limited->upstream.external_supported_output ==
			fixed_point_t { 3 }
	);
	CHECK(
		water_limited->upstream.actual_output ==
			fixed_point_t { 3 }
	);

	REQUIRE(
		runtime.set_upstream_site_utility_availability(
			"site:primary",
			ProductiveSiteUtilityKind::IndustrialWater,
			fixed_point_t { 4 }
		)
	);

	cycle();

	auto full = runtime.get_latest_provenance();
	REQUIRE(full.has_value());
	CHECK_FALSE(full->upstream.external_limited);
	REQUIRE(full->upstream.external_supported_output.has_value());
	CHECK(
		*full->upstream.external_supported_output ==
			fixed_point_t { 4 }
	);
	CHECK(full->upstream.actual_output == fixed_point_t { 4 });
}

TEST_CASE(
	"Shared electricity grid prevents generation and transmission double spending",
	"[economy][utilities][electricity-grid][b9]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer first {
		"grid-site-a",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};
	AggregateProducer second {
		"grid-site-b",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{ .employer_id = "grid-site-a", .producer = &first },
		{ .employer_id = "grid-site-b", .producer = &second }
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "grid-site-a",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		},
		{
			.employer_id = "grid-site-b",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "shared-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 1 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 6 }
					}
				},
				LogisticsGraphEdge {
					.edge_id = "site-a-line",
					.source = market_node_index_t { 1 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				},
				LogisticsGraphEdge {
					.edge_id = "site-b-line",
					.source = market_node_index_t { 1 },
					.destination = market_node_index_t { 20 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	auto allocations = ProductiveSiteElectricityGridResolver::resolve(
		ProductiveSiteElectricityGridState {
			.source_node = market_node_index_t { 0 },
			.available_generation_per_tick = fixed_point_t { 6 }
		},
		transmission,
		{
			{
				.employer_id = "grid-site-a",
				.destination_node = market_node_index_t { 10 }
			},
			{
				.employer_id = "grid-site-b",
				.destination_node = market_node_index_t { 20 }
			}
		},
		targets,
		requirements
	);

	REQUIRE(allocations.size() == 2);
	CHECK(allocations[0].requested == fixed_point_t { 4 });
	CHECK(allocations[1].requested == fixed_point_t { 4 });
	CHECK(allocations[0].delivered == fixed_point_t { 3 });
	CHECK(allocations[1].delivered == fixed_point_t { 3 });
	CHECK(requirements[0].available_per_tick == fixed_point_t { 3 });
	CHECK(requirements[1].available_per_tick == fixed_point_t { 3 });

	// Generation becomes tighter than transmission: the same two loads
	// proportionally share one finite generation pool.
	allocations = ProductiveSiteElectricityGridResolver::resolve(
		ProductiveSiteElectricityGridState {
			.source_node = market_node_index_t { 0 },
			.available_generation_per_tick = fixed_point_t { 4 }
		},
		transmission,
		{
			{
				.employer_id = "grid-site-a",
				.destination_node = market_node_index_t { 10 }
			},
			{
				.employer_id = "grid-site-b",
				.destination_node = market_node_index_t { 20 }
			}
		},
		targets,
		requirements
	);

	CHECK(allocations[0].transmission_allocated == fixed_point_t { 3 });
	CHECK(allocations[1].transmission_allocated == fixed_point_t { 3 });
	CHECK(allocations[0].delivered == fixed_point_t { 2 });
	CHECK(allocations[1].delivered == fixed_point_t { 2 });
}

TEST_CASE(
	"Runtime electricity grid drives productive-site utility ceiling",
	"[economy][utilities][electricity-grid][runtime][b9]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();
	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t::_0
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_grid(
			ProductiveSiteElectricityGridState {
				.source_node = market_node_index_t { 0 },
				.available_generation_per_tick = fixed_point_t { 2 }
			},
			{
				LogisticsGraphEdge {
					.edge_id = "primary-grid-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			},
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "site:primary",
					.destination_node = market_node_index_t { 10 }
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();
	auto limited = runtime.get_latest_provenance();
	REQUIRE(limited.has_value());
	CHECK(limited->upstream.external_limited);
	CHECK(limited->upstream.actual_output == fixed_point_t { 2 });

	REQUIRE(
		runtime.set_upstream_electricity_generation(
			fixed_point_t { 4 }
		)
	);

	cycle();
	auto restored = runtime.get_latest_provenance();
	REQUIRE(restored.has_value());
	CHECK_FALSE(restored->upstream.external_limited);
	CHECK(restored->upstream.actual_output == fixed_point_t { 4 });

	REQUIRE(
		runtime.set_upstream_electricity_edge_open(
			"primary-grid-line",
			false
		)
	);

	cycle();
	auto outage = runtime.get_latest_provenance();
	REQUIRE(outage.has_value());
	CHECK(outage->upstream.external_limited);
	CHECK(outage->upstream.actual_output == fixed_point_t::_0);
}

TEST_CASE(
	"Multiple electricity sources inject at distinct grid nodes without double spending",
	"[economy][utilities][electricity-grid][multi-source][b10]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer plant {
		"multi-source-plant",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{
			.employer_id = "multi-source-plant",
			.producer = &plant
		}
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "multi-source-plant",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "north-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 2 }
					}
				},
				LogisticsGraphEdge {
					.edge_id = "south-line",
					.source = market_node_index_t { 20 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 2 }
					}
				}
			}
		)
	);

	auto allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "north",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick =
						fixed_point_t { 2 }
				},
				ProductiveSiteElectricitySource {
					.source_id = "south",
					.source_node = market_node_index_t { 20 },
					.available_generation_per_tick =
						fixed_point_t { 2 }
				}
			},
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "multi-source-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].requested == fixed_point_t { 4 });
	CHECK(allocations[0].delivered == fixed_point_t { 4 });
	CHECK(requirements[0].available_per_tick == fixed_point_t { 4 });

	// Losing one generator removes only that generator's contribution.
	allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "north",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick =
						fixed_point_t::_0
				},
				ProductiveSiteElectricitySource {
					.source_id = "south",
					.source_node = market_node_index_t { 20 },
					.available_generation_per_tick =
						fixed_point_t { 2 }
				}
			},
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "multi-source-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	CHECK(allocations[0].delivered == fixed_point_t { 2 });
	CHECK(requirements[0].available_per_tick == fixed_point_t { 2 });
}

TEST_CASE(
	"Runtime multi-source electricity generation changes productive output",
	"[economy][utilities][electricity-grid][multi-source][runtime][b10]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();
	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t::_0
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "plant_a",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick =
						fixed_point_t { 2 }
				},
				ProductiveSiteElectricitySource {
					.source_id = "plant_b",
					.source_node = market_node_index_t { 20 },
					.available_generation_per_tick =
						fixed_point_t { 2 }
				}
			},
			{
				LogisticsGraphEdge {
					.edge_id = "plant-a-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 2 }
					}
				},
				LogisticsGraphEdge {
					.edge_id = "plant-b-line",
					.source = market_node_index_t { 20 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 2 }
					}
				}
			},
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "site:primary",
					.destination_node = market_node_index_t { 10 }
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();
	auto full = runtime.get_latest_provenance();
	REQUIRE(full.has_value());
	CHECK(full->upstream.actual_output == fixed_point_t { 4 });

	REQUIRE(
		runtime.set_upstream_electricity_source_generation(
			"plant_a",
			fixed_point_t::_0
		)
	);

	cycle();
	auto partial = runtime.get_latest_provenance();
	REQUIRE(partial.has_value());
	CHECK(partial->upstream.external_limited);
	CHECK(partial->upstream.actual_output == fixed_point_t { 2 });

	REQUIRE(
		runtime.set_upstream_electricity_source_generation(
			"plant_a",
			fixed_point_t { 2 }
		)
	);

	REQUIRE(
		runtime.set_upstream_electricity_edge_open(
			"plant-b-line",
			false
		)
	);

	cycle();
	auto stranded = runtime.get_latest_provenance();
	REQUIRE(stranded.has_value());
	CHECK(stranded->upstream.external_limited);
	CHECK(stranded->upstream.actual_output == fixed_point_t { 2 });
}

TEST_CASE(
	"Electricity redispatch uses reachable spare generation after source outage",
	"[economy][utilities][electricity-grid][redispatch][b11]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer plant {
		"redispatch-plant",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{
			.employer_id = "redispatch-plant",
			.producer = &plant
		}
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "redispatch-plant",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "blocked-source-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 },
						.open = false
					}
				},
				LogisticsGraphEdge {
					.edge_id = "backup-source-line",
					.source = market_node_index_t { 20 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	auto allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "a_blocked",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick =
						fixed_point_t { 4 }
				},
				ProductiveSiteElectricitySource {
					.source_id = "b_backup",
					.source_node = market_node_index_t { 20 },
					.available_generation_per_tick =
						fixed_point_t { 4 }
				}
			},
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "redispatch-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].requested == fixed_point_t { 4 });
	CHECK(allocations[0].delivered == fixed_point_t { 4 });
	CHECK(requirements[0].available_per_tick == fixed_point_t { 4 });
}

TEST_CASE(
	"Runtime electricity redispatch restores output through surviving generator",
	"[economy][utilities][electricity-grid][redispatch][runtime][b11]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();
	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t::_0
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "a_primary",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick =
						fixed_point_t { 4 }
				},
				ProductiveSiteElectricitySource {
					.source_id = "b_backup",
					.source_node = market_node_index_t { 20 },
					.available_generation_per_tick =
						fixed_point_t { 4 }
				}
			},
			{
				LogisticsGraphEdge {
					.edge_id = "primary-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				},
				LogisticsGraphEdge {
					.edge_id = "backup-line",
					.source = market_node_index_t { 20 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			},
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "site:primary",
					.destination_node = market_node_index_t { 10 }
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();
	auto full = runtime.get_latest_provenance();
	REQUIRE(full.has_value());
	CHECK(full->upstream.actual_output == fixed_point_t { 4 });

	REQUIRE(
		runtime.set_upstream_electricity_edge_open(
			"primary-line",
			false
		)
	);

	cycle();
	auto redispatched = runtime.get_latest_provenance();
	REQUIRE(redispatched.has_value());
	CHECK_FALSE(redispatched->upstream.external_limited);
	CHECK(redispatched->upstream.actual_output == fixed_point_t { 4 });
}

TEST_CASE(
	"Generator operating characteristics constrain and order dispatch",
	"[economy][utilities][electricity-grid][generator-characteristics][b12]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer plant {
		"characteristics-plant",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{
			.employer_id = "characteristics-plant",
			.producer = &plant
		}
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "characteristics-plant",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "cheap-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				},
				LogisticsGraphEdge {
					.edge_id = "fast-line",
					.source = market_node_index_t { 20 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	std::vector<ProductiveSiteElectricitySource> sources {
		ProductiveSiteElectricitySource {
			.source_id = "cheap_slow",
			.source_node = market_node_index_t { 0 },
			.available_generation_per_tick = fixed_point_t { 4 },
			.availability_fraction = fixed_point_t::_1,
			.minimum_stable_output = fixed_point_t { 2 },
			.ramp_up_per_tick = fixed_point_t { 2 },
			.ramp_down_per_tick = fixed_point_t { 2 },
			.marginal_cost = fixed_point_t { 1 },
			.dispatch_priority = 0
		},
		ProductiveSiteElectricitySource {
			.source_id = "fast_expensive",
			.source_node = market_node_index_t { 20 },
			.available_generation_per_tick = fixed_point_t { 4 },
			.availability_fraction = fixed_point_t::_1,
			.ramp_up_per_tick = fixed_point_t { 4 },
			.ramp_down_per_tick = fixed_point_t { 4 },
			.marginal_cost = fixed_point_t { 5 },
			.dispatch_priority = 0
		}
	};

	auto allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
			sources,
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "characteristics-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].delivered == fixed_point_t { 4 });

	// Cheap unit can only ramp from 0 to 2 on first tick; expensive unit fills 2.
	CHECK(sources[0].current_dispatch_per_tick == fixed_point_t { 2 });
	CHECK(sources[1].current_dispatch_per_tick == fixed_point_t { 2 });

	allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
			sources,
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "characteristics-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	CHECK(allocations[0].delivered == fixed_point_t { 4 });

	// Second tick cheap unit can ramp to 4 and displace expensive generation.
	CHECK(sources[0].current_dispatch_per_tick == fixed_point_t { 4 });
	CHECK(sources[1].current_dispatch_per_tick == fixed_point_t::_0);
}

TEST_CASE(
	"Runtime generator availability factor propagates into production",
	"[economy][utilities][electricity-grid][generator-characteristics][availability][runtime][b12]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();
	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t::_0
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "variable_source",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick = fixed_point_t { 4 },
					.availability_fraction = fixed_point_t::_1,
					.ramp_up_per_tick = fixed_point_t { 4 },
					.ramp_down_per_tick = fixed_point_t { 4 }
				}
			},
			{
				LogisticsGraphEdge {
					.edge_id = "variable-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			},
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "site:primary",
					.destination_node = market_node_index_t { 10 }
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();
	auto full = runtime.get_latest_provenance();
	REQUIRE(full.has_value());
	CHECK(full->upstream.actual_output == fixed_point_t { 4 });

	REQUIRE(
		runtime.set_upstream_electricity_source_availability(
			"variable_source",
			fixed_point_t::_1 / fixed_point_t { 2 }
		)
	);

	cycle();
	auto half = runtime.get_latest_provenance();
	REQUIRE(half.has_value());
	CHECK(half->upstream.external_limited);
	CHECK(half->upstream.actual_output == fixed_point_t { 2 });
}

TEST_CASE(
	"Generator fuel inventory physically limits dispatch and is consumed",
	"[economy][utilities][electricity-grid][generator-fuel][b13]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer plant {
		"fuel-limited-plant",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{
			.employer_id = "fuel-limited-plant",
			.producer = &plant
		}
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "fuel-limited-plant",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "fuel-generator-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	std::vector<ProductiveSiteElectricitySource> sources {
		ProductiveSiteElectricitySource {
			.source_id = "gas_unit",
			.source_node = market_node_index_t { 0 },
			.available_generation_per_tick = fixed_point_t { 4 },
			.ramp_up_per_tick = fixed_point_t { 4 },
			.ramp_down_per_tick = fixed_point_t { 4 },
			.fuel_good = fixture.feedstock,
			.fuel_per_output = fixed_point_t { 2 },
			.fuel_inventory = fixed_point_t { 4 }
		}
	};

	auto allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
			sources,
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "fuel-limited-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].delivered == fixed_point_t { 2 });
	CHECK(sources[0].current_dispatch_per_tick == fixed_point_t { 2 });
	CHECK(sources[0].fuel_inventory == fixed_point_t::_0);
}

TEST_CASE(
	"Generator fuel replenishment shares authoritative material allocation",
	"[economy][materials][generator-fuel][shared-supply][b13]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer producer {
		"factory",
		*fixture.upstream_process,
		fixed_point_t { 2 },
		fixed_point_t::_1
	};
	AggregateProducerMarketBridge bridge { producer };

	fixed_point_t generator_inventory = fixed_point_t::_0;

	ResourceSupplyNetwork network {
		{
			ResourceSourceState {
				.source_id = "shared-fuel-source",
				.node = market_node_index_t { 0 },
				.supply = ResourceSupplyState {
					.nominal_per_tick = fixed_point_t { 4 }
				}
			}
		}
	};

	LogisticsGraph logistics;
	REQUIRE(
		logistics.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "shared-freight",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	auto result = ProductiveSiteMaterialFlowResolver::resolve(
		{
			ProductiveSiteMaterialInput {
				.good = fixture.feedstock,
				.desired_flow = fixed_point_t { 4 },
				.network = &network,
				.primary = true
			}
		},
		{
			ProductiveSiteMaterialTarget {
				.employer_id = "factory",
				.producer = &producer,
				.bridge = &bridge
			}
		},
		{
			MaterialInventoryTarget {
				.consumer_id = "generator:gas",
				.good = fixture.feedstock,
				.desired_inventory = fixed_point_t { 2 },
				.inventory = &generator_inventory
			}
		},
		{
			ProductiveSiteResourceRoute {
				.employer_id = "factory",
				.source_node = market_node_index_t { 0 },
				.destination_node = market_node_index_t { 10 },
				.good = fixture.feedstock
			},
			ProductiveSiteResourceRoute {
				.employer_id = "generator:gas",
				.source_node = market_node_index_t { 0 },
				.destination_node = market_node_index_t { 10 },
				.good = fixture.feedstock
			}
		},
		logistics
	);

	CHECK(result.delivered == fixed_point_t { 4 });

	// Factory and generator are resolved inside one source-network call and
	// one shared logistics batch; their combined receipt cannot exceed 4.
	CHECK(
		producer.get_inventory(*fixture.feedstock) +
			generator_inventory ==
		fixed_point_t { 4 }
	);
	CHECK(generator_inventory > fixed_point_t::_0);
}

TEST_CASE(
	"Runtime generator fuel shortage propagates into industrial output",
	"[economy][utilities][electricity-grid][generator-fuel][runtime][b13]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();

	// Isolate the electricity causal leg in this regression. The separate
	// shared-supply B13 test covers factory/generator competition for scarce
	// feedstock. Here there is enough incoming material for both the factory
	// and generator replenishment, so the initial 2-unit generator fuel stock
	// is the only reason industrial output is capped at 2.
	scenario.source_inflow_per_daily_tick = fixed_point_t { 8 };

	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t::_0
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "fuelled_unit",
					.source_node = market_node_index_t { 0 },
					.available_generation_per_tick = fixed_point_t { 4 },
					.ramp_up_per_tick = fixed_point_t { 4 },
					.ramp_down_per_tick = fixed_point_t { 4 }
				}
			},
			{
				LogisticsGraphEdge {
					.edge_id = "fuelled-unit-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			},
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "site:primary",
					.destination_node = market_node_index_t { 10 }
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_source_fuel(
			"fuelled_unit",
			scenario.source_inflow_good,
			fixed_point_t::_1,
			fixed_point_t { 2 }
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	runtime.pre_market_daily_tick();
	execute_intermediate_market(intermediate_market);
	runtime.post_market_daily_tick();

	auto limited = runtime.get_latest_provenance();
	REQUIRE(limited.has_value());
	CHECK(limited->upstream.external_limited);
	CHECK(limited->upstream.actual_output == fixed_point_t { 2 });
}

TEST_CASE(
	"Generator resource availability and forced outage constrain dispatch",
	"[economy][utilities][electricity-grid][generator-type][intermittency][outage][b14]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer plant {
		"variable-power-plant",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{
			.employer_id = "variable-power-plant",
			.producer = &plant
		}
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "variable-power-plant",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "wind-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	std::vector<ProductiveSiteElectricitySource> sources {
		ProductiveSiteElectricitySource {
			.source_id = "wind_farm",
			.source_node = market_node_index_t { 0 },
			.generator_kind = ElectricityGeneratorKind::Wind,
			.available_generation_per_tick = fixed_point_t { 4 },
			.ramp_up_per_tick = fixed_point_t { 4 },
			.ramp_down_per_tick = fixed_point_t { 4 },
			.resource_availability_fraction =
				fixed_point_t::_1 / fixed_point_t { 2 }
		}
	};

	auto allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
			sources,
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "variable-power-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].delivered == fixed_point_t { 2 });

	sources[0].forced_outage = true;

	allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
			sources,
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "variable-power-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	CHECK(allocations[0].delivered == fixed_point_t::_0);
}

TEST_CASE(
	"Generator heat rate multiplier changes physical fuel requirement",
	"[economy][utilities][electricity-grid][generator-type][heat-rate][fuel][b14]"
) {
	LiveEconomyFixture fixture;

	AggregateProducer plant {
		"thermal-power-plant",
		*fixture.upstream_process,
		fixed_point_t { 4 },
		fixed_point_t::_1
	};

	std::vector<ProductiveSiteUtilityTarget> targets {
		{
			.employer_id = "thermal-power-plant",
			.producer = &plant
		}
	};

	std::vector<ProductiveSiteUtilityRequirement> requirements {
		{
			.employer_id = "thermal-power-plant",
			.kind = ProductiveSiteUtilityKind::Electricity,
			.required_per_output = fixed_point_t::_1,
			.available_per_tick = fixed_point_t::_0
		}
	};

	LogisticsGraph transmission;
	REQUIRE(
		transmission.configure(
			{
				LogisticsGraphEdge {
					.edge_id = "thermal-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			}
		)
	);

	std::vector<ProductiveSiteElectricitySource> sources {
		ProductiveSiteElectricitySource {
			.source_id = "gas_turbine",
			.source_node = market_node_index_t { 0 },
			.generator_kind = ElectricityGeneratorKind::Thermal,
			.available_generation_per_tick = fixed_point_t { 4 },
			.ramp_up_per_tick = fixed_point_t { 4 },
			.ramp_down_per_tick = fixed_point_t { 4 },
			.fuel_good = fixture.feedstock,
			.fuel_per_output = fixed_point_t::_1,
			.fuel_inventory = fixed_point_t { 4 },
			.heat_rate_multiplier = fixed_point_t { 2 }
		}
	};

	auto allocations =
		ProductiveSiteElectricityGridResolver::resolve_sources_stateful(
			sources,
			transmission,
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "thermal-power-plant",
					.destination_node = market_node_index_t { 10 }
				}
			},
			targets,
			requirements
		);

	REQUIRE(allocations.size() == 1);
	CHECK(allocations[0].delivered == fixed_point_t { 2 });
	CHECK(sources[0].fuel_inventory == fixed_point_t::_0);
}

TEST_CASE(
	"Runtime variable generation and forced outage propagate into industry",
	"[economy][utilities][electricity-grid][generator-type][runtime][b14]"
) {
	LiveEconomyFixture fixture;
	GoodInstanceManager goods { fixture.definitions, fixture.rules };
	auto scenario = fixture.make_scenario();
	LiveEconomyRuntime runtime { fixture.rules, goods, scenario };

	REQUIRE(
		runtime.configure_upstream_site_utility_requirements(
			{
				ProductiveSiteUtilityRequirement {
					.employer_id = "site:primary",
					.kind = ProductiveSiteUtilityKind::Electricity,
					.required_per_output = fixed_point_t::_1,
					.available_per_tick = fixed_point_t::_0
				}
			}
		)
	);

	REQUIRE(
		runtime.configure_upstream_electricity_sources(
			{
				ProductiveSiteElectricitySource {
					.source_id = "wind_source",
					.source_node = market_node_index_t { 0 },
					.generator_kind = ElectricityGeneratorKind::Wind,
					.available_generation_per_tick = fixed_point_t { 4 },
					.ramp_up_per_tick = fixed_point_t { 4 },
					.ramp_down_per_tick = fixed_point_t { 4 }
				}
			},
			{
				LogisticsGraphEdge {
					.edge_id = "wind-runtime-line",
					.source = market_node_index_t { 0 },
					.destination = market_node_index_t { 10 },
					.leg = TransportLeg {
						.nominal_capacity = fixed_point_t { 4 }
					}
				}
			},
			{
				ProductiveSiteElectricityConnection {
					.employer_id = "site:primary",
					.destination_node = market_node_index_t { 10 }
				}
			}
		)
	);

	GoodInstance& intermediate_market =
		goods.get_good_instance_by_definition(*fixture.intermediate);

	auto cycle = [&]() {
		runtime.pre_market_daily_tick();
		execute_intermediate_market(intermediate_market);
		runtime.post_market_daily_tick();
	};

	cycle();
	auto full = runtime.get_latest_provenance();
	REQUIRE(full.has_value());
	CHECK(full->upstream.actual_output == fixed_point_t { 4 });

	REQUIRE(
		runtime.set_upstream_electricity_source_resource_availability(
			"wind_source",
			fixed_point_t::_1 / fixed_point_t { 2 }
		)
	);

	cycle();
	auto variable = runtime.get_latest_provenance();
	REQUIRE(variable.has_value());
	CHECK(variable->upstream.external_limited);
	CHECK(variable->upstream.actual_output == fixed_point_t { 2 });

	REQUIRE(
		runtime.set_upstream_electricity_source_forced_outage(
			"wind_source",
			true
		)
	);

	cycle();
	auto outage = runtime.get_latest_provenance();
	REQUIRE(outage.has_value());
	CHECK(outage->upstream.external_limited);
	CHECK(outage->upstream.actual_output == fixed_point_t::_0);
}
