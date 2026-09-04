#pragma once

#include <optional>

#include "openvic-simulation/economy/GoodInstance.hpp"
#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/AggregateProducerMarketBridge.hpp"
#include "openvic-simulation/economy/trading/MarketNodeAccess.hpp"
#include "openvic-simulation/economy/trading/TransportCorridor.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"

namespace OpenVic {

struct LiveEconomyStatus final {
	bool configured = false;
	uint64_t completed_daily_ticks = 0;

	fixed_point_t upstream_output = 0;
	fixed_point_t downstream_desired_output = 0;
	fixed_point_t downstream_output = 0;
	bool downstream_input_limited = false;

	fixed_point_t intermediate_upstream_inventory = 0;
	fixed_point_t intermediate_downstream_inventory = 0;
	fixed_point_t final_inventory = 0;

	fixed_point_t corridor_capacity = 0;
	fixed_point_t deliverable_intermediate = 0;

	fixed_point_t intermediate_price = 0;
	fixed_point_t intermediate_supply_yesterday = 0;
	fixed_point_t intermediate_demand_yesterday = 0;
	fixed_point_t intermediate_quantity_traded_yesterday = 0;
};

/// Small authoritative live-economy bootstrap used to move aggregate mechanisms
/// out of isolated tests and into the real campaign tick.
///
/// This is intentionally a bootstrap scenario, not the final world economy
/// loader. It uses real loaded goods and the real GoodMarket instance.
class LiveEconomyRuntime final {
private:
	GameRulesManager const& game_rules_manager;
	GoodInstanceManager& good_instance_manager;

	GoodDefinition const& feedstock_good;
	GoodDefinition const& intermediate_good;
	GoodDefinition const& final_good;

	ProductionType upstream_process;
	ProductionType downstream_process;

	AggregateProducer upstream;
	AggregateProducer downstream;

	AggregateProducerMarketBridge upstream_bridge;
	AggregateProducerMarketBridge downstream_bridge;

	MarketNodeAccessTable access_table;
	TransportCorridor corridor;

	LiveEconomyStatus status {};

	[[nodiscard]] static ProductionType make_upstream_process(
		GameRulesManager const& rules,
		GoodDefinition const& feedstock,
		GoodDefinition const& intermediate
	) {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&feedstock, fixed_point_t::_1);

		return ProductionType {
			rules,
			"live_bootstrap_upstream",
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

	[[nodiscard]] static ProductionType make_downstream_process(
		GameRulesManager const& rules,
		GoodDefinition const& intermediate,
		GoodDefinition const& final_output
	) {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&intermediate, fixed_point_t(2));

		return ProductionType {
			rules,
			"live_bootstrap_downstream",
			std::nullopt,
			{},
			ProductionType::template_type_t::FACTORY,
			pop_size_t { 0 },
			std::move(inputs),
			final_output,
			fixed_point_t::_1,
			{},
			{},
			false,
			false,
			false
		};
	}

	void refresh_status_from_market() {
		GoodInstance& market = good_instance_manager.get_good_instance_by_definition(intermediate_good);

		status.intermediate_upstream_inventory =
			upstream.get_inventory(intermediate_good);
		status.intermediate_downstream_inventory =
			downstream.get_inventory(intermediate_good);
		status.final_inventory =
			downstream.get_inventory(final_good);

		status.corridor_capacity =
			corridor.calculate_bottleneck_capacity();
		status.deliverable_intermediate =
			access_table.calculate_deliverable_quantity(
				corridor.get_source_node(),
				corridor.get_destination_node()
			);

		status.intermediate_price = market.get_price();
		status.intermediate_supply_yesterday = market.get_total_supply_yesterday();
		status.intermediate_demand_yesterday = market.get_total_demand_yesterday();
		status.intermediate_quantity_traded_yesterday =
			market.get_quantity_traded_yesterday();
	}

public:
	LiveEconomyRuntime(
		GameRulesManager const& new_game_rules_manager,
		GoodInstanceManager& new_good_instance_manager,
		GoodDefinition const& new_feedstock_good,
		GoodDefinition const& new_intermediate_good,
		GoodDefinition const& new_final_good
	) : game_rules_manager { new_game_rules_manager },
		good_instance_manager { new_good_instance_manager },
		feedstock_good { new_feedstock_good },
		intermediate_good { new_intermediate_good },
		final_good { new_final_good },
		upstream_process {
			make_upstream_process(
				new_game_rules_manager,
				new_feedstock_good,
				new_intermediate_good
			)
		},
		downstream_process {
			make_downstream_process(
				new_game_rules_manager,
				new_intermediate_good,
				new_final_good
			)
		},
		upstream {
			"live_bootstrap_upstream",
			upstream_process,
			fixed_point_t(4),
			fixed_point_t::_1
		},
		downstream {
			"live_bootstrap_downstream",
			downstream_process,
			fixed_point_t(4),
			fixed_point_t::_1
		},
		upstream_bridge { upstream },
		downstream_bridge { downstream },
		corridor {
			market_node_index_t { 0 },
			market_node_index_t { 1 }
		} {

		corridor.add_leg({
			.nominal_capacity = fixed_point_t(4),
			.availability_fraction = fixed_point_t::_1,
			.open = true
		});

		status.configured = true;
		refresh_status_from_market();
	}

	/// Pre-market phase of one live daily economy cycle.
	///
	/// Bootstrap raw supply is injected as a stand-in for a future resource
	/// extraction vertical. Everything after that uses real production,
	/// corridor/access logic, and the real loaded intermediate GoodMarket.
	void pre_market_daily_tick() {
		upstream.add_inventory(feedstock_good, fixed_point_t(4));

		const AggregateProductionResult upstream_result = upstream.produce();
		status.upstream_output = upstream_result.actual_output;

		const fixed_point_t physical_intermediate =
			upstream.get_inventory(intermediate_good);

		corridor.publish_access(
			access_table,
			physical_intermediate
		);

		const fixed_point_t deliverable =
			access_table.calculate_deliverable_quantity(
				corridor.get_source_node(),
				corridor.get_destination_node()
			);

		GoodInstance& market =
			good_instance_manager.get_good_instance_by_definition(intermediate_good);

		if (auto sell_order = upstream_bridge.make_output_sell_order();
			sell_order.has_value()) {
			market.add_market_sell_order(std::move(*sell_order));
		}

		const fixed_point_t shortfall =
			downstream_bridge.calculate_input_shortfall(intermediate_good);

		if (auto buy_order = downstream_bridge.make_input_buy_order(
				intermediate_good,
				shortfall * market.get_max_next_price(),
				std::nullopt,
				deliverable
			); buy_order.has_value()) {
			market.add_buy_up_to_order(std::move(*buy_order));
		}

		refresh_status_from_market();
	}

	/// Post-market phase after MarketInstance::execute_orders().
	void post_market_daily_tick() {
		upstream_bridge.clear_completed_orders();
		downstream_bridge.clear_completed_orders();

		const AggregateProductionResult downstream_result = downstream.produce();

		status.downstream_desired_output =
			downstream_result.desired_output;
		status.downstream_output =
			downstream_result.actual_output;
		status.downstream_input_limited =
			downstream_result.input_limited;

		++status.completed_daily_ticks;
		refresh_status_from_market();
	}

	[[nodiscard]] LiveEconomyStatus get_status() const {
		return status;
	}
};

}