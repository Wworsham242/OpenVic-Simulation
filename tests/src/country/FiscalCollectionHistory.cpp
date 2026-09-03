#include "openvic-simulation/country/FiscalCollectionHistory.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE("Empty fiscal history preserves capacity", "[vertical][nation][fiscal][history]") {
	FiscalCollectionCapacity capacity;
	capacity.set_tax_base_coverage(fixed_point_t::_0_50);
	capacity.set_compliance_rate(fixed_point_t::_0_50);

	FiscalCollectionHistory const entry {};
	CHECK(entry.empty());

	entry.apply_to(capacity);

	CHECK(capacity.get_tax_base_coverage() == fixed_point_t::_0_50);
	CHECK(capacity.get_compliance_rate() == fixed_point_t::_0_50);
	CHECK(capacity.get_collection_execution() == fixed_point_t::_1);
}

TEST_CASE("Fiscal history can initialize all dimensions", "[vertical][nation][fiscal][history]") {
	FiscalCollectionCapacity capacity;

	FiscalCollectionHistory const entry {
		.tax_base_coverage = fixed_point_t::_0_50,
		.compliance_rate = fixed_point_t::_0_50,
		.collection_execution = fixed_point_t::_0_50
	};

	CHECK_FALSE(entry.empty());
	entry.apply_to(capacity);

	CHECK(capacity.get_tax_base_coverage() == fixed_point_t::_0_50);
	CHECK(capacity.get_compliance_rate() == fixed_point_t::_0_50);
	CHECK(capacity.get_collection_execution() == fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t(1) / 8);
}

TEST_CASE("Later fiscal history can override one dimension only", "[vertical][nation][fiscal][history][dated]") {
	FiscalCollectionCapacity capacity;

	FiscalCollectionHistory const initial {
		.tax_base_coverage = fixed_point_t::_0_50,
		.compliance_rate = fixed_point_t::_0_50,
		.collection_execution = fixed_point_t::_0_50
	};
	initial.apply_to(capacity);

	FiscalCollectionHistory const later {
		.tax_base_coverage = fixed_point_t::_1
	};
	later.apply_to(capacity);

	CHECK(capacity.get_tax_base_coverage() == fixed_point_t::_1);
	CHECK(capacity.get_compliance_rate() == fixed_point_t::_0_50);
	CHECK(capacity.get_collection_execution() == fixed_point_t::_0_50);
	CHECK(capacity.get_realization_factor() == fixed_point_t::_0_50 * fixed_point_t::_0_50);
}

TEST_CASE("History application inherits capacity clamping", "[vertical][nation][fiscal][history][validation]") {
	FiscalCollectionCapacity capacity;

	FiscalCollectionHistory const entry {
		.tax_base_coverage = fixed_point_t(2),
		.compliance_rate = fixed_point_t(-1),
		.collection_execution = fixed_point_t(3)
	};
	entry.apply_to(capacity);

	CHECK(capacity.get_tax_base_coverage() == fixed_point_t::_1);
	CHECK(capacity.get_compliance_rate() == fixed_point_t::_0);
	CHECK(capacity.get_collection_execution() == fixed_point_t::_1);
}