#include "openvic-simulation/economy/trading/DeliverableSupply.hpp"

#include <array>
#include <optional>

#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"
#include "openvic-simulation/economy/trading/GoodMarket.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	GoodCategory category { "deliverable_supply_goods", good_category_index_t { 0 } };

	GoodDefinition input_good {
		"input_good",
		colour_rgb_t {},
		good_index_t { 0 },
		category,
		1,
		true,
		true,
		false,
		false
	};

	GoodDefinition output_good {
		"output_good",
		colour_rgb_t {},
		good_index_t { 1 },
		category,
		1,
		true,
		true,
		false,
		false
	};

	GameRulesManager rules {};

	ProductionType make_process() {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&input_good, fixed_point_t::_1);

		return ProductionType {
			rules,
			"deliverable_supply_process",
			std::nullopt,
			{},
			ProductionType::template_type_t::FACTORY,
			pop_size_t { 0 },
			std::move(inputs),
			output_good,
			fixed_point_t::_1,
			{},
			{},
			false,
			false,
			false
		};
	}

	struct Seller {
		fixed_point_t sold = 0;

		static void after_sell(
			void* actor,
			SellResult const& result,
			memory::vector<fixed_point_t>&
		) {
			static_cast<Seller*>(actor)->sold += result.quantity_sold;
		}
	};

	void execute_market(GoodMarket& market) {
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
	"Deliverable supply combines physical availability access and delivery capacity",
	"[economy][deliverable-supply]"
) {
	DeliverableSupply envelope {
		.physical_supply = fixed_point_t(10),
		.accessible_fraction = fixed_point_t::_0_50,
		.delivery_capacity = fixed_point_t(3),
		.access_allowed = true
	};

	CHECK(envelope.calculate_deliverable_quantity() == fixed_point_t(3));

	envelope.delivery_capacity = fixed_point_t(8);
	CHECK(envelope.calculate_deliverable_quantity() == fixed_point_t(5));

	envelope.access_allowed = false;
	CHECK(envelope.calculate_deliverable_quantity() == fixed_point_t::_0);
}

TEST_CASE(
	"Deliverable supply clamps invalid access inputs",
	"[economy][deliverable-supply][validation]"
) {
	DeliverableSupply envelope {
		.physical_supply = fixed_point_t(10),
		.accessible_fraction = fixed_point_t(2),
		.delivery_capacity = fixed_point_t(20),
		.access_allowed = true
	};

	CHECK(envelope.calculate_deliverable_quantity() == fixed_point_t(10));

	envelope.accessible_fraction = fixed_point_t(-1);
	CHECK(envelope.calculate_deliverable_quantity() == fixed_point_t::_0);

	envelope.accessible_fraction = fixed_point_t::_1;
	envelope.delivery_capacity = fixed_point_t(-5);
	CHECK(envelope.calculate_deliverable_quantity() == fixed_point_t::_0);
}

TEST_CASE(
	"Market buy order is capped by deliverable rather than global physical supply",
	"[economy][deliverable-supply][market]"
) {
	ProductionType process = make_process();

	AggregateProducer buyer {
		"buyer_industry",
		process,
		fixed_point_t(8),
		fixed_point_t::_1
	};
	AggregateProducerMarketBridge buyer_bridge { buyer };

	GoodMarket market { rules, input_good };

	const fixed_point_t global_physical_supply = fixed_point_t(10);

	DeliverableSupply access {
		.physical_supply = global_physical_supply,
		.accessible_fraction = fixed_point_t::_0_50,
		.delivery_capacity = fixed_point_t(3),
		.access_allowed = true
	};

	const fixed_point_t deliverable = access.calculate_deliverable_quantity();
	REQUIRE(deliverable == fixed_point_t(3));

	Seller seller {};
	market.add_market_sell_order({
		std::nullopt,
		global_physical_supply,
		&seller,
		Seller::after_sell
	});

	const fixed_point_t shortfall =
		buyer_bridge.calculate_input_shortfall(input_good);
	REQUIRE(shortfall == fixed_point_t(8));

	auto buy_order = buyer_bridge.make_input_buy_order(
		input_good,
		shortfall * market.get_max_next_price(),
		std::nullopt,
		deliverable
	);
	REQUIRE(buy_order.has_value());
	CHECK(buy_order->max_quantity == fixed_point_t(3));

	market.add_buy_up_to_order(std::move(*buy_order));
	execute_market(market);
	buyer_bridge.clear_completed_orders();

	CHECK(buyer.get_inventory(input_good) == fixed_point_t(3));
	CHECK(seller.sold == fixed_point_t(3));

	const AggregateProductionResult result = buyer.produce();

	CHECK(result.desired_output == fixed_point_t(8));
	CHECK(result.actual_output == fixed_point_t(3));
	CHECK(result.input_limited);
}

TEST_CASE(
	"Denied access prevents market demand despite abundant physical supply",
	"[economy][deliverable-supply][denied]"
) {
	ProductionType process = make_process();

	AggregateProducer buyer {
		"buyer_industry",
		process,
		fixed_point_t(8),
		fixed_point_t::_1
	};
	AggregateProducerMarketBridge buyer_bridge { buyer };

	DeliverableSupply denied {
		.physical_supply = fixed_point_t(100),
		.accessible_fraction = fixed_point_t::_1,
		.delivery_capacity = fixed_point_t(100),
		.access_allowed = false
	};

	CHECK(denied.calculate_deliverable_quantity() == fixed_point_t::_0);

	auto buy_order = buyer_bridge.make_input_buy_order(
		input_good,
		fixed_point_t(1000),
		std::nullopt,
		denied.calculate_deliverable_quantity()
	);

	CHECK_FALSE(buy_order.has_value());
	CHECK(buyer.get_inventory(input_good) == fixed_point_t::_0);
}

TEST_CASE(
	"Access shock propagates through inventory into production without scripted penalty",
	"[economy][deliverable-supply][shock]"
) {
	ProductionType process = make_process();

	AggregateProducer buyer {
		"buyer_industry",
		process,
		fixed_point_t(8),
		fixed_point_t::_1
	};
	AggregateProducerMarketBridge buyer_bridge { buyer };

	GoodMarket market { rules, input_good };

	DeliverableSupply access {
		.physical_supply = fixed_point_t(8),
		.accessible_fraction = fixed_point_t::_1,
		.delivery_capacity = fixed_point_t(8),
		.access_allowed = true
	};

	// Route/port/access shock: only half the prior physical supply is deliverable.
	access.delivery_capacity = fixed_point_t(4);

	Seller seller {};
	market.add_market_sell_order({
		std::nullopt,
		fixed_point_t(8),
		&seller,
		Seller::after_sell
	});

	const fixed_point_t shortfall =
		buyer_bridge.calculate_input_shortfall(input_good);

	auto buy_order = buyer_bridge.make_input_buy_order(
		input_good,
		shortfall * market.get_max_next_price(),
		std::nullopt,
		access.calculate_deliverable_quantity()
	);
	REQUIRE(buy_order.has_value());

	market.add_buy_up_to_order(std::move(*buy_order));
	execute_market(market);
	buyer_bridge.clear_completed_orders();

	const AggregateProductionResult result = buyer.produce();

	CHECK(buyer.get_inventory(input_good) == fixed_point_t::_0);
	CHECK(result.desired_output == fixed_point_t(8));
	CHECK(result.actual_output == fixed_point_t(4));
	CHECK(result.input_limited);
}