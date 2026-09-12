#include "snitch/snitch.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAllocation.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAssignment.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

namespace {

struct ReplenishmentFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryDomainDefinition const* domain =
        nullptr;

    ReplenishmentFixture() {
        REQUIRE(
            domains.add_military_domain(
                "generic"
            )
        );

        domain =
            domains.
                get_military_domain_by_identifier(
                    "generic"
                );

        REQUIRE(domain != nullptr);
    }

    MilitaryFormationDefinition const*
    add_definition(
        std::string_view identifier
    ) {
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

        std::array equipment {
            MilitaryEquipmentRequirementSpec {
                .item_id = "equipment",
                .required_quantity =
                    fixed_point_t { 10 }
            }
        };

        REQUIRE(
            definitions.add_military_formation(
                identifier,
                *domain,
                capabilities,
                provisions,
                hosting,
                support,
                equipment
            )
        );

        return
            definitions.
                get_military_formation_by_identifier(
                    identifier
                );
    }

    void create(
        std::string_view name,
        MilitaryFormationDefinition const&
            definition
    ) {
        REQUIRE(
            runtime.
                create_military_formation_instance(
                    name,
                    definition
                )
        );
    }

    MilitaryEquipmentAllocationResult
    initial_allocate(
        unique_id_t formation_unique_id,
        fixed_point_t& stock
    ) {
        std::array requests {
            MilitaryEquipmentAllocationRequest {
                .formation_unique_id =
                    formation_unique_id,
                .priority = 0
            }
        };

        MilitaryEquipmentAllocationResult result;

        REQUIRE(
            MilitaryEquipmentAllocator::allocate(
                runtime,
                requests,
                [&stock](
                    std::string_view,
                    fixed_point_t requested
                ) {
                    fixed_point_t const assigned =
                        std::min(
                            stock,
                            requested
                        );

                    stock -= assigned;
                    return assigned;
                },
                result
            )
        );

        return result;
    }
};

}

TEST_CASE(
    "005A21 replenishment requests only persistent shortfall",
    "[convergence][005a21][military][replenishment]"
) {
    ReplenishmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t initial_stock =
        fixed_point_t { 6 };

    auto initial =
        fixture.initial_allocate(
            1,
            initial_stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            initial
        )
    );

    fixed_point_t replenishment_stock =
        fixed_point_t { 10 };

    fixed_point_t observed_request =
        fixed_point_t::_0;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    MilitaryEquipmentAllocationResult refill;

    REQUIRE(
        state.allocate_replenishment(
            fixture.runtime,
            requests,
            [
                &replenishment_stock,
                &observed_request
            ](
                std::string_view,
                fixed_point_t requested
            ) {
                observed_request = requested;

                fixed_point_t const assigned =
                    std::min(
                        replenishment_stock,
                        requested
                    );

                replenishment_stock -= assigned;
                return assigned;
            },
            refill
        )
    );

    CHECK(
        observed_request ==
        fixed_point_t { 4 }
    );

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            refill
        )
    );

    CHECK(
        state.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A21 fully equipped formation performs no draw",
    "[convergence][005a21][military][replenishment]"
) {
    ReplenishmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t initial_stock =
        fixed_point_t { 10 };

    auto initial =
        fixture.initial_allocate(
            1,
            initial_stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            initial
        )
    );

    bool draw_called = false;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    MilitaryEquipmentAllocationResult refill;

    REQUIRE(
        state.allocate_replenishment(
            fixture.runtime,
            requests,
            [&draw_called](
                std::string_view,
                fixed_point_t requested
            ) {
                draw_called = true;
                return requested;
            },
            refill
        )
    );

    CHECK_FALSE(draw_called);

    CHECK(
        refill.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A21 priority still governs scarce replenishment",
    "[convergence][005a21][military][replenishment][priority]"
) {
    ReplenishmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Partially equipped",
        *definition
    );

    fixture.create(
        "Empty high priority",
        *definition
    );

    fixed_point_t initial_stock =
        fixed_point_t { 8 };

    auto initial =
        fixture.initial_allocate(
            1,
            initial_stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            initial
        )
    );

    fixed_point_t stock =
        fixed_point_t { 5 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        },
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 2,
            .priority = 100
        }
    };

    MilitaryEquipmentAllocationResult refill;

    REQUIRE(
        state.allocate_replenishment(
            fixture.runtime,
            requests,
            [&stock](
                std::string_view,
                fixed_point_t requested
            ) {
                fixed_point_t const assigned =
                    std::min(
                        stock,
                        requested
                    );

                stock -= assigned;
                return assigned;
            },
            refill
        )
    );

    CHECK(
        refill.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );

    CHECK(
        refill.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A21 released equipment creates replenishment demand",
    "[convergence][005a21][military][replenishment][release]"
) {
    ReplenishmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t initial_stock =
        fixed_point_t { 10 };

    auto initial =
        fixture.initial_allocate(
            1,
            initial_stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            initial
        )
    );

    fixed_point_t returned =
        fixed_point_t::_0;

    REQUIRE(
        state.release(
            1,
            "equipment",
            fixed_point_t { 3 },
            [&returned](
                std::string_view,
                fixed_point_t quantity
            ) {
                returned += quantity;
                return true;
            }
        )
    );

    CHECK(
        state.get_outstanding_requirement(
            fixture.runtime,
            1,
            "equipment"
        ) ==
        fixed_point_t { 3 }
    );

    fixed_point_t observed_request =
        fixed_point_t::_0;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    MilitaryEquipmentAllocationResult refill;

    REQUIRE(
        state.allocate_replenishment(
            fixture.runtime,
            requests,
            [&observed_request](
                std::string_view,
                fixed_point_t requested
            ) {
                observed_request = requested;
                return requested;
            },
            refill
        )
    );

    CHECK(
        observed_request ==
        fixed_point_t { 3 }
    );
}

TEST_CASE(
    "005A21 request quantity seam can represent execution constraint",
    "[convergence][005a21][military][execution-boundary]"
) {
    ReplenishmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    fixed_point_t stock =
        fixed_point_t { 100 };

    fixed_point_t observed_draw_request =
        fixed_point_t::_0;

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::
            allocate_with_quantity_provider(
                fixture.runtime,
                requests,
                [](
                    unique_id_t,
                    std::string_view,
                    fixed_point_t declared
                ) {
                    /*
                     * Proof of boundary only:
                     *
                     * some external command/logistics mechanism says
                     * only 40% of declared demand is executable.
                     *
                     * 005A21 does not define why.
                     */
                    return
                        (
                            declared * 4
                        ) / 10;
                },
                [
                    &stock,
                    &observed_draw_request
                ](
                    std::string_view,
                    fixed_point_t requested
                ) {
                    observed_draw_request =
                        requested;

                    fixed_point_t const assigned =
                        std::min(
                            stock,
                            requested
                        );

                    stock -= assigned;
                    return assigned;
                },
                result
            )
    );

    CHECK(
        observed_draw_request ==
        fixed_point_t { 4 }
    );

    CHECK(
        result.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 4 }
    );
}
