#include "openvic-simulation/environment/ProvinceSoilWaterBalance.hpp"

#include "openvic-simulation/economy/production/AgriculturalProductionConstraint.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
    "006B2.1 zero forcing preserves soil water storage",
    "[convergence][006b2][006b2.1][environment][soil-water]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 60 }
    };

    auto const result =
        advance_province_soil_water_balance(starting, {});

    REQUIRE(result.valid());
    CHECK(result.ending == starting);
    CHECK(
        result.environmental_state.get_water_availability()
        == (fixed_point_t { 60 } / fixed_point_t { 100 })
    );
}

TEST_CASE(
    "006B2.1 effective precipitation recharges persistent soil water",
    "[convergence][006b2][006b2.1][environment][soil-water][precipitation]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 40 }
    };

    ProvinceSoilWaterForcing const forcing {
        .precipitation = fixed_point_t { 30 },
        .surface_runoff = fixed_point_t { 5 },
        .evapotranspiration_demand = fixed_point_t::_0
    };

    auto const result =
        advance_province_soil_water_balance(starting, forcing);

    REQUIRE(result.valid());
    CHECK(result.effective_precipitation == fixed_point_t { 25 });
    CHECK(result.ending.storage == fixed_point_t { 65 });
}

TEST_CASE(
    "006B2.1 evapotranspiration depletes stored water",
    "[convergence][006b2][006b2.1][environment][soil-water][evapotranspiration]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 80 }
    };

    ProvinceSoilWaterForcing const forcing {
        .evapotranspiration_demand = fixed_point_t { 25 }
    };

    auto const result =
        advance_province_soil_water_balance(starting, forcing);

    REQUIRE(result.valid());
    CHECK(result.actual_evapotranspiration == fixed_point_t { 25 });
    CHECK(result.evapotranspiration_deficit == fixed_point_t::_0);
    CHECK(result.ending.storage == fixed_point_t { 55 });
}

TEST_CASE(
    "006B2.1 dry soil exposes unmet evapotranspiration demand",
    "[convergence][006b2][006b2.1][environment][soil-water][deficit]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 10 }
    };

    ProvinceSoilWaterForcing const forcing {
        .evapotranspiration_demand = fixed_point_t { 25 }
    };

    auto const result =
        advance_province_soil_water_balance(starting, forcing);

    REQUIRE(result.valid());
    CHECK(result.actual_evapotranspiration == fixed_point_t { 10 });
    CHECK(result.evapotranspiration_deficit == fixed_point_t { 15 });
    CHECK(result.ending.storage == fixed_point_t::_0);
}

TEST_CASE(
    "006B2.1 excess water drains only after current interval evapotranspiration",
    "[convergence][006b2][006b2.1][environment][soil-water][drainage]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 90 }
    };

    ProvinceSoilWaterForcing const forcing {
        .precipitation = fixed_point_t { 50 },
        .evapotranspiration_demand = fixed_point_t { 20 }
    };

    auto const result =
        advance_province_soil_water_balance(starting, forcing);

    REQUIRE(result.valid());
    CHECK(result.actual_evapotranspiration == fixed_point_t { 20 });
    CHECK(result.deep_drainage == fixed_point_t { 20 });
    CHECK(result.ending.storage == fixed_point_t { 100 });
}

TEST_CASE(
    "006B2.1 repeated dry intervals preserve drought memory",
    "[convergence][006b2][006b2.1][environment][soil-water][memory]"
) {
    ProvinceSoilWaterState const initial {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 100 }
    };

    ProvinceSoilWaterForcing const dry {
        .evapotranspiration_demand = fixed_point_t { 25 }
    };

    auto const first =
        advance_province_soil_water_balance(initial, dry);

    REQUIRE(first.valid());
    CHECK(first.ending.storage == fixed_point_t { 75 });

    auto const second =
        advance_province_soil_water_balance(first.ending, dry);

    REQUIRE(second.valid());
    CHECK(second.ending.storage == fixed_point_t { 50 });
    CHECK(
        second.environmental_state.get_water_availability()
        == fixed_point_t::_0_50
    );
}

TEST_CASE(
    "006B2.1 rainfall recovery need not immediately restore full water availability",
    "[convergence][006b2][006b2.1][environment][soil-water][recovery]"
) {
    ProvinceSoilWaterState const drought {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 30 }
    };

    ProvinceSoilWaterForcing const recovery {
        .precipitation = fixed_point_t { 25 },
        .evapotranspiration_demand = fixed_point_t { 10 }
    };

    auto const result =
        advance_province_soil_water_balance(drought, recovery);

    REQUIRE(result.valid());
    CHECK(result.ending.storage == fixed_point_t { 45 });
    CHECK(
        result.environmental_state.get_water_availability()
        < fixed_point_t::_1
    );
}

TEST_CASE(
    "006B2.1 derived physical state feeds existing agricultural constraint",
    "[convergence][006b2][006b2.1][environment][agriculture][bridge]"
) {
    ProvinceSoilWaterState const state {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 50 }
    };

    auto const environment =
        state.to_environmental_state();

    auto const agriculture =
        constrain_agricultural_production(
            environment,
            fixed_point_t { 8 }
        );

    CHECK(
        environment.get_water_availability()
        == fixed_point_t::_0_50
    );
    CHECK(agriculture.yield_factor == fixed_point_t::_0_50);
    CHECK(agriculture.constrained_output == fixed_point_t { 4 });
}

TEST_CASE(
    "006B2.1 invalid runoff forcing is rejected transactionally",
    "[convergence][006b2][006b2.1][environment][soil-water][validation]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 100 },
        .storage = fixed_point_t { 50 }
    };

    ProvinceSoilWaterForcing const invalid {
        .precipitation = fixed_point_t { 5 },
        .surface_runoff = fixed_point_t { 6 }
    };

    auto const result =
        advance_province_soil_water_balance(starting, invalid);

    CHECK_FALSE(result.valid());
    CHECK(
        result.status
        == province_soil_water_balance_status_t::INVALID_FORCING
    );
    CHECK(result.ending == starting);
}

TEST_CASE(
    "006B2.1 invalid soil water state is rejected",
    "[convergence][006b2][006b2.1][environment][soil-water][validation]"
) {
    ProvinceSoilWaterState const invalid {
        .capacity = fixed_point_t::_0,
        .storage = fixed_point_t::_0
    };

    auto const result =
        advance_province_soil_water_balance(invalid, {});

    CHECK_FALSE(result.valid());
    CHECK(
        result.status
        == province_soil_water_balance_status_t::INVALID_STARTING_STATE
    );
}

TEST_CASE(
    "006B2.1 identical water balance inputs are deterministic",
    "[convergence][006b2][006b2.1][environment][soil-water][determinism]"
) {
    ProvinceSoilWaterState const starting {
        .capacity = fixed_point_t { 120 },
        .storage = fixed_point_t { 75 }
    };

    ProvinceSoilWaterForcing const forcing {
        .precipitation = fixed_point_t { 18 },
        .surface_runoff = fixed_point_t { 3 },
        .evapotranspiration_demand = fixed_point_t { 11 }
    };

    auto const first =
        advance_province_soil_water_balance(starting, forcing);

    auto const second =
        advance_province_soil_water_balance(starting, forcing);

    REQUIRE(first.valid());
    REQUIRE(second.valid());
    CHECK(first == second);
}