#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A12 matching hosting profile admits guest",
    "[convergence][005a12][military][hosting-contract]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("naval")
    );

    REQUIRE(
        domains.add_military_domain("air")
    );

    MilitaryFormationManager definitions;

    REQUIRE(
        definitions.add_military_hosting_profile(
            "aviation_support"
        )
    );

    auto const* naval =
        domains.get_military_domain_by_identifier(
            "naval"
        );

    auto const* air =
        domains.get_military_domain_by_identifier(
            "air"
        );

    auto const* aviation =
        definitions.
            get_military_hosting_profile_by_identifier(
                "aviation_support"
            );

    REQUIRE(naval != nullptr);
    REQUIRE(air != nullptr);
    REQUIRE(aviation != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        1
    > host_provisions {{
        MilitaryHostingProvisionSpec {
            .profile = aviation,
            .capacity = fixed_point_t { 4 }
        }
    }};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_requirements {};

    REQUIRE(
        definitions.add_military_formation(
            "mobile_host",
            *naval,
            no_capabilities,
            host_provisions,
            no_requirements
        )
    );

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        1
    > guest_requirements {{
        MilitaryHostingRequirementSpec {
            .profile = aviation,
            .demand = fixed_point_t { 2 }
        }
    }};

    REQUIRE(
        definitions.add_military_formation(
            "hosted_air_group",
            *air,
            no_capabilities,
            no_provisions,
            guest_requirements
        )
    );

    auto const* host_definition =
        definitions.
            get_military_formation_by_identifier(
                "mobile_host"
            );

    auto const* guest_definition =
        definitions.
            get_military_formation_by_identifier(
                "hosted_air_group"
            );

    REQUIRE(host_definition != nullptr);
    REQUIRE(guest_definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Host",
            *host_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Guest",
            *guest_definition
        )
    );

    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "location_alpha"
        )
    );

    REQUIRE(
        runtime.host_formation(
            2,
            1
        )
    );

    auto const* guest =
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            );

    REQUIRE(guest != nullptr);
    CHECK(guest->is_hosted());

    CHECK(
        runtime.
            get_effective_operational_position_id(
                2
            )
        ==
        "location_alpha"
    );
}

TEST_CASE(
    "005A12 incompatible hosting profile is rejected",
    "[convergence][005a12][military][compatibility]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("host")
    );

    REQUIRE(
        domains.add_military_domain("guest")
    );

    MilitaryFormationManager definitions;

    REQUIRE(
        definitions.add_military_hosting_profile(
            "profile_a"
        )
    );

    REQUIRE(
        definitions.add_military_hosting_profile(
            "profile_b"
        )
    );

    auto const* host_domain =
        domains.get_military_domain_by_identifier(
            "host"
        );

    auto const* guest_domain =
        domains.get_military_domain_by_identifier(
            "guest"
        );

    auto const* profile_a =
        definitions.
            get_military_hosting_profile_by_identifier(
                "profile_a"
            );

    auto const* profile_b =
        definitions.
            get_military_hosting_profile_by_identifier(
                "profile_b"
            );

    REQUIRE(host_domain != nullptr);
    REQUIRE(guest_domain != nullptr);
    REQUIRE(profile_a != nullptr);
    REQUIRE(profile_b != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        1
    > provisions {{
        MilitaryHostingProvisionSpec {
            .profile = profile_a,
            .capacity = fixed_point_t { 10 }
        }
    }};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "host_definition",
            *host_domain,
            no_capabilities,
            provisions,
            none
        )
    );

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        1
    > requirements {{
        MilitaryHostingRequirementSpec {
            .profile = profile_b,
            .demand = fixed_point_t { 1 }
        }
    }};

    REQUIRE(
        definitions.add_military_formation(
            "guest_definition",
            *guest_domain,
            no_capabilities,
            no_provisions,
            requirements
        )
    );

    auto const* host_definition =
        definitions.
            get_military_formation_by_identifier(
                "host_definition"
            );

    auto const* guest_definition =
        definitions.
            get_military_formation_by_identifier(
                "guest_definition"
            );

    REQUIRE(host_definition != nullptr);
    REQUIRE(guest_definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Host",
            *host_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Guest",
            *guest_definition
        )
    );

    CHECK_FALSE(
        runtime.host_formation(
            2,
            1
        )
    );

    auto const* guest =
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            );

    REQUIRE(guest != nullptr);
    CHECK_FALSE(guest->is_hosted());
}

TEST_CASE(
    "005A12 hosting capacity is shared across guests",
    "[convergence][005a12][military][capacity]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("host")
    );

    REQUIRE(
        domains.add_military_domain("guest")
    );

    MilitaryFormationManager definitions;

    REQUIRE(
        definitions.add_military_hosting_profile(
            "transport_capacity"
        )
    );

    auto const* host_domain =
        domains.get_military_domain_by_identifier(
            "host"
        );

    auto const* guest_domain =
        domains.get_military_domain_by_identifier(
            "guest"
        );

    auto const* transport =
        definitions.
            get_military_hosting_profile_by_identifier(
                "transport_capacity"
            );

    REQUIRE(host_domain != nullptr);
    REQUIRE(guest_domain != nullptr);
    REQUIRE(transport != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        1
    > provisions {{
        MilitaryHostingProvisionSpec {
            .profile = transport,
            .capacity = fixed_point_t { 5 }
        }
    }};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_requirements {};

    REQUIRE(
        definitions.add_military_formation(
            "transport_host",
            *host_domain,
            no_capabilities,
            provisions,
            no_requirements
        )
    );

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        1
    > requirements {{
        MilitaryHostingRequirementSpec {
            .profile = transport,
            .demand = fixed_point_t { 3 }
        }
    }};

    REQUIRE(
        definitions.add_military_formation(
            "transport_guest",
            *guest_domain,
            no_capabilities,
            no_provisions,
            requirements
        )
    );

    auto const* host_definition =
        definitions.
            get_military_formation_by_identifier(
                "transport_host"
            );

    auto const* guest_definition =
        definitions.
            get_military_formation_by_identifier(
                "transport_guest"
            );

    REQUIRE(host_definition != nullptr);
    REQUIRE(guest_definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Host",
            *host_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Guest One",
            *guest_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Guest Two",
            *guest_definition
        )
    );

    REQUIRE(
        runtime.host_formation(
            2,
            1
        )
    );

    CHECK_FALSE(
        runtime.host_formation(
            3,
            1
        )
    );

    auto const* first =
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            );

    auto const* second =
        runtime.
            get_military_formation_instance_by_unique_id(
                3
            );

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    CHECK(first->is_hosted());
    CHECK_FALSE(second->is_hosted());

    /*
     * Capacity becomes available again after detachment.
     */
    REQUIRE(
        runtime.detach_formation(2)
    );

    REQUIRE(
        runtime.host_formation(
            3,
            1
        )
    );

    CHECK(second->is_hosted());
}

TEST_CASE(
    "005A12 host can provide multiple independent profiles",
    "[convergence][005a12][military][multi-profile]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("host")
    );

    REQUIRE(
        domains.add_military_domain("air")
    );

    REQUIRE(
        domains.add_military_domain("land")
    );

    MilitaryFormationManager definitions;

    REQUIRE(
        definitions.add_military_hosting_profile(
            "aviation"
        )
    );

    REQUIRE(
        definitions.add_military_hosting_profile(
            "personnel"
        )
    );

    auto const* host_domain =
        domains.get_military_domain_by_identifier(
            "host"
        );

    auto const* air_domain =
        domains.get_military_domain_by_identifier(
            "air"
        );

    auto const* land_domain =
        domains.get_military_domain_by_identifier(
            "land"
        );

    auto const* aviation =
        definitions.
            get_military_hosting_profile_by_identifier(
                "aviation"
            );

    auto const* personnel =
        definitions.
            get_military_hosting_profile_by_identifier(
                "personnel"
            );

    REQUIRE(host_domain != nullptr);
    REQUIRE(air_domain != nullptr);
    REQUIRE(land_domain != nullptr);
    REQUIRE(aviation != nullptr);
    REQUIRE(personnel != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        2
    > provisions {{
        MilitaryHostingProvisionSpec {
            .profile = aviation,
            .capacity = fixed_point_t { 2 }
        },
        MilitaryHostingProvisionSpec {
            .profile = personnel,
            .capacity = fixed_point_t { 4 }
        }
    }};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_requirements {};

    REQUIRE(
        definitions.add_military_formation(
            "multi_role_host",
            *host_domain,
            no_capabilities,
            provisions,
            no_requirements
        )
    );

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        1
    > air_requirement {{
        MilitaryHostingRequirementSpec {
            .profile = aviation,
            .demand = fixed_point_t { 2 }
        }
    }};

    std::array<
        MilitaryHostingRequirementSpec,
        1
    > land_requirement {{
        MilitaryHostingRequirementSpec {
            .profile = personnel,
            .demand = fixed_point_t { 4 }
        }
    }};

    REQUIRE(
        definitions.add_military_formation(
            "air_guest",
            *air_domain,
            no_capabilities,
            no_provisions,
            air_requirement
        )
    );

    REQUIRE(
        definitions.add_military_formation(
            "land_guest",
            *land_domain,
            no_capabilities,
            no_provisions,
            land_requirement
        )
    );

    auto const* host_definition =
        definitions.
            get_military_formation_by_identifier(
                "multi_role_host"
            );

    auto const* air_definition =
        definitions.
            get_military_formation_by_identifier(
                "air_guest"
            );

    auto const* land_definition =
        definitions.
            get_military_formation_by_identifier(
                "land_guest"
            );

    REQUIRE(host_definition != nullptr);
    REQUIRE(air_definition != nullptr);
    REQUIRE(land_definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Host",
            *host_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Air Guest",
            *air_definition
        )
    );

    REQUIRE(
        runtime.create_military_formation_instance(
            "Land Guest",
            *land_definition
        )
    );

    REQUIRE(runtime.host_formation(2, 1));
    REQUIRE(runtime.host_formation(3, 1));

    CHECK(
        runtime.
            get_military_formation_instance_by_unique_id(
                2
            )->is_hosted()
    );

    CHECK(
        runtime.
            get_military_formation_instance_by_unique_id(
                3
            )->is_hosted()
    );
}

TEST_CASE(
    "005A12 invalid hosting contract definitions are rejected",
    "[convergence][005a12][military][validation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("generic")
    );

    MilitaryFormationManager definitions;

    REQUIRE(
        definitions.add_military_hosting_profile(
            "profile"
        )
    );

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    auto const* profile =
        definitions.
            get_military_hosting_profile_by_identifier(
                "profile"
            );

    REQUIRE(domain != nullptr);
    REQUIRE(profile != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        1
    > invalid_provision {{
        MilitaryHostingProvisionSpec {
            .profile = profile,
            .capacity = 0
        }
    }};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_requirements {};

    CHECK_FALSE(
        definitions.add_military_formation(
            "invalid_capacity",
            *domain,
            no_capabilities,
            invalid_provision,
            no_requirements
        )
    );

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        1
    > invalid_requirement {{
        MilitaryHostingRequirementSpec {
            .profile = profile,
            .demand = 0
        }
    }};

    CHECK_FALSE(
        definitions.add_military_formation(
            "invalid_demand",
            *domain,
            no_capabilities,
            no_provisions,
            invalid_requirement
        )
    );
}
