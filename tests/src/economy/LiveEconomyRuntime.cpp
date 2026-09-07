#include "openvic-simulation/economy/LiveEconomyRuntime.hpp"

#include <array>
#include <optional>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/LiveEconomyScenario.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"

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

		LiveEconomyFixture() {
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
				memory::vector<Job> {},
				ProductionType::template_type_t::FACTORY,
				pop_size_t { 0 },
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
