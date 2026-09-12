#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

namespace {

struct EquipmentReadinessFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    EquipmentReadinessFixture(
        fixed_point_t initial_readiness =
            fixed_point_t::_1
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

        auto const* definition =
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

    MilitaryFormationInstance*
    mutable_instance() {
        return runtime.
            get_military_formation_instance_by_unique_id(
                1
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
    "005A16 equipment condition defaults healthy",
    "[convergence][005a16][military][equipment]"
) {
    EquipmentReadinessFixture fixture;

    auto const* instance =
        fixture.instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_equipment_condition()
        ==
        fixed_point_t::_1
    );

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_1
    );
}

TEST_CASE(
    "005A16 equipment condition independently constrains readiness",
    "[convergence][005a16][military][equipment][constraint]"
) {
    EquipmentReadinessFixture fixture {
        fixed_point_t::_1
    };

    auto* instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    REQUIRE(
        instance->set_equipment_condition(
            fixed_point_t::_0_50
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

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_1
    );

    CHECK(
        instance->get_equipment_condition()
        ==
        fixed_point_t::_0_50
    );

    CHECK(
        instance->get_readiness()
        ==
        fixed_point_t::_0_50
    );
}

TEST_CASE(
    "005A16 sustainment and equipment use lower independent constraint",
    "[convergence][005a16][military][multi-cause]"
) {
    EquipmentReadinessFixture fixture {
        fixed_point_t::_1
    };

    auto* instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    /*
     * Set equipment condition below perfect sustainment.
     */
    REQUIRE(
        instance->set_equipment_condition(
            fixed_point_t::_0_50
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

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        fixed_point_t::_0_50
    );

    /*
     * Improve equipment while another independent desired-readiness
     * cause remains lower.
     */
    REQUIRE(
        instance->set_equipment_condition(
            fixed_point_t::_1
        )
    );

    REQUIRE(
        instance->set_readiness(
            fixed_point_t::_1
        )
    );

    REQUIRE(
        fixture.runtime.
            adjust_readiness_toward(
                1,
                fixed_point_t::_0_20,
                fixed_point_t::_1
            )
    );

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_equipment_condition()
        ==
        fixed_point_t::_1
    );

    CHECK(
        instance->get_sustainment()
        ==
        fixed_point_t::_1
    );

    CHECK(
        instance->get_readiness()
        ==
        fixed_point_t::_0_20
    );
}

TEST_CASE(
    "005A16 equipment degradation affects readiness gradually",
    "[convergence][005a16][military][history]"
) {
    EquipmentReadinessFixture fixture {
        fixed_point_t::_1
    };

    auto* instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    REQUIRE(
        instance->set_equipment_condition(
            fixed_point_t::_0_50
        )
    );

    REQUIRE(
        fixture.runtime.
            adjust_readiness_toward(
                1,
                fixed_point_t::_1,
                fixed_point_t::_0_20
            )
    );

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        (
            fixed_point_t::_1 -
            fixed_point_t::_0_20
        )
    );

    CHECK(
        instance->get_readiness()
        >
        instance->get_equipment_condition()
    );

    REQUIRE(
        fixture.runtime.
            adjust_readiness_toward(
                1,
                fixed_point_t::_1,
                fixed_point_t::_0_20
            )
    );

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        (
            fixed_point_t::_1 -
            (
                fixed_point_t::_0_20 *
                2
            )
        )
    );
}

TEST_CASE(
    "005A16 equipment recovery permits readiness recovery",
    "[convergence][005a16][military][recovery]"
) {
    EquipmentReadinessFixture fixture {
        fixed_point_t::_0_20
    };

    auto* instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    REQUIRE(
        instance->set_equipment_condition(
            fixed_point_t::_0_20
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

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        fixed_point_t::_0_20
    );

    REQUIRE(
        instance->set_equipment_condition(
            fixed_point_t::_1
        )
    );

    REQUIRE(
        fixture.runtime.
            adjust_readiness_toward(
                1,
                fixed_point_t::_1,
                fixed_point_t::_0_20
            )
    );

    instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK(
        instance->get_readiness()
        ==
        (
            fixed_point_t::_0_20 *
            2
        )
    );
}

TEST_CASE(
    "005A16 invalid equipment condition does not mutate state",
    "[convergence][005a16][military][validation]"
) {
    EquipmentReadinessFixture fixture;

    auto* instance =
        fixture.mutable_instance();

    REQUIRE(instance != nullptr);

    CHECK_FALSE(
        instance->set_equipment_condition(
            fixed_point_t { 2 }
        )
    );

    CHECK(
        instance->get_equipment_condition()
        ==
        fixed_point_t::_1
    );

    CHECK_FALSE(
        instance->set_equipment_condition(
            fixed_point_t { -1 }
        )
    );

    CHECK(
        instance->get_equipment_condition()
        ==
        fixed_point_t::_1
    );
}
