#include "snitch/snitch.hpp"

#include <array>
#include <string_view>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

namespace {

struct EquipmentRequirementFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryDomainDefinition const* domain = nullptr;

    EquipmentRequirementFixture() {
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
    add_formation(
        std::string_view identifier,
        std::span<
            MilitaryEquipmentRequirementSpec const
        > requirements
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
        > hosting_requirements {};

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
                hosting_requirements,
                support,
                requirements
            )
        );

        return
            definitions.
                get_military_formation_by_identifier(
                    identifier
                );
    }
};

}

TEST_CASE(
    "005A17 equipment requirements remain optional",
    "[convergence][005a17][military][equipment]"
) {
    EquipmentRequirementFixture fixture;

    std::array<
        MilitaryEquipmentRequirementSpec,
        0
    > none {};

    auto const* definition =
        fixture.add_formation(
            "austere",
            none
        );

    REQUIRE(definition != nullptr);

    REQUIRE(
        fixture.runtime.
            create_military_formation_instance(
                "Austere",
                *definition,
                fixed_point_t::_1
            )
    );

    bool provider_called = false;

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [&provider_called](
                    std::string_view
                ) {
                    provider_called = true;
                    return fixed_point_t::_0;
                }
            )
    );

    auto const* formation =
        fixture.runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(formation != nullptr);

    CHECK_FALSE(provider_called);

    CHECK(
        formation->get_equipment_condition()
        ==
        fixed_point_t::_1
    );
}

TEST_CASE(
    "005A17 equipment quantity derives equipment condition",
    "[convergence][005a17][military][equipment][quantity]"
) {
    EquipmentRequirementFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_formation(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    REQUIRE(
        fixture.runtime.
            create_military_formation_instance(
                "Formation",
                *definition,
                fixed_point_t::_1
            )
    );

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [](
                    std::string_view item
                ) {
                    REQUIRE(
                        item == "equipment"
                    );

                    return fixed_point_t { 5 };
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
        formation->get_equipment_condition()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A17 multiple equipment requirements form a bottleneck",
    "[convergence][005a17][military][equipment][bottleneck]"
) {
    EquipmentRequirementFixture fixture;

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
        fixture.add_formation(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    REQUIRE(
        fixture.runtime.
            create_military_formation_instance(
                "Formation",
                *definition
            )
    );

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [](
                    std::string_view item
                ) {
                    if (item == "primary") {
                        return fixed_point_t { 8 };
                    }

                    if (item == "secondary") {
                        return fixed_point_t { 10 };
                    }

                    return fixed_point_t::_0;
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
        formation->get_equipment_condition()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A17 excess equipment does not exceed healthy condition",
    "[convergence][005a17][military][equipment][clamp]"
) {
    EquipmentRequirementFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_formation(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    REQUIRE(
        fixture.runtime.
            create_military_formation_instance(
                "Formation",
                *definition
            )
    );

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [](
                    std::string_view
                ) {
                    return fixed_point_t { 20 };
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
        formation->get_equipment_condition()
        ==
        fixed_point_t::_1
    );
}

TEST_CASE(
    "005A17 derived equipment condition constrains readiness",
    "[convergence][005a17][military][readiness]"
) {
    EquipmentRequirementFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_formation(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    REQUIRE(
        fixture.runtime.
            create_military_formation_instance(
                "Formation",
                *definition,
                fixed_point_t::_1
            )
    );

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [](
                    std::string_view
                ) {
                    return fixed_point_t { 5 };
                }
            )
    );

    REQUIRE(
        fixture.runtime.
            adjust_readiness_toward(
                1,
                fixed_point_t::_1,
                fixed_point_t::_1
            )
    );

    auto const* formation =
        fixture.runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(formation != nullptr);

    CHECK(
        formation->get_equipment_condition()
        ==
        fixed_point_t::_0_50
    );

    CHECK(
        formation->get_readiness()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A17 invalid provider result preserves equipment condition",
    "[convergence][005a17][military][validation]"
) {
    EquipmentRequirementFixture fixture;

    std::array requirements {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        }
    };

    auto const* definition =
        fixture.add_formation(
            "formation",
            requirements
        );

    REQUIRE(definition != nullptr);

    REQUIRE(
        fixture.runtime.
            create_military_formation_instance(
                "Formation",
                *definition
            )
    );

    REQUIRE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [](
                    std::string_view
                ) {
                    return fixed_point_t { 5 };
                }
            )
    );

    auto const* before =
        fixture.runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(before != nullptr);

    REQUIRE(
        before->get_equipment_condition()
        ==
        fixed_point_t::_0_50
    );

    CHECK_FALSE(
        fixture.runtime.
            evaluate_equipment_condition(
                1,
                [](
                    std::string_view
                ) {
                    return fixed_point_t { -1 };
                }
            )
    );

    auto const* after =
        fixture.runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(after != nullptr);

    CHECK(
        after->get_equipment_condition()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A17 invalid equipment requirement definitions fail",
    "[convergence][005a17][military][validation]"
) {
    EquipmentRequirementFixture fixture;

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

    std::array invalid_quantity {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t::_0
        }
    };

    CHECK_FALSE(
        fixture.definitions.
            add_military_formation(
                "invalid_quantity",
                *fixture.domain,
                capabilities,
                provisions,
                hosting,
                support,
                invalid_quantity
            )
    );

    std::array duplicate {
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 10 }
        },
        MilitaryEquipmentRequirementSpec {
            .item_id = "equipment",
            .required_quantity =
                fixed_point_t { 20 }
        }
    };

    CHECK_FALSE(
        fixture.definitions.
            add_military_formation(
                "duplicate",
                *fixture.domain,
                capabilities,
                provisions,
                hosting,
                support,
                duplicate
            )
    );
}
