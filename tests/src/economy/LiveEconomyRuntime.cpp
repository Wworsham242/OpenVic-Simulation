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
uint8_t priority,
int workers
) {
return WorkforceEmployerRequest {
.employer_id = id,
.priority = priority,
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
first.request("first", 2, 80),
second.request("second", 1, 80)
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
"Competing employer priority changes allocation deterministically",
"[economy][native-workforce][competition][priority]"
) {
for (bool const first_has_priority : { true, false }) {
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
first.request("first", first_has_priority ? 2 : 1, 80),
second.request("second", first_has_priority ? 1 : 2, 80)
},
WorkforcePool { std::span<Pop> { pops } }
);

REQUIRE(result.size() == 2);

if (first_has_priority) {
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
"Equal-priority competing employers use stable employer identity",
"[economy][native-workforce][competition][determinism]"
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
requests.push_back(beta.request("beta", 1, 80));
requests.push_back(alpha.request("alpha", 1, 80));
} else {
requests.push_back(alpha.request("alpha", 1, 80));
requests.push_back(beta.request("beta", 1, 80));
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
			BuildingType::building_type_args_t args;
			args.type = "productive_site";
			args.in_province = true;
			args.capacity_per_level = 2;
			args.max_level = building_level_t { 5 };
			args.production_type = &*economy.upstream_process;
			REQUIRE(population.buildings.add_building_type("plant", args));
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
		void replace_population(std::string_view id, int size) {
			auto& location = province(id);
			location.get_mutable_pops().clear();
			location.get_mutable_pops().emplace(population.make_pop(population.eligible, size, 1, &location));
		}
		void cycle() {
			auto previous = timeline.current_time();
			REQUIRE(timeline.advance(24));
			// Fresh authoritative POPs in this fixture represent prepared daily
			// availability. Production uses the same post-map resolver as InstanceManager.
			runtime.run_due_daily_cycles(previous, timeline.current_time(),
				[&](SimTime) { return runtime.prepare_upstream_site(*map); },
				[&]() {
					++clearings;
					execute_intermediate_market(goods.get_good_instance_by_definition(*economy.intermediate));
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

TEST_CASE("Site binding validates identity and uses only residual authoritative unemployment",
	"[economy][productive-site][validation]") {
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
	CHECK_FALSE(world.binding.resolve(*world.map, *world.economy.downstream_process).has_value());
	invalid = world.binding;
	invalid.labor_scope = static_cast<LaborPoolScope>(99);
	CHECK_FALSE(world.runtime.bind_upstream_site(invalid, *world.map));
	// Model employment already claimed by the inherited earlier employer phase.
	world.province("1").get_mutable_pops().begin()->hire(pop_size_t { 20 });
	world.cycle();
	auto p = world.runtime.get_latest_provenance();
	REQUIRE(p.has_value());
	REQUIRE(p->workforce.has_value());
	CHECK(p->workforce->allocated == fixed_point_t { 20 });
	CHECK(p->upstream.labor_limited);
	CHECK(p->productive_site == std::optional<ProductiveSiteBinding> { world.binding });
}
