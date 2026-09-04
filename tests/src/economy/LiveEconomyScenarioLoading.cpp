#include "openvic-simulation/economy/EconomyManager.hpp"

#include <filesystem>
#include <fstream>

#include "openvic-simulation/dataloader/Dataloader.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/population/PopManager.hpp"
#include "openvic-simulation/types/Colour.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Live economy scenario can be populated from mod-data text",
	"[economy][live-scenario][loader]"
) {
	EconomyManager economy;
	GameRulesManager rules;
	PopManager pops;

	GoodDefinitionManager& goods = economy.get_good_definition_manager();

	REQUIRE(goods.add_good_category("live", 3));
	GoodCategory const* category = goods.get_good_category_by_identifier("live");
	REQUIRE(category != nullptr);

	REQUIRE(goods.add_good_definition(
		"feedstock", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
		fixed_point_t::_1, true, true, false, false
	));
	REQUIRE(goods.add_good_definition(
		"intermediate", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
		fixed_point_t::_1, true, true, false, false
	));
	REQUIRE(goods.add_good_definition(
		"final", colour_rgb_t {}, *const_cast<GoodCategory*>(category),
		fixed_point_t::_1, true, true, false, false
	));

	GoodDefinition const* feedstock =
		goods.get_good_definition_by_identifier("feedstock");
	GoodDefinition const* intermediate =
		goods.get_good_definition_by_identifier("intermediate");
	GoodDefinition const* final_good =
		goods.get_good_definition_by_identifier("final");

	REQUIRE(feedstock != nullptr);
	REQUIRE(intermediate != nullptr);
	REQUIRE(final_good != nullptr);

	ProductionTypeManager& production =
		economy.get_production_type_manager();

	fixed_point_map_t<GoodDefinition const*> upstream_inputs;
	upstream_inputs.emplace(feedstock, fixed_point_t::_1);

	REQUIRE(production.add_production_type(
		rules,
		TypedSpan<pop_type_index_t, const PopType> {},
		"scenario_upstream",
		std::nullopt,
		memory::vector<Job> {},
		ProductionType::template_type_t::ARTISAN,
		pop_size_t { 1 },
		std::move(upstream_inputs),
		intermediate,
		fixed_point_t::_1,
		memory::vector<ProductionType::bonus_t> {},
		fixed_point_map_t<GoodDefinition const*> {},
		false,
		false,
		false
	));

	fixed_point_map_t<GoodDefinition const*> downstream_inputs;
	downstream_inputs.emplace(intermediate, fixed_point_t(2));

	REQUIRE(production.add_production_type(
		rules,
		TypedSpan<pop_type_index_t, const PopType> {},
		"scenario_downstream",
		std::nullopt,
		memory::vector<Job> {},
		ProductionType::template_type_t::ARTISAN,
		pop_size_t { 1 },
		std::move(downstream_inputs),
		final_good,
		fixed_point_t::_1,
		memory::vector<ProductionType::bonus_t> {},
		fixed_point_map_t<GoodDefinition const*> {},
		false,
		false,
		false
	));

	const std::filesystem::path path =
		std::filesystem::temp_directory_path() / "openvic-live-economy-loader-test.txt";

	{
		std::ofstream file { path };
		REQUIRE(file.good());
		file
			<< "upstream_process = scenario_upstream\n"
			<< "downstream_process = scenario_downstream\n"
			<< "upstream_capacity = 4\n"
			<< "upstream_utilization = 1\n"
			<< "downstream_capacity = 4\n"
			<< "downstream_utilization = 1\n"
			<< "source_inflow_good = feedstock\n"
			<< "source_inflow_per_daily_tick = 4\n"
			<< "source_node = 11\n"
			<< "destination_node = 22\n"
			<< "corridor_capacities = { 10 6 8 }\n";
	}

	const auto parser = Dataloader::parse_defines(path);
	REQUIRE(economy.load_live_economy_scenario_file(parser.get_file_node()));

	std::filesystem::remove(path);

	LiveEconomyScenarioDefinition const* scenario =
		economy.get_live_economy_scenario();

	REQUIRE(scenario != nullptr);
	REQUIRE(scenario->is_valid());

	CHECK(scenario->upstream_process->get_identifier() == "scenario_upstream");
	CHECK(scenario->downstream_process->get_identifier() == "scenario_downstream");
	CHECK(scenario->source_inflow_good->get_identifier() == "feedstock");
	CHECK(scenario->source_node == market_node_index_t { 11 });
	CHECK(scenario->destination_node == market_node_index_t { 22 });
	REQUIRE(scenario->corridor_legs.size() == 3);
	CHECK(scenario->corridor_legs[0].nominal_capacity == fixed_point_t(10));
	CHECK(scenario->corridor_legs[1].nominal_capacity == fixed_point_t(6));
	CHECK(scenario->corridor_legs[2].nominal_capacity == fixed_point_t(8));
}