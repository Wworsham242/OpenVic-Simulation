#include "snitch/snitch.hpp"

#include "openvic-simulation/economy/trading/TransportDemand.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A26 derives fixed and quantity-scaled aggregate demand",
    "[convergence][005a26][logistics][transport-demand]"
) {
    TransportDemandProfile profile {
        .profile_id = "aggregate_road",
        .factors = {
            TransportDemandFactor {
                .resource_id = "movement_control",
                .fixed_capacity = fixed_point_t { 1 }
            },
            TransportDemandFactor {
                .resource_id = "road_lift",
                .capacity_per_quantity = fixed_point_t { 1 } / 8
            }
        },
        .minimum_occupation_time = Timespan { 2 }
    };

    TransportDemandResult result;

    REQUIRE(
        TransportDemandDeriver::derive(
            profile,
            fixed_point_t { 80 },
            Timespan { 5 },
            result
        )
    );

    REQUIRE(result.requirements.size() == 2);
    CHECK(
        result.requirements[0].resource_id ==
        "movement_control"
    );
    CHECK(
        result.requirements[0].required_capacity ==
        fixed_point_t { 1 }
    );
    CHECK(
        result.requirements[1].resource_id ==
        "road_lift"
    );
    CHECK(
        result.requirements[1].required_capacity ==
        fixed_point_t { 10 }
    );
    CHECK(result.occupation_time == Timespan { 5 });
}

TEST_CASE(
    "005A26 fixed resource cost does not scale with cargo quantity",
    "[convergence][005a26][logistics][transport-demand][aggregate]"
) {
    TransportDemandProfile profile {
        .profile_id = "sealift_batch",
        .factors = {
            TransportDemandFactor {
                .resource_id = "movement_control",
                .fixed_capacity = fixed_point_t { 1 }
            },
            TransportDemandFactor {
                .resource_id = "sealift",
                .capacity_per_quantity = fixed_point_t { 1 } / 4
            }
        },
        .minimum_occupation_time = Timespan { 3 }
    };

    TransportDemandResult small;
    TransportDemandResult large;

    REQUIRE(
        TransportDemandDeriver::derive(
            profile,
            fixed_point_t { 4 },
            Timespan { 1 },
            small
        )
    );

    REQUIRE(
        TransportDemandDeriver::derive(
            profile,
            fixed_point_t { 20 },
            Timespan { 1 },
            large
        )
    );

    CHECK(
        small.requirements[0].required_capacity ==
        fixed_point_t { 1 }
    );
    CHECK(
        large.requirements[0].required_capacity ==
        fixed_point_t { 1 }
    );

    CHECK(
        small.requirements[1].required_capacity ==
        fixed_point_t { 1 }
    );
    CHECK(
        large.requirements[1].required_capacity ==
        fixed_point_t { 5 }
    );

    CHECK(small.occupation_time == Timespan { 3 });
    CHECK(large.occupation_time == Timespan { 3 });
}

TEST_CASE(
    "005A26 profiles remain data-defined rather than mode-enumerated",
    "[convergence][005a26][logistics][transport-demand][generic]"
) {
    TransportDemandProfile profile {
        .profile_id = "caravan_or_future_system",
        .factors = {
            TransportDemandFactor {
                .resource_id = "arbitrary_resource",
                .fixed_capacity = fixed_point_t { 2 }
            }
        },
        .minimum_occupation_time = Timespan { 1 }
    };

    TransportDemandResult result;

    REQUIRE(
        TransportDemandDeriver::derive(
            profile,
            fixed_point_t { 500 },
            Timespan { 7 },
            result
        )
    );

    REQUIRE(result.requirements.size() == 1);
    CHECK(
        result.requirements[0].resource_id ==
        "arbitrary_resource"
    );
    CHECK(
        result.requirements[0].required_capacity ==
        fixed_point_t { 2 }
    );
}
