#include "openvic-simulation/economy/trading/MarketNodeAccess.hpp"

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
	GoodCategory category { "market_node_goods", good_category_index_t { 0 } };

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
			"market_node_process",
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
	"Market node access distinguishes source destination direction",
	"[economy][market-node-access]"
) {
	MarketNodeAccessTable table {};

	const market_node_index_t source { 1 };
	const market_node_index_t destination { 2 };

	table.set_access(
		source,
		destination,
		DeliverableSupply {
			.physical_supply = fixed_point_t(10),
			.accessible_fraction = fixed_point_t::_0_50,
			.delivery_capacity = fixed_point_t(4),
			.access_allowed = true
		}
	);

	CHECK(table.has_access(source, destination));
	CHECK_FALSE(table.has_access(destination, source));

	CHECK(
		table.calculate_deliverable_quantity(source, destination)
		== fixed_point_t(4)
	);
	CHECK(
		table.calculate_deliverable_quantity(destination, source)
		== fixed_point_t::_0
	);
}

TEST_CASE(
	"Different destinations can have different access to identical source supply",
	"[economy][market-node-access][destination]"
) {
	MarketNodeAccessTable table {};

	const market_node_index_t source { 1 };
	const market_node_index_t destination_a { 2 };
	const market_node_index_t destination_b { 3 };

	table.set_access(
		source,
		destination_a,
		DeliverableSupply {
			.physical_supply = fixed_point_t(10),
			.accessible_fraction = fixed_point_t::_1,
			.delivery_capacity = fixed_point_t(8),
			.access_allowed = true
		}
	);

	table.set_access(
		source,
		destination_b,
		DeliverableSupply {
			.physical_supply = fixed_point_t(10),
			.accessible_fraction = fixed_point_t::_0_50,
			.delivery_capacity = fixed_point_t(3),
			.access_allowed = true
		}
	);

	CHECK(
		table.calculate_deliverable_quantity(source, destination_a)
		== fixed_point_t(8)
	);
	CHECK(
		table.calculate_deliverable_quantity(source, destination_b)
		== fixed_point_t(3)
	);
}

TEST_CASE(
	"Source destination access caps an aggregate producer market order",
	"[economy][market-node-access][market]"
) {
	ProductionType process = make_process();

	AggregateProducer buyer {
		"destination_industry",
		process,
		fixed_point_t(8),
		fixed_point_t::_1
	};
	AggregateProducerMarketBridge buyer_bridge { buyer };

	const market_node_index_t source { 10 };
	const market_node_index_t destination { 20 };

	MarketNodeAccessTable table {};
	table.set_access(
		source,
		destination,
		DeliverableSupply {
			.physical_supply = fixed_point_t(10),
			.accessible_fraction = fixed_point_t::_1,
			.delivery_capacity = fixed_point_t(3),
			.access_allowed = true
		}
	);

	const fixed_point_t deliverable =
		table.calculate_deliverable_quantity(source, destination);
	REQUIRE(deliverable == fixed_point_t(3));

	GoodMarket market { rules, input_good };
	Seller seller {};

	market.add_market_sell_order({
		std::nullopt,
		fixed_point_t(10),
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
	REQUIRE(buy_order->max_quantity == fixed_point_t(3));

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
	"Blocked source destination pair denies access without changing other pairs",
	"[economy][market-node-access][blocked]"
) {
	MarketNodeAccessTable table {};

	const market_node_index_t source_a { 1 };
	const market_node_index_t source_b { 2 };
	const market_node_index_t destination { 3 };

	table.set_access(
		source_a,
		destination,
		DeliverableSupply {
			.physical_supply = fixed_point_t(10),
			.accessible_fraction = fixed_point_t::_1,
			.delivery_capacity = fixed_point_t(10),
			.access_allowed = false
		}
	);

	table.set_access(
		source_b,
		destination,
		DeliverableSupply {
			.physical_supply = fixed_point_t(10),
			.accessible_fraction = fixed_point_t::_1,
			.delivery_capacity = fixed_point_t(6),
			.access_allowed = true
		}
	);

	CHECK(
		table.calculate_deliverable_quantity(source_a, destination)
		== fixed_point_t::_0
	);
	CHECK(
		table.calculate_deliverable_quantity(source_b, destination)
		== fixed_point_t(6)
	);
}

TEST_CASE(
	"Market node access lookup is stable under insertion order",
	"[economy][market-node-access][determinism]"
) {
	const market_node_index_t source_a { 1 };
	const market_node_index_t source_b { 2 };
	const market_node_index_t destination { 3 };

	DeliverableSupply access_a {
		.physical_supply = fixed_point_t(10),
		.accessible_fraction = fixed_point_t::_1,
		.delivery_capacity = fixed_point_t(4),
		.access_allowed = true
	};

	DeliverableSupply access_b {
		.physical_supply = fixed_point_t(10),
		.accessible_fraction = fixed_point_t::_1,
		.delivery_capacity = fixed_point_t(7),
		.access_allowed = true
	};

	MarketNodeAccessTable first {};
	first.set_access(source_a, destination, access_a);
	first.set_access(source_b, destination, access_b);

	MarketNodeAccessTable second {};
	second.set_access(source_b, destination, access_b);
	second.set_access(source_a, destination, access_a);

	CHECK(first.size() == second.size());
	CHECK(
		first.calculate_deliverable_quantity(source_a, destination)
		== second.calculate_deliverable_quantity(source_a, destination)
	);
	CHECK(
		first.calculate_deliverable_quantity(source_b, destination)
		== second.calculate_deliverable_quantity(source_b, destination)
	);
}