#include "openvic-simulation/economy/LiveEconomyRuntime.hpp"

#include <array>

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/GoodInstance.hpp"
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

	LiveEconomyRuntime runtime {
		fixture.rules,
		good_instances,
		*fixture.feedstock,
		*fixture.intermediate,
		*fixture.final_good
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
	"Live economy status exposes real corridor and market state",
	"[economy][live-runtime][status]"
) {
	LiveEconomyFixture fixture;

	GoodInstanceManager good_instances {
		fixture.definitions,
		fixture.rules
	};

	LiveEconomyRuntime runtime {
		fixture.rules,
		good_instances,
		*fixture.feedstock,
		*fixture.intermediate,
		*fixture.final_good
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