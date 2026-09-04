#include "openvic-simulation/economy/trading/TransportCorridor.hpp"

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
	GoodCategory category { "corridor_goods", good_category_index_t { 0 } };

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
			"corridor_process",
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
	"Transport corridor capacity is set by the tightest leg",
	"[economy][transport-corridor]"
) {
	TransportCorridor corridor {
		market_node_index_t { 1 },
		market_node_index_t { 2 }
	};

	corridor.add_leg({
		.nominal_capacity = fixed_point_t(10),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(6),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(8),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	CHECK(corridor.calculate_bottleneck_capacity() == fixed_point_t(6));
}

TEST_CASE(
	"Transport leg degradation reduces corridor capacity",
	"[economy][transport-corridor][degradation]"
) {
	TransportCorridor corridor {
		market_node_index_t { 1 },
		market_node_index_t { 2 }
	};

	corridor.add_leg({
		.nominal_capacity = fixed_point_t(10),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(8),
		.availability_fraction = fixed_point_t::_0_50,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(9),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	CHECK(corridor.calculate_bottleneck_capacity() == fixed_point_t(4));
}

TEST_CASE(
	"Closed transport leg closes the corridor",
	"[economy][transport-corridor][closure]"
) {
	TransportCorridor corridor {
		market_node_index_t { 1 },
		market_node_index_t { 2 }
	};

	corridor.add_leg({
		.nominal_capacity = fixed_point_t(10),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(10),
		.availability_fraction = fixed_point_t::_1,
		.open = false
	});

	CHECK(corridor.calculate_bottleneck_capacity() == fixed_point_t::_0);
}

TEST_CASE(
	"Corridor publishes directional deliverable supply to market node access",
	"[economy][transport-corridor][market-node-access]"
) {
	const market_node_index_t source { 10 };
	const market_node_index_t destination { 20 };

	TransportCorridor corridor { source, destination };
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(12),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(5),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	MarketNodeAccessTable table {};
	corridor.publish_access(
		table,
		fixed_point_t(20),
		fixed_point_t::_1,
		true
	);

	CHECK(table.has_access(source, destination));
	CHECK_FALSE(table.has_access(destination, source));
	CHECK(
		table.calculate_deliverable_quantity(source, destination)
		== fixed_point_t(5)
	);
}

TEST_CASE(
	"Corridor bottleneck constrains a real market purchase and production",
	"[economy][transport-corridor][integration]"
) {
	ProductionType process = make_process();

	AggregateProducer buyer {
		"destination_industry",
		process,
		fixed_point_t(10),
		fixed_point_t::_1
	};
	AggregateProducerMarketBridge buyer_bridge { buyer };

	const market_node_index_t source { 1 };
	const market_node_index_t destination { 2 };

	TransportCorridor corridor { source, destination };
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(10),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(4),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	MarketNodeAccessTable table {};
	corridor.publish_access(
		table,
		fixed_point_t(10)
	);

	const fixed_point_t deliverable =
		table.calculate_deliverable_quantity(source, destination);
	REQUIRE(deliverable == fixed_point_t(4));

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
	REQUIRE(shortfall == fixed_point_t(10));

	auto buy_order = buyer_bridge.make_input_buy_order(
		input_good,
		shortfall * market.get_max_next_price(),
		std::nullopt,
		deliverable
	);
	REQUIRE(buy_order.has_value());
	REQUIRE(buy_order->max_quantity == fixed_point_t(4));

	market.add_buy_up_to_order(std::move(*buy_order));
	execute_market(market);
	buyer_bridge.clear_completed_orders();

	CHECK(buyer.get_inventory(input_good) == fixed_point_t(4));
	CHECK(seller.sold == fixed_point_t(4));

	const AggregateProductionResult result = buyer.produce();

	CHECK(result.desired_output == fixed_point_t(10));
	CHECK(result.actual_output == fixed_point_t(4));
	CHECK(result.input_limited);
}

TEST_CASE(
	"Physical shortage remains binding when below corridor capacity",
	"[economy][transport-corridor][physical-shortage]"
) {
	const market_node_index_t source { 1 };
	const market_node_index_t destination { 2 };

	TransportCorridor corridor { source, destination };
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(10),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});
	corridor.add_leg({
		.nominal_capacity = fixed_point_t(8),
		.availability_fraction = fixed_point_t::_1,
		.open = true
	});

	const DeliverableSupply supply =
		corridor.make_deliverable_supply(fixed_point_t(3));

	CHECK(supply.delivery_capacity == fixed_point_t(8));
	CHECK(supply.calculate_deliverable_quantity() == fixed_point_t(3));
}