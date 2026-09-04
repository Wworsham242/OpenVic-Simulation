#pragma once

#include <algorithm>
#include <deque>
#include <optional>

#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/trading/BuyResult.hpp"
#include "openvic-simulation/economy/trading/BuyUpToOrder.hpp"
#include "openvic-simulation/economy/trading/MarketSellOrder.hpp"
#include "openvic-simulation/economy/trading/SellResult.hpp"

namespace OpenVic {

class AggregateProducerMarketBridge final {
private:
	struct pending_buy_t {
		AggregateProducerMarketBridge* bridge = nullptr;
		GoodDefinition const* good = nullptr;
		bool completed = false;
	};
	struct pending_sell_t {
		AggregateProducerMarketBridge* bridge = nullptr;
		GoodDefinition const* good = nullptr;
		bool completed = false;
	};

	AggregateProducer& producer;
	std::deque<pending_buy_t> pending_buys;
	std::deque<pending_sell_t> pending_sells;
	fixed_point_t money_spent_on_inputs = 0;
	fixed_point_t money_spent_on_imports = 0;
	fixed_point_t money_received_from_sales = 0;

	static void after_input_buy(void* actor, BuyResult const& result) {
		auto& pending = *static_cast<pending_buy_t*>(actor);
		pending.bridge->producer.add_inventory(*pending.good, result.quantity_bought);
		pending.bridge->money_spent_on_inputs += result.money_spent_total;
		pending.bridge->money_spent_on_imports += result.money_spent_on_imports;
		pending.completed = true;
	}

	static void after_output_sell(void* actor, SellResult const& result, memory::vector<fixed_point_t>&) {
		auto& pending = *static_cast<pending_sell_t*>(actor);
		pending.bridge->producer.add_inventory(*pending.good, -result.quantity_sold);
		pending.bridge->money_received_from_sales += result.money_gained;
		pending.completed = true;
	}

	[[nodiscard]] std::optional<fixed_point_t> get_input_per_output(GoodDefinition const& good) const {
		auto const& inputs = producer.get_production_type().input_goods;
		auto const it = inputs.find(&good);
		if(it == inputs.end() || it->second <= 0){return std::nullopt;}
		return it->second;
	}

public:
	explicit AggregateProducerMarketBridge(AggregateProducer& new_producer) : producer{new_producer} {}

	[[nodiscard]] fixed_point_t calculate_input_shortfall(GoodDefinition const& good) const {
		auto const input_per_output = get_input_per_output(good);
		if(!input_per_output.has_value()){return 0;}
		const fixed_point_t required = *input_per_output * producer.calculate_desired_output();
		return std::max(required - producer.get_inventory(good), fixed_point_t::_0);
	}

	[[nodiscard]] std::optional<BuyUpToOrder> make_input_buy_order(
		GoodDefinition const& good,
		fixed_point_t money_to_spend,
		std::optional<country_index_t> country_index_optional = std::nullopt,
		std::optional<fixed_point_t> max_deliverable_quantity = std::nullopt
	) {
		fixed_point_t order_quantity = calculate_input_shortfall(good);

		if (max_deliverable_quantity.has_value()) {
			order_quantity = std::min(
				order_quantity,
				std::max(*max_deliverable_quantity, fixed_point_t::_0)
			);
		}

		if(order_quantity <= 0 || money_to_spend <= 0){return std::nullopt;}
		pending_buys.push_back({this,&good,false});
		auto& pending = pending_buys.back();
		return BuyUpToOrder{
			good.index,country_index_optional,order_quantity,money_to_spend,&pending,after_input_buy
		};
	}

	[[nodiscard]] std::optional<MarketSellOrder> make_output_sell_order(
		std::optional<fixed_point_t> max_quantity = std::nullopt,
		std::optional<country_index_t> country_index_optional = std::nullopt
	) {
		GoodDefinition const& output_good = producer.get_production_type().output_good;
		fixed_point_t quantity = producer.get_inventory(output_good);
		if(max_quantity.has_value()){
			quantity = std::min(quantity,std::max(*max_quantity,fixed_point_t::_0));
		}
		if(quantity <= 0){return std::nullopt;}
		pending_sells.push_back({this,&output_good,false});
		auto& pending = pending_sells.back();
		return MarketSellOrder{
			output_good.index,country_index_optional,quantity,&pending,after_output_sell
		};
	}

	void clear_completed_orders() {
		std::erase_if(pending_buys,[](pending_buy_t const& p){return p.completed;});
		std::erase_if(pending_sells,[](pending_sell_t const& p){return p.completed;});
	}

	[[nodiscard]] size_t get_pending_buy_count() const { return pending_buys.size(); }
	[[nodiscard]] size_t get_pending_sell_count() const { return pending_sells.size(); }
	[[nodiscard]] fixed_point_t get_money_spent_on_inputs() const { return money_spent_on_inputs; }
	[[nodiscard]] fixed_point_t get_money_spent_on_imports() const { return money_spent_on_imports; }
	[[nodiscard]] fixed_point_t get_money_received_from_sales() const { return money_received_from_sales; }
};

}