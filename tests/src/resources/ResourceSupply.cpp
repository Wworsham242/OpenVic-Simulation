#include "openvic-simulation/resources/ResourceSupply.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Resource supply separates nominal capability from accessible flow",
	"[resources][supply]"
) {
	ResourceSupplyState supply {
		.nominal_per_tick = fixed_point_t(8),
		.availability_fraction = fixed_point_t::_1
	};

	REQUIRE(supply.is_valid());
	CHECK(supply.accessible_per_tick() == fixed_point_t(8));

	REQUIRE(supply.set_availability_fraction(fixed_point_t::_0));
	CHECK(supply.accessible_per_tick() == fixed_point_t::_0);

	CHECK_FALSE(supply.set_availability_fraction(fixed_point_t(2)));
	CHECK(supply.availability_fraction == fixed_point_t::_0);
}