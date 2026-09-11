#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"
#include "openvic-simulation/military/MilitarySupport.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A13 formation may have no support relationships",
    "[convergence][005a13][military][optional-support]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("generic")
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
            "formation",
            *domain,
            none
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
            "Formation",
            *definition
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->
            get_support_relationships().
            empty()
    );
}

TEST_CASE(
    "005A13 layered support overlaps operational placement",
    "[convergence][005a13][military][layered-support]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("air")
    );

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "air"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "air_formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "air_formation"
            );

    REQUIRE(definition != nullptr);

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "home_station"
        )
    );

    REQUIRE(
        support.add_military_support_type(
            "forward_support"
        )
    );

    REQUIRE(
        support.add_military_support_type(
            "maintenance"
        )
    );

    auto const* home =
        support.
            get_military_support_type_by_identifier(
                "home_station"
            );

    auto const* forward =
        support.
            get_military_support_type_by_identifier(
                "forward_support"
            );

    auto const* maintenance =
        support.
            get_military_support_type_by_identifier(
                "maintenance"
            );

    REQUIRE(home != nullptr);
    REQUIRE(forward != nullptr);
    REQUIRE(maintenance != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Air Formation",
            *definition
        )
    );

    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "forward_operating_location"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *home,
            "main_air_station"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *forward,
            "forward_support_site"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *maintenance,
            "regional_maintenance_hub"
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->
            get_support_relationships().
            size()
        ==
        3
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                1
            )
        ==
        "forward_operating_location"
    );

    CHECK(
        instance->has_support_relationship(
            *home,
            "main_air_station"
        )
    );

    CHECK(
        instance->has_support_relationship(
            *forward,
            "forward_support_site"
        )
    );

    CHECK(
        instance->has_support_relationship(
            *maintenance,
            "regional_maintenance_hub"
        )
    );
}

TEST_CASE(
    "005A13 one support type may reference multiple nodes",
    "[convergence][005a13][military][redundancy]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("naval")
    );

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "naval"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "naval_formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "naval_formation"
            );

    REQUIRE(definition != nullptr);

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "logistics_support"
        )
    );

    auto const* logistics =
        support.
            get_military_support_type_by_identifier(
                "logistics_support"
            );

    REQUIRE(logistics != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Naval Formation",
            *definition
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *logistics,
            "support_node_a"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *logistics,
            "support_node_b"
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->
            get_support_relationships().
            size()
        ==
        2
    );
}

TEST_CASE(
    "005A13 support links coexist with current host",
    "[convergence][005a13][military][hosting-overlap]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("host")
    );

    REQUIRE(
        domains.add_military_domain("guest")
    );

    MilitaryFormationManager definitions;

    auto const* host_domain =
        domains.get_military_domain_by_identifier(
            "host"
        );

    auto const* guest_domain =
        domains.get_military_domain_by_identifier(
            "guest"
        );

    REQUIRE(host_domain != nullptr);
    REQUIRE(guest_domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > none {};

    REQUIRE(
        definitions.add_military_formation(
            "host_definition",
            *host_domain,
            none
        )
    );

    REQUIRE(
        definitions.add_military_formation(
            "guest_definition",
            *guest_domain,
            none
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

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "home_station"
        )
    );

    auto const* home =
        support.
            get_military_support_type_by_identifier(
                "home_station"
            );

    REQUIRE(home != nullptr);

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
            "mobile_host_location"
        )
    );

    REQUIRE(
        runtime.host_formation(
            2,
            1
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            2,
            *home,
            "permanent_home_station"
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
        "mobile_host_location"
    );

    CHECK(
        guest->has_support_relationship(
            *home,
            "permanent_home_station"
        )
    );
}

TEST_CASE(
    "005A13 exact duplicate support relationship is rejected",
    "[convergence][005a13][military][validation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("generic")
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
            "formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "formation"
            );

    REQUIRE(definition != nullptr);

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "support"
        )
    );

    auto const* support_type =
        support.
            get_military_support_type_by_identifier(
                "support"
            );

    REQUIRE(support_type != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Formation",
            *definition
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *support_type,
            "node"
        )
    );

    CHECK_FALSE(
        runtime.add_support_relationship(
            1,
            *support_type,
            "node"
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->
            get_support_relationships().
            size()
        ==
        1
    );
}

TEST_CASE(
    "005A13 one support link can be removed independently",
    "[convergence][005a13][military][removal]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain("generic")
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
            "formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "formation"
            );

    REQUIRE(definition != nullptr);

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "support"
        )
    );

    auto const* support_type =
        support.
            get_military_support_type_by_identifier(
                "support"
            );

    REQUIRE(support_type != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Formation",
            *definition
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *support_type,
            "node_a"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *support_type,
            "node_b"
        )
    );

    REQUIRE(
        runtime.remove_support_relationship(
            1,
            *support_type,
            "node_a"
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK_FALSE(
        instance->has_support_relationship(
            *support_type,
            "node_a"
        )
    );

    CHECK(
        instance->has_support_relationship(
            *support_type,
            "node_b"
        )
    );

    CHECK(
        instance->
            get_support_relationships().
            size()
        ==
        1
    );
}
