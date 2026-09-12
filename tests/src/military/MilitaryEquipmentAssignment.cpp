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

struct AssignmentFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryDomainDefinition const* domain =
        nullptr;

    AssignmentFixture() {
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
    allocate(
        std::span<
            MilitaryEquipmentAllocationRequest const
        > requests,
        fixed_point_t& stock
    ) {
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
    "005A20 committed assignment survives transient allocation result",
    "[convergence][005a20][military][assignment][persistence]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 6 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
        )
    );

    result.clear();

    CHECK(
        state.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 6 }
    );

    CHECK(
        state.get_outstanding_requirement(
            fixture.runtime,
            1,
            "equipment"
        ) ==
        fixed_point_t { 4 }
    );
}

TEST_CASE(
    "005A20 repeated commit cannot duplicate assignment",
    "[convergence][005a20][military][assignment][validation]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 10 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
        )
    );

    CHECK_FALSE(
        state.apply_allocation_result(
            fixture.runtime,
            result
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
    "005A20 release returns equipment before assignment decreases",
    "[convergence][005a20][military][assignment][release]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 8 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
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
        returned ==
        fixed_point_t { 3 }
    );

    CHECK(
        state.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );

    CHECK(
        state.get_outstanding_requirement(
            fixture.runtime,
            1,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );
}

TEST_CASE(
    "005A20 rejected return preserves assignment",
    "[convergence][005a20][military][assignment][release]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 8 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
        )
    );

    CHECK_FALSE(
        state.release(
            1,
            "equipment",
            fixed_point_t { 3 },
            [](
                std::string_view,
                fixed_point_t
            ) {
                return false;
            }
        )
    );

    CHECK(
        state.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 8 }
    );
}

TEST_CASE(
    "005A20 transfer conserves persistent assigned quantity",
    "[convergence][005a20][military][assignment][transfer]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Source",
        *definition
    );

    fixture.create(
        "Target",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 10 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 100
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
        )
    );

    REQUIRE(
        state.transfer(
            fixture.runtime,
            1,
            2,
            "equipment",
            fixed_point_t { 4 }
        )
    );

    CHECK(
        state.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 6 }
    );

    CHECK(
        state.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 4 }
    );

    CHECK(
        state.get_total_assigned(
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A20 transfer cannot exceed source or target requirement",
    "[convergence][005a20][military][assignment][validation]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Source",
        *definition
    );

    fixture.create(
        "Target",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 10 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
        )
    );

    CHECK_FALSE(
        state.transfer(
            fixture.runtime,
            1,
            2,
            "equipment",
            fixed_point_t { 11 }
        )
    );

    CHECK(
        state.get_total_assigned(
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A20 persistent assignment directly feeds equipment condition",
    "[convergence][005a20][military][assignment][integration]"
) {
    AssignmentFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 7 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    auto result =
        fixture.allocate(
            requests,
            stock
        );

    MilitaryEquipmentAssignmentState state;

    REQUIRE(
        state.apply_allocation_result(
            fixture.runtime,
            result
        )
    );

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [&state](
                    std::string_view item
                ) {
                    return
                        state.
                            get_assigned_quantity(
                                1,
                                item
                            );
                }
            )
    );

    auto const* formation =
        fixture.runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(formation != nullptr);

    CHECK(
        formation->
            get_equipment_condition()
        ==
        (fixed_point_t { 7 } / 10)
    );
}
