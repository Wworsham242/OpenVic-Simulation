#include "snitch/snitch.hpp"

#include <array>
#include <string_view>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"
#include "openvic-simulation/military/MilitarySupport.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A14 undeclared support remains optional",
    "[convergence][005a14][military][optional-support]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

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
            "austere_formation",
            *domain,
            none
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "austere_formation"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Austere Formation",
            *definition
        )
    );

    bool provider_called = false;

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [&provider_called](
                std::string_view
            ) {
                provider_called = true;
                return fixed_point_t { 0 };
            }
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK_FALSE(provider_called);

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_1
    );
}

TEST_CASE(
    "005A14 missing declared support collapses sustainment",
    "[convergence][005a14][military][missing-support]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "required_logistics"
        )
    );

    auto const* logistics =
        support.
            get_military_support_type_by_identifier(
                "required_logistics"
            );

    REQUIRE(logistics != nullptr);

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_hosting_requirements {};

    std::array<
        MilitarySupportTypeDefinition const*,
        1
    > required_support {{
        logistics
    }};

    REQUIRE(
        definitions.add_military_formation(
            "dependent_formation",
            *domain,
            no_capabilities,
            no_provisions,
            no_hosting_requirements,
            required_support
        )
    );

    auto const* definition =
        definitions.
            get_military_formation_by_identifier(
                "dependent_formation"
            );

    REQUIRE(definition != nullptr);

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Dependent Formation",
            *definition
        )
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view
            ) {
                return fixed_point_t::_1;
            }
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A14 support target availability drives sustainment",
    "[convergence][005a14][military][availability]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "maintenance"
        )
    );

    auto const* maintenance =
        support.
            get_military_support_type_by_identifier(
                "maintenance"
            );

    REQUIRE(maintenance != nullptr);

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_hosting_requirements {};

    std::array<
        MilitarySupportTypeDefinition const*,
        1
    > required_support {{
        maintenance
    }};

    REQUIRE(
        definitions.add_military_formation(
            "formation",
            *domain,
            no_capabilities,
            no_provisions,
            no_hosting_requirements,
            required_support
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

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *maintenance,
            "maintenance_node"
        )
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view target
            ) {
                if (
                    target ==
                    "maintenance_node"
                ) {
                    return (fixed_point_t { 3 } / 5);
                }

                return fixed_point_t::_0;
            }
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        (fixed_point_t { 3 } / 5)
    );
}

TEST_CASE(
    "005A14 redundant same-type support uses best available node",
    "[convergence][005a14][military][redundancy]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "logistics"
        )
    );

    auto const* logistics =
        support.
            get_military_support_type_by_identifier(
                "logistics"
            );

    REQUIRE(logistics != nullptr);

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_hosting_requirements {};

    std::array<
        MilitarySupportTypeDefinition const*,
        1
    > required_support {{
        logistics
    }};

    REQUIRE(
        definitions.add_military_formation(
            "formation",
            *domain,
            no_capabilities,
            no_provisions,
            no_hosting_requirements,
            required_support
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

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *logistics,
            "node_a"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *logistics,
            "node_b"
        )
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view target
            ) {
                if (target == "node_a") {
                    return fixed_point_t::_0_20;
                }

                if (target == "node_b") {
                    return (fixed_point_t { 4 } / 5);
                }

                return fixed_point_t::_0;
            }
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        (fixed_point_t { 4 } / 5)
    );
}

TEST_CASE(
    "005A14 different required support types form bottleneck",
    "[convergence][005a14][military][bottleneck]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

    MilitarySupportManager support;

    REQUIRE(
        support.add_military_support_type(
            "logistics"
        )
    );

    REQUIRE(
        support.add_military_support_type(
            "maintenance"
        )
    );

    auto const* logistics =
        support.
            get_military_support_type_by_identifier(
                "logistics"
            );

    auto const* maintenance =
        support.
            get_military_support_type_by_identifier(
                "maintenance"
            );

    REQUIRE(logistics != nullptr);
    REQUIRE(maintenance != nullptr);

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_hosting_requirements {};

    std::array<
        MilitarySupportTypeDefinition const*,
        2
    > required_support {{
        logistics,
        maintenance
    }};

    REQUIRE(
        definitions.add_military_formation(
            "formation",
            *domain,
            no_capabilities,
            no_provisions,
            no_hosting_requirements,
            required_support
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

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *logistics,
            "logistics_node"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *maintenance,
            "maintenance_node"
        )
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view target
            ) {
                if (
                    target ==
                    "logistics_node"
                ) {
                    return (fixed_point_t { 4 } / 5);
                }

                if (
                    target ==
                    "maintenance_node"
                ) {
                    return fixed_point_t::_0_50;
                }

                return fixed_point_t::_0;
            }
        )
    );

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A14 support loss changes sustainment without moving formation",
    "[convergence][005a14][military][causal]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

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

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_hosting_requirements {};

    std::array<
        MilitarySupportTypeDefinition const*,
        1
    > required_support {{
        support_type
    }};

    REQUIRE(
        definitions.add_military_formation(
            "formation",
            *domain,
            no_capabilities,
            no_provisions,
            no_hosting_requirements,
            required_support
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

    REQUIRE(
        runtime.set_direct_operational_position(
            1,
            "operational_location"
        )
    );

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *support_type,
            "support_node"
        )
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view
            ) {
                return fixed_point_t::_1;
            }
        )
    );

    auto const* before =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(before != nullptr);

    CHECK(
        before->get_sustainment()
        ==
        fixed_point_t::_1
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view
            ) {
                return fixed_point_t::_0;
            }
        )
    );

    auto const* after =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(after != nullptr);

    CHECK(
        after->get_sustainment()
        ==
        fixed_point_t::_0
    );

    CHECK(
        runtime.
            get_effective_operational_position_id(
                1
            )
        ==
        "operational_location"
    );
}

TEST_CASE(
    "005A14 invalid provider value does not mutate sustainment",
    "[convergence][005a14][military][validation]"
) {
    MilitaryDomainManager domains;
    REQUIRE(domains.add_military_domain("generic"));

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

    MilitaryFormationManager definitions;

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "generic"
        );

    REQUIRE(domain != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        0
    > no_capabilities {};

    std::array<
        MilitaryHostingProvisionSpec,
        0
    > no_provisions {};

    std::array<
        MilitaryHostingRequirementSpec,
        0
    > no_hosting_requirements {};

    std::array<
        MilitarySupportTypeDefinition const*,
        1
    > required_support {{
        support_type
    }};

    REQUIRE(
        definitions.add_military_formation(
            "formation",
            *domain,
            no_capabilities,
            no_provisions,
            no_hosting_requirements,
            required_support
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

    REQUIRE(
        runtime.add_support_relationship(
            1,
            *support_type,
            "support_node"
        )
    );

    REQUIRE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view
            ) {
                return (fixed_point_t { 7 } / 10);
            }
        )
    );

    auto const* before =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(before != nullptr);

    CHECK(
        before->get_sustainment()
        ==
        (fixed_point_t { 7 } / 10)
    );

    CHECK_FALSE(
        runtime.evaluate_support_sustainment(
            1,
            [](
                std::string_view
            ) {
                return fixed_point_t {
                    2
                };
            }
        )
    );

    auto const* after =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(after != nullptr);

    CHECK(
        after->get_sustainment()
        ==
        (fixed_point_t { 7 } / 10)
    );
}
