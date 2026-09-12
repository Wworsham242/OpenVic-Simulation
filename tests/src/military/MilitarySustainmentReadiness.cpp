#include "snitch/snitch.hpp"

#include <array>
#include <string_view>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"
#include "openvic-simulation/military/MilitarySupport.hpp"

using namespace OpenVic;

namespace {

struct TestFormationFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryFormationDefinition const*
        definition = nullptr;

    TestFormationFixture(
        fixed_point_t initial_readiness
    ) {
        REQUIRE(
            domains.add_military_domain(
                "generic"
            )
        );

        auto const* domain =
            domains.
                get_military_domain_by_identifier(
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

        definition =
            definitions.
                get_military_formation_by_identifier(
                    "formation"
                );

        REQUIRE(definition != nullptr);

        REQUIRE(
            runtime.
                create_military_formation_instance(
                    "Formation",
                    *definition,
                    initial_readiness
                )
        );
    }

    MilitaryFormationInstance const*
    instance() const {
        return runtime.
            get_military_formation_instance_by_unique_id(
                1
            );
    }
};

}

TEST_CASE(
    "005A15 healthy sustainment permits bounded readiness recovery",
    "[convergence][005a15][military][recovery]"
) {
    TestFormationFixture fixture {
        fixed_point_t::_0_20
    };

    REQUIRE(
        fixture.runtime.adjust_readiness_toward(
            1,
            fixed_point_t::_1,
            fixed_point_t::_0_20
        )
    );

    auto const* instance =
        fixture.instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_1
    );

    CHECK(
        instance->get_readiness()
        ==
        (fixed_point_t::_0_20 * 2)
    );
}

TEST_CASE(
    "005A15 sustainment constrains readiness ceiling",
    "[convergence][005a15][military][ceiling]"
) {
    TestFormationFixture fixture {
        fixed_point_t::_0_20
    };

    auto* instance =
        fixture.runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

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

    /*
     * This test manipulates sustainment through its real 005A14
     * causal path rather than through direct state mutation.
     *
     * The existing formation definition has no declared support
     * requirement, so create a second definition that does.
     */
    MilitaryDomainManager dependent_domains;

    REQUIRE(
        dependent_domains.
            add_military_domain(
                "dependent"
            )
    );

    auto const* dependent_domain =
        dependent_domains.
            get_military_domain_by_identifier(
                "dependent"
            );

    REQUIRE(dependent_domain != nullptr);

    MilitaryFormationManager dependent_definitions;

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
        dependent_definitions.
            add_military_formation(
                "dependent_formation",
                *dependent_domain,
                no_capabilities,
                no_provisions,
                no_hosting_requirements,
                required_support
            )
    );

    auto const* dependent_definition =
        dependent_definitions.
            get_military_formation_by_identifier(
                "dependent_formation"
            );

    REQUIRE(
        dependent_definition != nullptr
    );

    MilitaryFormationInstanceManager runtime;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Dependent Formation",
            *dependent_definition,
            fixed_point_t::_0_20
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
                return
                    fixed_point_t::_0_50;
            }
        )
    );

    REQUIRE(
        runtime.adjust_readiness_toward(
            1,
            fixed_point_t::_1,
            fixed_point_t::_1
        )
    );

    auto const* dependent_instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(
        dependent_instance != nullptr
    );

    CHECK(
        dependent_instance->
            get_sustainment()
        ==
        fixed_point_t::_0_50
    );

    CHECK(
        dependent_instance->
            get_readiness()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A15 sustainment loss degrades readiness gradually",
    "[convergence][005a15][military][degradation]"
) {
    MilitaryDomainManager domains;
    REQUIRE(
        domains.add_military_domain(
            "generic"
        )
    );

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
        domains.
            get_military_domain_by_identifier(
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

    fixed_point_t const high_readiness =
        fixed_point_t::_1 -
        fixed_point_t::_0_10;

    REQUIRE(
        runtime.create_military_formation_instance(
            "Formation",
            *definition,
            high_readiness
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
                return
                    fixed_point_t::_0_50;
            }
        )
    );

    REQUIRE(
        runtime.adjust_readiness_toward(
            1,
            fixed_point_t::_1,
            fixed_point_t::_0_20
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

    CHECK(
        instance->get_readiness()
        ==
        (
            high_readiness -
            fixed_point_t::_0_20
        )
    );

    CHECK(
        instance->get_readiness()
        >
        instance->get_sustainment()
    );
}

TEST_CASE(
    "005A15 repeated updates converge to sustainment ceiling",
    "[convergence][005a15][military][convergence]"
) {
    MilitaryDomainManager domains;
    REQUIRE(
        domains.add_military_domain(
            "generic"
        )
    );

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
        domains.
            get_military_domain_by_identifier(
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
            *definition,
            fixed_point_t::_1
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
                return
                    fixed_point_t::_0_50;
            }
        )
    );

    for (int i = 0; i < 3; ++i) {
        REQUIRE(
            runtime.adjust_readiness_toward(
                1,
                fixed_point_t::_1,
                fixed_point_t::_0_20
            )
        );
    }

    auto const* instance =
        runtime.
            get_military_formation_instance_by_unique_id(
                1
            );

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A15 lower non-sustainment readiness target remains authoritative",
    "[convergence][005a15][military][multi-cause]"
) {
    TestFormationFixture fixture {
        fixed_point_t::_1
    };

    fixed_point_t const desired =
        fixed_point_t::_0_50;

    REQUIRE(
        fixture.runtime.adjust_readiness_toward(
            1,
            desired,
            fixed_point_t::_0_20
        )
    );

    auto const* instance =
        fixture.instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_1
    );

    CHECK(
        instance->get_readiness()
        ==
        (
            fixed_point_t::_1 -
            fixed_point_t::_0_20
        )
    );

    /*
     * This proves sustainment is not the sole readiness cause:
     * another mechanism may request a lower target even while
     * sustainment remains perfect.
     */
    CHECK(
        instance->get_readiness()
        >
        desired
    );
}

TEST_CASE(
    "005A15 invalid adjustment input does not mutate readiness",
    "[convergence][005a15][military][validation]"
) {
    TestFormationFixture fixture {
        fixed_point_t::_0_50
    };

    CHECK_FALSE(
        fixture.runtime.adjust_readiness_toward(
            1,
            fixed_point_t { 2 },
            fixed_point_t::_0_20
        )
    );

    auto const* after_bad_target =
        fixture.instance();

    REQUIRE(after_bad_target != nullptr);

    CHECK(
        after_bad_target->get_readiness()
        ==
        fixed_point_t::_0_50
    );

    CHECK_FALSE(
        fixture.runtime.adjust_readiness_toward(
            1,
            fixed_point_t::_1,
            fixed_point_t::_0
        )
    );

    auto const* after_bad_step =
        fixture.instance();

    REQUIRE(after_bad_step != nullptr);

    CHECK(
        after_bad_step->get_readiness()
        ==
        fixed_point_t::_0_50
    );
}
