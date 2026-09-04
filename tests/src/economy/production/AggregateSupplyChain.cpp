#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"

#include <array>
#include <optional>

#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	GoodCategory category { "supply_chain_goods", good_category_index_t { 0 } };

	GoodDefinition feedstock {
		"feedstock",
		colour_rgb_t {},
		good_index_t { 0 },
		category,
		1,
		true,
		true,
		false,
		false
	};

	GoodDefinition intermediate {
		"intermediate",
		colour_rgb_t {},
		good_index_t { 1 },
		category,
		1,
		true,
		true,
		false,
		false
	};

	GoodDefinition final_good {
		"final_good",
		colour_rgb_t {},
		good_index_t { 2 },
		category,
		1,
		true,
		true,
		false,
		false
	};

	GameRulesManager rules {};

	ProductionType make_upstream_process() {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&feedstock, fixed_point_t::_1);

		return ProductionType {
			rules,
			"upstream_process",
			std::nullopt,
			{},
			ProductionType::template_type_t::FACTORY,
			pop_size_t { 0 },
			std::move(inputs),
			intermediate,
			fixed_point_t::_1,
			{},
			{},
			false,
			false,
			false
		};
	}

	ProductionType make_downstream_process() {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&intermediate, fixed_point_t(2));

		return ProductionType {
			rules,
			"downstream_process",
			std::nullopt,
			{},
			ProductionType::template_type_t::FACTORY,
			pop_size_t { 0 },
			std::move(inputs),
			final_good,
			fixed_point_t::_1,
			{},
			{},
			false,
			false,
			false
		};
	}

	struct ExternalBuyer {
		fixed_point_t quantity_bought = 0;
		fixed_point_t money_spent = 0;

		static void after_buy(void* actor, BuyResult const& result) {
			auto& buyer = *static_cast<ExternalBuyer*>(actor);
			buyer.quantity_bought += result.quantity_bought;
			buyer.money_spent += result.money_spent_total;
		}
	};

	void execute_market(GoodMarket& market) {
		TypedSpan<country_index_t, fixed_point_t> country_map_0 {};
		TypedSpan<country_index_t, fixed_point_t> country_map_1 {};
		std::array<
			memory::vector<fixed_point_t>,
			GoodMarket::VECTORS_FOR_EXECUTE_ORDERS
		> reusable_vectors;

		market.execute_orders(
			country_map_0,
			country_map_1,
			reusable_vectors
		);
	}

	void transfer_intermediate(
		AggregateProducerMarketBridge& upstream_bridge,
		AggregateProducerMarketBridge& downstream_bridge,
		GoodMarket& intermediate_market
	) {
		auto sell_order = upstream_bridge.make_output_sell_order();
		REQUIRE(sell_order.has_value());

		const fixed_point_t wanted =
			downstream_bridge.calculate_input_shortfall(intermediate);

		auto buy_order = downstream_bridge.make_input_buy_order(
			intermediate,
			wanted * intermediate_market.get_max_next_price()
		);
		REQUIRE(buy_order.has_value());

		intermediate_market.add_market_sell_order(std::move(*sell_order));
		intermediate_market.add_buy_up_to_order(std::move(*buy_order));
		execute_market(intermediate_market);

		upstream_bridge.clear_completed_orders();
		downstream_bridge.clear_completed_orders();
	}
}

TEST_CASE(
	"Two aggregate producers form a causal market-mediated supply chain",
	"[economy][aggregate-supply-chain][integration]"
) {
	ProductionType upstream_process = make_upstream_process();
	ProductionType downstream_process = make_downstream_process();

	AggregateProducer upstream {
		"upstream_industry",
		upstream_process,
		fixed_point_t(4),
		fixed_point_t::_1
	};
	AggregateProducer downstream {
		"downstream_industry",
		downstream_process,
		fixed_point_t(4),
		fixed_point_t::_1
	};

	AggregateProducerMarketBridge upstream_bridge { upstream };
	AggregateProducerMarketBridge downstream_bridge { downstream };

	upstream.set_inventory(feedstock, fixed_point_t(4));

	AggregateProductionResult const upstream_result = upstream.produce();
	REQUIRE(upstream_result.actual_output == fixed_point_t(4));
	REQUIRE(upstream.get_inventory(intermediate) == fixed_point_t(4));

	CHECK(downstream.calculate_desired_output() == fixed_point_t(4));
	CHECK(downstream_bridge.calculate_input_shortfall(intermediate) == fixed_point_t(8));

	GoodMarket intermediate_market { rules, intermediate };
	transfer_intermediate(
		upstream_bridge,
		downstream_bridge,
		intermediate_market
	);

	CHECK(upstream.get_inventory(intermediate) == fixed_point_t::_0);
	CHECK(downstream.get_inventory(intermediate) == fixed_point_t(4));

	AggregateProductionResult const downstream_result = downstream.produce();

	CHECK(downstream_result.desired_output == fixed_point_t(4));
	CHECK(downstream_result.actual_output == fixed_point_t(2));
	CHECK(downstream_result.input_limited);
	CHECK(downstream.get_inventory(intermediate) == fixed_point_t::_0);
	CHECK(downstream.get_inventory(final_good) == fixed_point_t(2));
}

TEST_CASE(
	"Downstream output reaches a second market and unsold inventory is preserved",
	"[economy][aggregate-supply-chain][final-market]"
) {
	ProductionType upstream_process = make_upstream_process();
	ProductionType downstream_process = make_downstream_process();

	AggregateProducer upstream {
		"upstream_industry",
		upstream_process,
		fixed_point_t(8),
		fixed_point_t::_1
	};
	AggregateProducer downstream {
		"downstream_industry",
		downstream_process,
		fixed_point_t(4),
		fixed_point_t::_1
	};

	AggregateProducerMarketBridge upstream_bridge { upstream };
	AggregateProducerMarketBridge downstream_bridge { downstream };

	upstream.set_inventory(feedstock, fixed_point_t(8));

	REQUIRE(upstream.produce().actual_output == fixed_point_t(8));

	GoodMarket intermediate_market { rules, intermediate };
	transfer_intermediate(
		upstream_bridge,
		downstream_bridge,
		intermediate_market
	);

	REQUIRE(downstream.get_inventory(intermediate) == fixed_point_t(8));
	REQUIRE(downstream.produce().actual_output == fixed_point_t(4));
	REQUIRE(downstream.get_inventory(final_good) == fixed_point_t(4));

	GoodMarket final_market { rules, final_good };
	ExternalBuyer buyer {};

	const fixed_point_t external_demand = fixed_point_t(3);

	final_market.add_buy_up_to_order({
		std::nullopt,
		external_demand,
		external_demand * final_market.get_max_next_price(),
		&buyer,
		ExternalBuyer::after_buy
	});

	auto final_sell_order = downstream_bridge.make_output_sell_order();
	REQUIRE(final_sell_order.has_value());

	final_market.add_market_sell_order(std::move(*final_sell_order));
	execute_market(final_market);
	downstream_bridge.clear_completed_orders();

	CHECK(buyer.quantity_bought == fixed_point_t(3));
	CHECK(downstream.get_inventory(final_good) == fixed_point_t(1));
	CHECK(downstream_bridge.get_money_received_from_sales() > 0);
}

TEST_CASE(
	"Upstream capacity shock propagates quantitatively into downstream output",
	"[economy][aggregate-supply-chain][shock]"
) {
	ProductionType upstream_process = make_upstream_process();
	ProductionType downstream_process = make_downstream_process();

	AggregateProducer upstream {
		"upstream_industry",
		upstream_process,
		fixed_point_t(8),
		fixed_point_t::_1
	};
	AggregateProducer downstream {
		"downstream_industry",
		downstream_process,
		fixed_point_t(4),
		fixed_point_t::_1
	};

	AggregateProducerMarketBridge upstream_bridge { upstream };
	AggregateProducerMarketBridge downstream_bridge { downstream };

	// Shock: upstream can operate at only half of its prior capacity.
	upstream.set_capacity(fixed_point_t(4));
	upstream.set_inventory(feedstock, fixed_point_t(8));

	const AggregateProductionResult upstream_result = upstream.produce();
	REQUIRE(upstream_result.actual_output == fixed_point_t(4));

	GoodMarket intermediate_market { rules, intermediate };
	transfer_intermediate(
		upstream_bridge,
		downstream_bridge,
		intermediate_market
	);

	const AggregateProductionResult downstream_result = downstream.produce();

	CHECK(downstream_result.desired_output == fixed_point_t(4));
	CHECK(downstream_result.actual_output == fixed_point_t(2));
	CHECK(downstream_result.input_limited);
}