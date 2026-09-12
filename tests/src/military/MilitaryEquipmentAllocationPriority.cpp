#include "snitch/snitch.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAllocation.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

namespace {

struct PriorityFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryDomainDefinition const* domain =
        nullptr;

    PriorityFixture() {
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
};

}

TEST_CASE(
    "005A19 explicit priority overrides formation identity",
    "[convergence][005a19][military][priority]"
) {
    PriorityFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
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
        fixed_point_t { 10 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 10
        },
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 2,
            .priority = 20
        }
    };

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
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
            result
        )
    );

    CHECK(
        result.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );

    CHECK(
        result.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A19 equal priorities use stable formation identity",
    "[convergence][005a19][military][priority][determinism]"
) {
    PriorityFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
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
        fixed_point_t { 10 };

    /*
     * Caller order is deliberately reversed.
     */
    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 2,
            .priority = 50
        },
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 50
        }
    };

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
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
    "005A19 compatibility overload preserves 005A18 ordering",
    "[convergence][005a19][military][compatibility]"
) {
    PriorityFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
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
        fixed_point_t { 10 };

    std::array ids {
        unique_id_t { 2 },
        unique_id_t { 1 }
    };

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
    "005A19 negative priority remains valid and ordered",
    "[convergence][005a19][military][priority]"
) {
    PriorityFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
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
        fixed_point_t { 10 };

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = -10
        },
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 2,
            .priority = 0
        }
    };

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
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
            result
        )
    );

    CHECK(
        result.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );

    CHECK(
        result.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A19 duplicate prioritized formation fails before draw",
    "[convergence][005a19][military][validation]"
) {
    PriorityFixture fixture;

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
            .priority = 100
        },
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = -100
        }
    };

    bool draw_called = false;

    MilitaryEquipmentAllocationResult result;

    CHECK_FALSE(
        MilitaryEquipmentAllocator::allocate(
            fixture.runtime,
            requests,
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
    "005A19 priority changes scarcity distribution not total stock",
    "[convergence][005a19][military][priority][scarcity]"
) {
    PriorityFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
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

    MilitaryEquipmentAllocationResult result;

    REQUIRE(
        MilitaryEquipmentAllocator::allocate(
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
            result
        )
    );

    CHECK(
        result.get_assigned_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );

    CHECK(
        result.get_assigned_quantity(
            1,
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
}
