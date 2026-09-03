#include "openvic-simulation/economy/production/AggregateProducer.hpp"

#include <optional>

#include "openvic-simulation/misc/GameRulesManager.hpp"
#include "openvic-simulation/types/Colour.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
	GoodCategory category { "aggregate_test_goods", good_category_index_t { 0 } };

	GoodDefinition oil {
		"oil", colour_rgb_t {}, good_index_t { 0 }, category, 1, true, true, false, false
	};

	GoodDefinition product {
		"product", colour_rgb_t {}, good_index_t { 1 }, category, 1, true, true, false, false
	};

	GameRulesManager rules {};

	ProductionType make_process(fixed_point_t input_per_output = fixed_point_t(2)) {
		fixed_point_map_t<GoodDefinition const*> inputs;
		inputs.emplace(&oil, input_per_output);
		return ProductionType {
			rules, "aggregate_test_process", std::nullopt, {},
			ProductionType::template_type_t::FACTORY, pop_size_t { 0 },
			std::move(inputs), product, fixed_point_t::_1, {}, {}, false, false, false
		};
	}
}

TEST_CASE("Aggregate producer converts capacity and utilization into output", "[economy][aggregate-production]") {
	ProductionType process = make_process();
	AggregateProducer producer { "regional_industry", process, fixed_point_t(16), fixed_point_t::_0_50 };
	producer.set_inventory(oil, fixed_point_t(100));

	auto const result = producer.produce();

	CHECK(result.desired_output == fixed_point_t(8));
	CHECK(result.actual_output == fixed_point_t(8));
	CHECK_FALSE(result.input_limited);
	CHECK(producer.get_inventory(oil) == fixed_point_t(84));
	CHECK(producer.get_inventory(product) == fixed_point_t(8));
}

TEST_CASE("Aggregate producer is constrained by scarce input inventory", "[economy][aggregate-production][scarcity]") {
	ProductionType process = make_process();
	AggregateProducer producer { "regional_industry", process, fixed_point_t(16), fixed_point_t::_0_50 };
	producer.set_inventory(oil, fixed_point_t(12));

	auto const result = producer.produce();

	CHECK(result.desired_output == fixed_point_t(8));
	CHECK(result.actual_output == fixed_point_t(6));
	CHECK(result.input_limited);
	CHECK(producer.get_inventory(oil) == fixed_point_t::_0);
	CHECK(producer.get_inventory(product) == fixed_point_t(6));
}

TEST_CASE("Aggregate producer uses the tightest multiple-input constraint", "[economy][aggregate-production][scarcity]") {
	GoodDefinition catalyst {
		"catalyst", colour_rgb_t {}, good_index_t { 2 }, category, 1, true, true, false, false
	};

	fixed_point_map_t<GoodDefinition const*> inputs;
	inputs.emplace(&oil, fixed_point_t(2));
	inputs.emplace(&catalyst, fixed_point_t::_0_50);

	ProductionType process {
		rules, "multi_input_process", std::nullopt, {},
		ProductionType::template_type_t::FACTORY, pop_size_t { 0 },
		std::move(inputs), product, fixed_point_t::_1, {}, {}, false, false, false
	};

	AggregateProducer producer { "regional_industry", process, fixed_point_t(10), fixed_point_t::_1 };
	producer.set_inventory(oil, fixed_point_t(20));
	producer.set_inventory(catalyst, fixed_point_t(3));

	auto const result = producer.produce();

	CHECK(result.desired_output == fixed_point_t(10));
	CHECK(result.actual_output == fixed_point_t(6));
	CHECK(result.input_limited);
	CHECK(producer.get_inventory(oil) == fixed_point_t(8));
	CHECK(producer.get_inventory(catalyst) == fixed_point_t::_0);
	CHECK(producer.get_inventory(product) == fixed_point_t(6));
}

TEST_CASE("Aggregate producer clamps capacity and utilization", "[economy][aggregate-production][validation]") {
	ProductionType process = make_process();
	AggregateProducer producer { "regional_industry", process, fixed_point_t(-10), fixed_point_t(2) };

	CHECK(producer.get_capacity() == fixed_point_t::_0);
	CHECK(producer.get_utilization() == fixed_point_t::_1);

	producer.set_capacity(fixed_point_t(10));
	producer.set_utilization(fixed_point_t(-1));

	CHECK(producer.get_capacity() == fixed_point_t(10));
	CHECK(producer.get_utilization() == fixed_point_t::_0);
	CHECK(producer.calculate_desired_output() == fixed_point_t::_0);
}

TEST_CASE("Aggregate producer cannot create output without required inputs", "[economy][aggregate-production][conservation]") {
	ProductionType process = make_process();
	AggregateProducer producer { "regional_industry", process, fixed_point_t(10), fixed_point_t::_1 };

	auto const result = producer.produce();

	CHECK(result.desired_output == fixed_point_t(10));
	CHECK(result.actual_output == fixed_point_t::_0);
	CHECK(result.input_limited);
	CHECK(producer.get_inventory(oil) == fixed_point_t::_0);
	CHECK(producer.get_inventory(product) == fixed_point_t::_0);
}