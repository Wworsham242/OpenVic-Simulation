#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A11 direct operational placement is generic",
    "[convergence][005a11][military][placement]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("land")
    );

    MilitaryFormationManager definitions;

    auto const* land =
        domains.get_military_domain_by_identifier(
            "land"
        );

    REQUIRE(land != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    REQUIRE(
        definitions.add_military_formation(
            "land_formation",
            *land,
            no_capabilities
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "land_formation"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Land Formation",
            *definition
        )
    );

    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "province_001"
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(instance->has_direct_position());
    CHECK_FALSE(instance->is_hosted());

    CHECK(
        runtime.
            get_effective_operational_position_id(
                1
            )
        ==
        "province_001"
    );
}

TEST_CASE(
    "005A11 carrier hosted air formation derives host position",
    "[convergence][005a11][military][hosting]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("naval")
    );

    REQUIRE(
        domains.add_military_domain("air")
    );

    MilitaryFormationManager definitions;

    auto const* naval =
        domains.get_military_domain_by_identifier(
            "naval"
        );

    auto const* air =
        domains.get_military_domain_by_identifier(
            "air"
        );

    REQUIRE(naval != nullptr);
    REQUIRE(air != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "carrier",
            *naval,
            none
        )
    );

    REQUIRE(
        definitions.add_military_formation(
            "carrier_air_group",
            *air,
            none
        )
    );

    auto const* carrier_definition =
        definitions.
            get_military_formation_by_identifier(
                "carrier"
            );

    auto const* air_definition =
        definitions.
            get_military_formation_by_identifier(
                "carrier_air_group"
            );

    REQUIRE(carrier_definition != nullptr);
    REQUIRE(air_definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Carrier",
            *carrier_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Air Group",
            *air_definition
        )
    );

    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "sea_region_alpha"
        )
    );

    REQUIRE(
        runtime.host_formation(
            2,
            1
        )
    );

    auto const* air_group =
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            );

    REQUIRE(air_group != nullptr);

    CHECK(air_group->is_hosted());
    CHECK_FALSE(
        air_group->has_direct_position()
    );

    CHECK(
        air_group->get_host_unique_id()
        ==
        1
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                2
            )
        ==
        "sea_region_alpha"
    );

    /*
     * Move the host. The guest has no duplicated spatial state;
     * its effective location follows automatically.
     */
    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "sea_region_beta"
        )
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                2
            )
        ==
        "sea_region_beta"
    );
}

TEST_CASE(
    "005A11 embarked land formation can detach and occupy directly",
    "[convergence][005a11][military][embarkation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("naval")
    );

    REQUIRE(
        domains.add_military_domain("land")
    );

    MilitaryFormationManager definitions;

    auto const* naval =
        domains.get_military_domain_by_identifier(
            "naval"
        );

    auto const* land =
        domains.get_military_domain_by_identifier(
            "land"
        );

    REQUIRE(naval != nullptr);
    REQUIRE(land != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "transport",
            *naval,
            none
        )
    );

    REQUIRE(
        definitions.add_military_formation(
            "embarked_troops",
            *land,
            none
        )
    );

    auto const* transport_definition =
        definitions.
            get_military_formation_by_identifier(
                "transport"
            );

    auto const* troop_definition =
        definitions.
            get_military_formation_by_identifier(
                "embarked_troops"
            );

    REQUIRE(transport_definition != nullptr);
    REQUIRE(troop_definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Transport",
            *transport_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Troops",
            *troop_definition
        )
    );

    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "sea_region_gamma"
        )
    );

    REQUIRE(
        runtime.host_formation(
            2,
            1
        )
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                2
            )
        ==
        "sea_region_gamma"
    );

    REQUIRE(
        runtime.detach_formation(2)
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                2
            ).empty()
    );

    REQUIRE(
        runtime.set_direct_operational_position(
            2,
            "province_beachhead"
        )
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                2
            )
        ==
        "province_beachhead"
    );

    auto const* troops =
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            );

    REQUIRE(troops != nullptr);

    CHECK_FALSE(troops->is_hosted());
    CHECK(troops->has_direct_position());
}

TEST_CASE(
    "005A11 hosting rejects self reference and cycles",
    "[convergence][005a11][military][validation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "generic"
        )
    );

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "generic_formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "generic_formation"
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

    REQUIRE(
        runtime.create_military_formation_instance(
            "Three",
            *definition
        )
    );

    CHECK_FALSE(
        runtime.host_formation(
            1,
            1
        )
    );

    REQUIRE(
        runtime.host_formation(
            2,
            1
        )
    );

    REQUIRE(
        runtime.host_formation(
            3,
            2
        )
    );

    /*
     * 1 -> 3 would create:
     *
     * 1 hosted by 3
     * 3 hosted by 2
     * 2 hosted by 1
     */
    CHECK_FALSE(
        runtime.host_formation(
            1,
            3
        )
    );

    auto const* one =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(one != nullptr);

    CHECK_FALSE(one->is_hosted());
}

TEST_CASE(
    "005A11 unplaced formation remains valid",
    "[convergence][005a11][military][optional-placement]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "nonspatial"
        )
    );

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "nonspatial"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "nonspatial_formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "nonspatial_formation"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Nonspatial",
            *definition
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK_FALSE(instance->has_direct_position());
    CHECK_FALSE(instance->is_hosted());

    CHECK(
        runtime.
            get_effective_operational_position_id(
                1
            ).empty()
    );
}
