#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitarySustainmentConsumption.hpp"

using namespace OpenVic;

namespace {

struct ConsumptionFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;
    MilitarySustainmentStockState stock;

    ConsumptionFixture() {
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
                fixed_point_t { 20 },
                fixed_point_t { 20 },
                quantity
            )
        );
    }
};

}

TEST_CASE(
    "005A29 activity rate and elapsed time derive aggregate consumption",
    "[convergence][005a29][military][sustainment][consumption]"
) {
    ConsumptionFixture fixture;
    fixture.add_stock("resource", fixed_point_t { 20 });

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "activity_a",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource",
                .quantity_per_day = fixed_point_t { 2 }
            }
        }
    };

    MilitarySustainmentConsumptionResult result;

    REQUIRE(
        MilitarySustainmentConsumer::consume_for_activity(
            1,
            profile,
            Timespan { 3 },
            fixture.stock,
            result
        )
    );

    CHECK(
        result.get_requested_quantity("resource") ==
        fixed_point_t { 6 }
    );

    CHECK(
        result.get_consumed_quantity("resource") ==
        fixed_point_t { 6 }
    );

    CHECK(
        result.get_unmet_quantity("resource") ==
        fixed_point_t::_0
    );

    CHECK(
        fixture.stock.get_quantity(1, "resource") ==
        fixed_point_t { 14 }
    );
}

TEST_CASE(
    "005A29 shortage is explicit when activity demand exceeds stock",
    "[convergence][005a29][military][sustainment][shortage]"
) {
    ConsumptionFixture fixture;
    fixture.add_stock("resource", fixed_point_t { 5 });

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "high_activity",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource",
                .quantity_per_day = fixed_point_t { 3 }
            }
        }
    };

    MilitarySustainmentConsumptionResult result;

    REQUIRE(
        MilitarySustainmentConsumer::consume_for_activity(
            1,
            profile,
            Timespan { 2 },
            fixture.stock,
            result
        )
    );

    CHECK(
        result.get_requested_quantity("resource") ==
        fixed_point_t { 6 }
    );

    CHECK(
        result.get_consumed_quantity("resource") ==
        fixed_point_t { 5 }
    );

    CHECK(
        result.get_unmet_quantity("resource") ==
        fixed_point_t { 1 }
    );

    CHECK(
        fixture.stock.get_quantity(1, "resource") ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A29 one activity can consume multiple data-defined resources",
    "[convergence][005a29][military][sustainment][profile]"
) {
    ConsumptionFixture fixture;
    fixture.add_stock("resource_a", fixed_point_t { 20 });
    fixture.add_stock("resource_b", fixed_point_t { 20 });

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "scenario_activity",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource_a",
                .quantity_per_day = fixed_point_t { 1 }
            },
            MilitarySustainmentConsumptionFactor {
                .item_id = "resource_b",
                .quantity_per_day = fixed_point_t { 2 }
            }
        }
    };

    MilitarySustainmentConsumptionResult result;

    REQUIRE(
        MilitarySustainmentConsumer::consume_for_activity(
            1,
            profile,
            Timespan { 4 },
            fixture.stock,
            result
        )
    );

    CHECK(
        fixture.stock.get_quantity(1, "resource_a") ==
        fixed_point_t { 16 }
    );

    CHECK(
        fixture.stock.get_quantity(1, "resource_b") ==
        fixed_point_t { 12 }
    );
}

TEST_CASE(
    "005A29 activity identity remains data defined",
    "[convergence][005a29][military][sustainment][generic]"
) {
    ConsumptionFixture fixture;
    fixture.add_stock(
        "scenario_resource",
        fixed_point_t { 10 }
    );

    MilitarySustainmentConsumptionProfile profile {
        .activity_id = "arbitrary_activity_from_any_era",
        .factors = {
            MilitarySustainmentConsumptionFactor {
                .item_id = "scenario_resource",
                .quantity_per_day = fixed_point_t { 1 }
            }
        }
    };

    MilitarySustainmentConsumptionResult result;

    REQUIRE(
        MilitarySustainmentConsumer::consume_for_activity(
            1,
            profile,
            Timespan { 1 },
            fixture.stock,
            result
        )
    );

    CHECK(
        result.get_consumed_quantity(
            "scenario_resource"
        ) == fixed_point_t { 1 }
    );
}
