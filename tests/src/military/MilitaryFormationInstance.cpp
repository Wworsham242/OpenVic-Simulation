#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A10 nonlegacy formation exists in authoritative runtime state",
    "[convergence][005a10][military][runtime]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("air")
    );

    MilitaryFormationManager definitions;

    REQUIRE(
        definitions.add_military_capability(
            "sensing"
        )
    );

    auto const* air =
        domains.get_military_domain_by_identifier(
            "air"
        );

    auto const* sensing =
        definitions.
            get_military_capability_by_identifier(
                "sensing"
            );

    REQUIRE(air != nullptr);
    REQUIRE(sensing != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        1
    > capabilities {
        sensing
    };

    REQUIRE(
        definitions.add_military_formation(
            "airborne_sensor_formation",
            *air,
            capabilities
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "airborne_sensor_formation"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Formation Alpha",
            *definition
        )
    );

    CHECK(
        runtime.
            get_military_formation_instance_count()
        == 1
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->
            get_formation_definition().
            get_identifier()
        ==
        "airborne_sensor_formation"
    );

    CHECK(
        instance->
            get_domain().
            get_identifier()
        ==
        "air"
    );

    CHECK(
        instance->has_capability(
            *sensing
        )
    );

    CHECK(
        instance->get_readiness() == 1
    );
}

TEST_CASE(
    "005A10 runtime identity is stable and unique",
    "[convergence][005a10][military][identity]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "nonlegacy"
        )
    );

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "nonlegacy"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    REQUIRE(
        definitions.add_military_formation(
            "formation",
            *domain,
            no_capabilities
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "formation"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "One",
            *definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Two",
            *definition
        )
    );

    auto const* first =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    auto const* second =
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            );

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    CHECK(first->unique_id != second->unique_id);

    CHECK(
        first->
            get_formation_definition().
            get_identifier()
        ==
        second->
            get_formation_definition().
            get_identifier()
    );
}

TEST_CASE(
    "005A10 readiness is bounded authoritative state",
    "[convergence][005a10][military][readiness]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("space")
    );

    MilitaryFormationManager definitions;

    auto const* space =
        domains.get_military_domain_by_identifier(
            "space"
        );

    REQUIRE(space != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    REQUIRE(
        definitions.add_military_formation(
            "space_asset_group",
            *space,
            no_capabilities
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "space_asset_group"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    fixed_point_t const readiness_half =
        fixed_point_t::_0_50;

    fixed_point_t const readiness_three_quarters =
        fixed_point_t::_0_50 +
        fixed_point_t::_0_25;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Orbital Group",
            *definition,
            readiness_half
        )
    );

    auto* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        readiness_half
    );

    REQUIRE(
        instance->set_readiness(
            readiness_three_quarters
        )
    );

    CHECK(
        instance->get_readiness()
        ==
        readiness_three_quarters
    );

    CHECK_FALSE(
        instance->set_readiness(
            fixed_point_t { 2 }
        )
    );

    CHECK(
        instance->get_readiness()
        ==
        readiness_three_quarters
    );
}

TEST_CASE(
    "005A10 invalid runtime creation does not mutate authoritative state",
    "[convergence][005a10][military][validation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "test_domain"
        )
    );

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "test_domain"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    REQUIRE(
        definitions.add_military_formation(
            "test_definition",
            *domain,
            no_capabilities
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "test_definition"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    CHECK_FALSE(
        runtime.create_military_formation_instance(
            "",
            *definition
        )
    );

    CHECK_FALSE(
        runtime.create_military_formation_instance(
            "Invalid readiness",
            *definition,
            fixed_point_t { 2 }
        )
    );

    CHECK(
        runtime.
            get_military_formation_instance_count()
        ==
        0
    );
}
