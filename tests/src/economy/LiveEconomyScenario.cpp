#include "openvic-simulation/economy/LiveEconomyRuntime.hpp"
#include "openvic-simulation/economy/LiveEconomyScenario.hpp"

#include <array>
#include <optional>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	struct ScenarioFixture {
		GoodDefinitionManager definitions;
		GameRulesManager rules;

		GoodCategory const* category = nullptr;
		GoodDefinition const* feedstock = nullptr;
		GoodDefinition const* intermediate = nullptr;
		GoodDefinition const* final_good = nullptr;

		std::optional<ProductionType> upstream_process;
		std::optional<ProductionType> downstream_process;

		ScenarioFixture() {
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
				"scenario_upstream",
				std::nullopt,
				memory::vector<Job> {},
				ProductionType::template_type_t::FACTORY,
				pop_size_t { 0 },
				std::move(upstream_inputs),
				*intermediate,
				fixed_point_t::_1,
				memory::vector<ProductionType::bonus_t> {},
				fixed_point_map_t<GoodDefinition const*> {},
				false,
				false,
				false
			);

			fixed_point_map_t<GoodDefinition const*> downstream_inputs;
			downstream_inputs.emplace(intermediate, fixed_point_t(2));

			downstream_process.emplace(
				rules,
				"scenario_downstream",
				std::nullopt,
				memory::vector<Job> {},
				ProductionType::template_type_t::FACTORY,
				pop_size_t { 0 },
				std::move(downstream_inputs),
				*final_good,
				fixed_point_t::_1,
				memory::vector<ProductionType::bonus_t> {},
				fixed_point_map_t<GoodDefinition const*> {},
				false,
				false,
				false
			);
		}

		LiveEconomyScenarioDefinition make_scenario(
			fixed_point_t corridor_capacity = fixed_point_t(4)
		) const {
			return LiveEconomyScenarioDefinition {
				.upstream_process = &*upstream_process,
				.downstream_process = &*downstream_process,
				.upstream_capacity = fixed_point_t(4),
				.upstream_utilization = fixed_point_t::_1,
				.downstream_capacity = fixed_point_t(4),
				.downstream_utilization = fixed_point_t::_1,
				.source_inflow_good = feedstock,
				.source_inflow_per_daily_tick = fixed_point_t(4),
				.source_node = market_node_index_t { 11 },
				.destination_node = market_node_index_t { 22 },
				.corridor_legs = {
					TransportLeg {
						.nominal_capacity = corridor_capacity,
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
	"Live economy scenario rejects missing causal links",
	"[economy][live-scenario][validation]"
) {
	ScenarioFixture fixture;

	LiveEconomyScenarioDefinition scenario = fixture.make_scenario();
	REQUIRE(scenario.is_valid());

	scenario.source_inflow_good = fixture.final_good;
	CHECK_FALSE(scenario.is_valid());

	scenario = fixture.make_scenario();
	scenario.corridor_legs.clear();
	CHECK_FALSE(scenario.is_valid());

	scenario = fixture.make_scenario();
	scenario.downstream_process = nullptr;
	CHECK_FALSE(scenario.is_valid());
}

TEST_CASE(
	"Live economy runtime obeys scenario-owned capacities nodes and corridor",
	"[economy][live-scenario][runtime]"
) {
	ScenarioFixture fixture;
	GoodInstanceManager good_instances {
		fixture.definitions,
		fixture.rules
	};

	LiveEconomyScenarioDefinition scenario =
		fixture.make_scenario(fixed_point_t(3));

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
	CHECK(status.completed_daily_ticks == 1);
	CHECK(status.upstream_output == fixed_point_t(4));
	CHECK(status.corridor_capacity == fixed_point_t(3));
	CHECK(status.intermediate_quantity_traded_yesterday == fixed_point_t(3));
	CHECK(status.downstream_desired_output == fixed_point_t(4));
	CHECK(status.downstream_output == fixed_point_t(3) / fixed_point_t(2));
	CHECK(status.downstream_input_limited);
}

TEST_CASE(
	"Scenario source inflow controls upstream live production",
	"[economy][live-scenario][source-inflow]"
) {
	ScenarioFixture fixture;
	GoodInstanceManager good_instances {
		fixture.definitions,
		fixture.rules
	};

	LiveEconomyScenarioDefinition scenario = fixture.make_scenario();
	scenario.source_inflow_per_daily_tick = fixed_point_t(2);

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

	CHECK(status.upstream_output == fixed_point_t(2));
	CHECK(status.intermediate_quantity_traded_yesterday == fixed_point_t(2));
	CHECK(status.downstream_output == fixed_point_t(1));
}