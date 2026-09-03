#include "openvic-simulation/country/FiscalCollectionCapacity.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("Fiscal collection capacity defaults to legacy-neutral full realization", "[vertical][nation][fiscal][compatibility]") {
	FiscalCollectionCapacity capacity;

	CHECK(capacity.get_tax_base_coverage() == fixed_point_t::_1);
	CHECK(capacity.get_compliance_rate() == fixed_point_t::_1);
	CHECK(capacity.get_collection_execution() == fixed_point_t::_1);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_1);
}

TEST_CASE("Fiscal collection bottlenecks compose multiplicatively", "[vertical][nation][fiscal][capacity]") {
	FiscalCollectionCapacity capacity;

	capacity.set_tax_base_coverage(fixed_point_t::_0_50);
	capacity.set_compliance_rate(fixed_point_t::_0_50);
	capacity.set_collection_execution(fixed_point_t::_0_50);

	CHECK(capacity.get_realization_factor() == fixed_point_t(1) / 8);
}

TEST_CASE("Fiscal collection dimensions remain causally distinct", "[vertical][nation][fiscal][capacity]") {
	FiscalCollectionCapacity capacity;

	capacity.set_tax_base_coverage(fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0_50);

	capacity.set_tax_base_coverage(fixed_point_t::_1);
	capacity.set_compliance_rate(fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0_50);

	capacity.set_compliance_rate(fixed_point_t::_1);
	capacity.set_collection_execution(fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0_50);
}

TEST_CASE("Fiscal collection inputs clamp to valid unit range", "[vertical][nation][fiscal][validation]") {
	FiscalCollectionCapacity capacity;

	capacity.set_tax_base_coverage(fixed_point_t(2));
	capacity.set_compliance_rate(fixed_point_t(-1));
	capacity.set_collection_execution(fixed_point_t(3));

	CHECK(capacity.get_tax_base_coverage() == fixed_point_t::_1);
	CHECK(capacity.get_compliance_rate() == fixed_point_t::_0);
	CHECK(capacity.get_collection_execution() == fixed_point_t::_1);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0);
}

TEST_CASE("Fiscal realization is reactive to capacity changes", "[vertical][nation][fiscal][reactive]") {
	FiscalCollectionCapacity capacity;

	CHECK(capacity.get_realization_factor() == fixed_point_t::_1);

	capacity.set_compliance_rate(fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0_50);

	capacity.set_collection_execution(fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0_50 * fixed_point_t::_0_50);
}