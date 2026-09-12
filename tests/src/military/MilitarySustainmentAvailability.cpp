#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitarySustainmentAvailability.hpp"

using namespace OpenVic;

namespace {

struct AvailabilityFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;
    MilitarySustainmentStockState stock;

    AvailabilityFixture() {
        REQUIRE(
            domains.add_military_domain("generic")
        );

        auto const* domain =
            domains.get_military_domain_by_identifier(
                "generic"
            );

        REQUIRE(domain != nullptr);

        std::array<
            MilitaryCapabilityDefinition const*,
            0
        > capabilities {};

        std::array<
            MilitaryHostingProvisionSpec,
            0
        > provisions {};

        std::array<
            MilitaryHostingRequirementSpec,
            0
        > hosting {};

        std::array<
            MilitarySupportTypeDefinition const*,
            0
        > support {};

        std::array<MilitaryEquipmentRequirementSpec, 0>
            equipment {};

        REQUIRE(
            definitions.add_military_formation(
                "formation",
                *domain,
                capabilities,
                provisions,
                hosting,
                support,
                equipment
            )
        );

        auto const* definition =
            definitions.
                get_military_formation_by_identifier(
                    "formation"
                );

        REQUIRE(definition != nullptr);

        REQUIRE(
            runtime.create_military_formation_instance(
                "Formation",
                *definition
            )
        );
    }

    void add_stock(
        std::string_view item_id,
        fixed_point_t quantity
    ) {
        REQUIRE(
            stock.add_stock(
                runtime,
                1,
                item_id,
                fixed_point_t { 32 },
                fixed_point_t { 32 },
                quantity
            )
        );
    }

    MilitarySustainmentConsumptionResult consume(
        MilitarySustainmentConsumptionProfile const& profile,
        Timespan elapsed
    ) {
        MilitarySustainmentConsumptionResult result;

        REQUIRE(
            MilitarySustainmentConsumer::
                consume_for_activity(
                    1,
                    profile,
                    elapsed,
                    stock,
                    result
                )
        );

        return result;
    }
};

}

TEST_CASE(
    "005A30 fully supplied activity yields full sustainment availability",
    "[convergence][005a30][military][sustainment][availability]"
) {
    AvailabilityFixture fixture;

    fixture.add_stock(
        "resource",
        fixed_point_t { 16 }
    );

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "activity",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource",
                .quantity_per_day = fixed_point_t { 4 }
            }
        }
    };

    auto consumption =
        fixture.consume(profile, Timespan { 2 });

    MilitarySustainmentAvailabilityResult availability;

    REQUIRE(
        MilitarySustainmentAvailabilityDeriver::derive(
            consumption,
            availability
        )
    );

    CHECK(
        availability.get_fulfillment_fraction("resource") ==
        fixed_point_t::_1
    );

    CHECK(
        availability.get_limiting_fraction() ==
        fixed_point_t::_1
    );
}

TEST_CASE(
    "005A30 partial sustainment demand produces proportional fulfillment",
    "[convergence][005a30][military][sustainment][shortage]"
) {
    AvailabilityFixture fixture;

    fixture.add_stock(
        "resource",
        fixed_point_t { 6 }
    );

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "activity",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource",
                .quantity_per_day = fixed_point_t { 4 }
            }
        }
    };

    auto consumption =
        fixture.consume(profile, Timespan { 2 });

    MilitarySustainmentAvailabilityResult availability;

    REQUIRE(
        MilitarySustainmentAvailabilityDeriver::derive(
            consumption,
            availability
        )
    );

    fixed_point_t const three_quarters =
        fixed_point_t { 3 } / 4;

    CHECK(
        availability.get_fulfillment_fraction("resource") ==
        three_quarters
    );

    CHECK(
        availability.get_limiting_fraction() ==
        three_quarters
    );
}

TEST_CASE(
    "005A30 limiting signal exposes weakest positive-demand resource",
    "[convergence][005a30][military][sustainment][limiting]"
) {
    AvailabilityFixture fixture;

    fixture.add_stock(
        "resource_a",
        fixed_point_t { 8 }
    );

    fixture.add_stock(
        "resource_b",
        fixed_point_t { 2 }
    );

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "activity",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource_a",
                .quantity_per_day = fixed_point_t { 4 }
            },
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource_b",
                .quantity_per_day = fixed_point_t { 2 }
            }
        }
    };

    auto consumption =
        fixture.consume(profile, Timespan { 2 });

    MilitarySustainmentAvailabilityResult availability;

    REQUIRE(
        MilitarySustainmentAvailabilityDeriver::derive(
            consumption,
            availability
        )
    );

    CHECK(
        availability.get_fulfillment_fraction("resource_a") ==
        fixed_point_t::_1
    );

    CHECK(
        availability.get_fulfillment_fraction("resource_b") ==
        fixed_point_t::_0_50
    );

    CHECK(
        availability.get_limiting_fraction() ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A30 zero-demand resource does not reduce availability",
    "[convergence][005a30][military][sustainment][zero-demand]"
) {
    AvailabilityFixture fixture;

    fixture.add_stock(
        "active_resource",
        fixed_point_t { 4 }
    );

    fixture.add_stock(
        "unused_resource",
        fixed_point_t::_0
    );

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "activity",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "active_resource",
                .quantity_per_day = fixed_point_t { 2 }
            },
            MilitarySustainmentConsumptionFactor {
                .item_id = "unused_resource",
                .quantity_per_day = fixed_point_t::_0
            }
        }
    };

    auto consumption =
        fixture.consume(profile, Timespan { 2 });

    MilitarySustainmentAvailabilityResult availability;

    REQUIRE(
        MilitarySustainmentAvailabilityDeriver::derive(
            consumption,
            availability
        )
    );

    CHECK(
        availability.get_fulfillment_fraction(
            "unused_resource"
        ) == fixed_point_t::_1
    );

    CHECK(
        availability.get_limiting_fraction() ==
        fixed_point_t::_1
    );
}
