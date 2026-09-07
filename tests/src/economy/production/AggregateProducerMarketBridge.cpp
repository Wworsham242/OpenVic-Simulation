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
	GoodCategory category{"aggregate_market_goods",good_category_index_t{0}};
	GoodDefinition oil{"oil",colour_rgb_t{},good_index_t{0},category,1,true,true,false,false};
	GoodDefinition product{"product",colour_rgb_t{},good_index_t{1},category,1,true,true,false,false};
	GameRulesManager rules{};

	ProductionType make_process() {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&oil,fixed_point_t(2));
		return ProductionType{
			rules,"market_bridge_process",std::nullopt,{},
			ProductionType::template_type_t::FACTORY,pop_size_t{0},
			std::move(inputs),product,fixed_point_t::_1,{},{},false,false,false
		};
	}

	struct Seller {
		fixed_point_t sold=0;
		static void after_sell(void* actor,SellResult const& result,memory::vector<fixed_point_t>&){
			static_cast<Seller*>(actor)->sold += result.quantity_sold;
		}
	};

	struct Buyer {
		fixed_point_t bought=0;
		static void after_buy(void* actor,BuyResult const& result){
			static_cast<Buyer*>(actor)->bought += result.quantity_bought;
		}
	};

	void execute_market(GoodMarket& market) {
		TypedSpan<country_index_t,fixed_point_t> map0{};
		TypedSpan<country_index_t,fixed_point_t> map1{};
		std::array<memory::vector<fixed_point_t>,GoodMarket::VECTORS_FOR_EXECUTE_ORDERS> vectors;
		market.execute_orders(map0,map1,vectors);
	}
}

TEST_CASE("Aggregate bridge calculates physical shortfall","[economy][aggregate-market]") {
	auto process=make_process();
	AggregateProducer producer{"refining",process,fixed_point_t(8),fixed_point_t::_0_50};
	AggregateProducerMarketBridge bridge{producer};

	CHECK(producer.calculate_desired_output()==fixed_point_t(4));
	CHECK(bridge.calculate_input_shortfall(oil)==fixed_point_t(8));
	producer.set_inventory(oil,fixed_point_t(3));
	CHECK(bridge.calculate_input_shortfall(oil)==fixed_point_t(5));
	producer.set_inventory(oil,fixed_point_t(8));
	CHECK(bridge.calculate_input_shortfall(oil)==fixed_point_t::_0);
	CHECK(bridge.calculate_input_shortfall(product)==fixed_point_t::_0);
}

TEST_CASE("Market-cleared input purchase becomes aggregate inventory","[economy][aggregate-market][buy]") {
	auto process=make_process();
	AggregateProducer producer{"refining",process,fixed_point_t(8),fixed_point_t::_0_50};
	AggregateProducerMarketBridge bridge{producer};
	GoodMarket market{rules,oil};

	const fixed_point_t shortfall=bridge.calculate_input_shortfall(oil);
	Seller seller{};
	market.add_market_sell_order({std::nullopt,shortfall,&seller,Seller::after_sell});

	auto order=bridge.make_input_buy_order(oil,shortfall*market.get_max_next_price());
	REQUIRE(order.has_value());
	market.add_buy_up_to_order(std::move(*order));
	execute_market(market);

	CHECK(producer.get_inventory(oil)==shortfall);
	CHECK(seller.sold==shortfall);
	CHECK(bridge.get_money_spent_on_inputs()>0);
	bridge.clear_completed_orders();
	CHECK(bridge.get_pending_buy_count()==0);
}

TEST_CASE("Partial fill constrains subsequent aggregate production","[economy][aggregate-market][scarcity]") {
	auto process=make_process();
	AggregateProducer producer{"refining",process,fixed_point_t(8),fixed_point_t::_0_50};
	AggregateProducerMarketBridge bridge{producer};
	GoodMarket market{rules,oil};

	const fixed_point_t offered=fixed_point_t(3);
	Seller seller{};
	market.add_market_sell_order({std::nullopt,offered,&seller,Seller::after_sell});

	const fixed_point_t wanted=bridge.calculate_input_shortfall(oil);
	auto order=bridge.make_input_buy_order(oil,wanted*market.get_max_next_price());
	REQUIRE(order.has_value());
	market.add_buy_up_to_order(std::move(*order));
	execute_market(market);

	CHECK(producer.get_inventory(oil)==offered);
	auto production=producer.produce();
	CHECK(production.desired_output==fixed_point_t(4));
	CHECK(production.actual_output==fixed_point_t::_1_50);
	CHECK(production.input_limited);
}

TEST_CASE("Market-cleared output sale removes only actual sold quantity","[economy][aggregate-market][sell]") {
	auto process=make_process();
	AggregateProducer producer{"refining",process,fixed_point_t(8),fixed_point_t::_0_50};
	AggregateProducerMarketBridge bridge{producer};

	producer.set_inventory(oil,fixed_point_t(8));
	auto production=producer.produce();
	REQUIRE(production.actual_output==fixed_point_t(4));

	GoodMarket market{rules,product};
	Buyer buyer{};
	const fixed_point_t wanted=fixed_point_t(2);
	market.add_buy_up_to_order({
		std::nullopt,wanted,wanted*market.get_max_next_price(),&buyer,Buyer::after_buy
	});

	auto sell_order=bridge.make_output_sell_order();
	REQUIRE(sell_order.has_value());
	market.add_market_sell_order(std::move(*sell_order));
	execute_market(market);

	CHECK(buyer.bought==wanted);
	CHECK(producer.get_inventory(product)==fixed_point_t(2));
	CHECK(bridge.get_money_received_from_sales()>0);
	bridge.clear_completed_orders();
	CHECK(bridge.get_pending_sell_count()==0);
}

TEST_CASE("Aggregate bridge rejects meaningless orders","[economy][aggregate-market][validation]") {
	auto process=make_process();
	AggregateProducer producer{"refining",process,fixed_point_t(8),fixed_point_t::_0_50};
	AggregateProducerMarketBridge bridge{producer};

	CHECK_FALSE(bridge.make_input_buy_order(product,fixed_point_t(100)).has_value());
	CHECK_FALSE(bridge.make_input_buy_order(oil,fixed_point_t::_0).has_value());
	CHECK_FALSE(bridge.make_output_sell_order().has_value());
	producer.set_inventory(oil,fixed_point_t(8));
	CHECK_FALSE(bridge.make_input_buy_order(oil,fixed_point_t(100)).has_value());
}
TEST_CASE("Aggregate bridge retains its own requested and completed transaction quantities",
	"[economy][aggregate-market][provenance]") {
	auto process = make_process();
	AggregateProducer producer { "buyer", process, 4, 1 };
	AggregateProducerMarketBridge bridge { producer };
	GoodMarket market { rules, oil };
	auto order = bridge.make_input_buy_order(oil, 100);
	REQUIRE(order.has_value());
	market.add_buy_up_to_order(std::move(*order));
	execute_market(market); // No seller can fill this order.
	bridge.clear_completed_orders();
	auto result = bridge.get_cycle_result();
	CHECK(result.input_requested == fixed_point_t { 8 });
	CHECK(result.input_ordered == fixed_point_t { 8 });
	CHECK(result.input_bought == fixed_point_t::_0);
	CHECK(result.transaction_limited());
	CHECK(result.money_spent == fixed_point_t::_0);
	bridge.reset_cycle_result();
	CHECK(bridge.get_cycle_result() == AggregateMarketCycleResult {});
}
