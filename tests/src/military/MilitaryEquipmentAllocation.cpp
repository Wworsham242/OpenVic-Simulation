#include "snitch/snitch.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <unordered_map>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAllocation.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

namespace {

struct AllocationFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryDomainDefinition const* domain =
        nullptr;

    AllocationFixture() {
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
        std::string_view identifier,
        std::span<
            MilitaryEquipmentRequirementSpec const
        > equipment_requirements
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

        REQUIRE(
            definitions.add_military_formation(
                identifier,
                *domain,
                capabilities,
                provisions,
                hosting,
                support,
                equipment_requirements
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
                    definition,
                    fixed_point_t::_1
                )
        );
    }
};

}

TEST_CASE(
    "005A18 scarce stock cannot be double allocated",
    "[convergence][005a18][military][allocation]"
) {
    AllocationFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_definition(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "First",
        *definition
    );

    fixture.create(
        "Second",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 15 };

    auto draw =
        [&stock](
            std::string_view item,
            fixed_point_t requested
        ) {
            REQUIRE(
                item == "equipment"
            );

            fixed_point_t const assigned =
                std::min(
                    stock,
                    requested
                );

            stock -= assigned;

            return assigned;
        };

    std::array ids {
        unique_id_t { 1 },
        unique_id_t { 2 }
    };

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
            draw,
            result
        )
    );

    CHECK(
        result.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );

    CHECK(
        result.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );

    CHECK(
        result.get_total_assigned(
            "equipment"
        ) ==
        fixed_point_t { 15 }
    );

    CHECK(
        stock ==
        fixed_point_t::_0
    );

    CHECK(
        result.get_unmet_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );
}

TEST_CASE(
    "005A18 allocation order is deterministic by formation identity",
    "[convergence][005a18][military][determinism]"
) {
    AllocationFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_definition(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "First",
        *definition
    );

    fixture.create(
        "Second",
        *definition
    );

    /*
     * Reverse caller order deliberately.
     * The proof allocator sorts by stable runtime identity.
     */
    std::array ids {
        unique_id_t { 2 },
        unique_id_t { 1 }
    };

    fixed_point_t stock =
        fixed_point_t { 10 };

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
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

    CHECK(
        result.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );

    CHECK(
        result.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A18 independent equipment pools remain independent",
    "[convergence][005a18][military][allocation][multi-item]"
) {
    AllocationFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "primary",
            .required_quantity =
                fixed_point_t { 10 }
        },
        MilitaryEquipmentRequirementSpec {
            .item_id = "secondary",
            .required_quantity =
                fixed_point_t { 20 }
        }
    };

    auto const* definition =
        fixture.add_definition(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    std::unordered_map<
        std::string_view,
        fixed_point_t
    > stock {
        {
            "primary",
            fixed_point_t { 8 }
        },
        {
            "secondary",
            fixed_point_t { 20 }
        }
    };

    MilitaryEquipmentAllocationResult result;

    std::array ids {
        unique_id_t { 1 }
    };

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
            [&stock](
                std::string_view item,
                fixed_point_t requested
            ) {
                fixed_point_t const available =
                    stock[item];

                fixed_point_t const assigned =
                    std::min(
                        available,
                        requested
                    );

                stock[item] -=
                    assigned;

                return assigned;
            },
            result
        )
    );

    CHECK(
        result.get_assigned_quantity(
            1,
            "primary"
        ) ==
        fixed_point_t { 8 }
    );

    CHECK(
        result.get_unmet_quantity(
            1,
            "primary"
        ) ==
        fixed_point_t { 2 }
    );

    CHECK(
        result.get_assigned_quantity(
            1,
            "secondary"
        ) ==
        fixed_point_t { 20 }
    );
}

TEST_CASE(
    "005A18 allocation result feeds equipment condition",
    "[convergence][005a18][military][integration]"
) {
    AllocationFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_definition(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    fixed_point_t stock =
        fixed_point_t { 6 };

    MilitaryEquipmentAllocationResult result;

    std::array ids {
        unique_id_t { 1 }
    };

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
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

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [&result](
                    std::string_view item
                ) {
                    return
                        result.
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
        (fixed_point_t { 3 } / 5)
    );
}

TEST_CASE(
    "005A18 formations without equipment requirements draw nothing",
    "[convergence][005a18][military][optional]"
) {
    AllocationFixture fixture;

    std::array<
        MilitaryEquipmentRequirementSpec,
        0
    > none {};

    auto const* definition =
        fixture.add_definition(
            "austere",
            none
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Austere",
        *definition
    );

    bool draw_called = false;

    MilitaryEquipmentAllocationResult result;

    std::array ids {
        unique_id_t { 1 }
    };

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
            [&draw_called](
                std::string_view,
                fixed_point_t
            ) {
                draw_called = true;

                return fixed_point_t::_0;
            },
            result
        )
    );

    CHECK_FALSE(draw_called);

    CHECK(
        result.get_allocations().empty()
    );
}

TEST_CASE(
    "005A18 duplicate formation request fails before stock mutation",
    "[convergence][005a18][military][validation]"
) {
    AllocationFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_definition(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    std::array ids {
        unique_id_t { 1 },
        unique_id_t { 1 }
    };

    bool draw_called = false;

    MilitaryEquipmentAllocationResult result;

    CHECK_FALSE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
            [&draw_called](
                std::string_view,
                fixed_point_t requested
            ) {
                draw_called = true;
                return requested;
            },
            result
        )
    );

    CHECK_FALSE(draw_called);

    CHECK(
        result.get_allocations().empty()
    );
}

TEST_CASE(
    "005A18 unknown formation fails before stock mutation",
    "[convergence][005a18][military][validation]"
) {
    AllocationFixture fixture;

    std::array ids {
        unique_id_t { 99 }
    };

    bool draw_called = false;

    MilitaryEquipmentAllocationResult result;

    CHECK_FALSE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
            [&draw_called](
                std::string_view,
                fixed_point_t requested
            ) {
                draw_called = true;
                return requested;
            },
            result
        )
    );

    CHECK_FALSE(draw_called);
}

TEST_CASE(
    "005A18 invalid authoritative draw is rejected",
    "[convergence][005a18][military][validation]"
) {
    AllocationFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_definition(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    std::array ids {
        unique_id_t { 1 }
    };

    MilitaryEquipmentAllocationResult result;

    CHECK_FALSE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            ids,
            [](
                std::string_view,
                fixed_point_t
            ) {
                return fixed_point_t { 11 };
            },
            result
        )
    );

    CHECK(
        result.get_allocations().empty()
    );
}
