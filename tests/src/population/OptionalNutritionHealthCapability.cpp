#include "openvic-simulation/population/NutritionHealthBurden.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
"005A5 nutrition health capability is absent by default",
"[convergence][005a5][population][optional-capability]"
) {
OptionalNutritionHealthCapability capability {};

CHECK_FALSE(capability.is_enabled());
CHECK(capability.get_burden() == fixed_point_t::_0);
CHECK(capability.get_last_update_nullable() == nullptr);

CHECK_FALSE(
capability.update(
fixed_point_t::_1
)
);

CHECK(capability.get_burden() == fixed_point_t::_0);
CHECK(capability.get_last_update_nullable() == nullptr);
}

TEST_CASE(
"005A5 nutrition health capability can be explicitly enabled",
"[convergence][005a5][population][optional-capability]"
) {
OptionalNutritionHealthCapability capability {};

REQUIRE(capability.enable());
CHECK(capability.is_enabled());

REQUIRE(
capability.update(
fixed_point_t::_1
)
);

REQUIRE(
capability.get_last_update_nullable()
!= nullptr
);

CHECK(
capability.get_burden()
== fixed_point_t::_1
/ fixed_point_t {
NUTRITION_HEALTH_DETERIORATION_DAYS
}
);
}

TEST_CASE(
"005A5 nutrition capability preserves history only when enabled",
"[convergence][005a5][population][optional-capability][history]"
) {
OptionalNutritionHealthCapability capability {};

REQUIRE(capability.enable());

REQUIRE(
capability.update(
fixed_point_t::_1
)
);

const fixed_point_t first =
capability.get_burden();

REQUIRE(
capability.update(
fixed_point_t::_1
)
);

const fixed_point_t second =
capability.get_burden();

CHECK(second > first);
CHECK_FALSE(capability.enable());
}
