#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitarySustainmentStock.hpp"

using namespace OpenVic;

namespace {

struct SustainmentFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    SustainmentFixture() {
        REQUIRE(
            domains.add_military_domain("generic")
        );

        auto const* domain =
            domains.get_military_domain_by_identifier(
                "generic"
            );

        REQUIRE(domain != nullptr);

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

        std::array<MilitaryEquipmentRequirementSpec, 0>
            equipment {};

        REQUIRE(
            definitions.add_military_formation(
                "formation",
                *domain,
                capabilities,
                provisions,
                hosting,
                support,
                equipment
            )
        );

        auto const* definition =
            definitions.get_military_formation_by_identifier(
                "formation"
            );

        REQUIRE(definition != nullptr);

        REQUIRE(
            runtime.create_military_formation_instance(
                "Formation",
                *definition
            )
        );
    }
};

}

TEST_CASE(
    "005A28 sustainment stock persists aggregate quantity and target shortfall",
    "[convergence][005a28][military][sustainment][stock]"
) {
    SustainmentFixture fixture;
    MilitarySustainmentStockState state;

    REQUIRE(
        state.add_stock(
            fixture.runtime,
            1,
            "generic_consumable",
            fixed_point_t { 8 },
            fixed_point_t { 10 },
            fixed_point_t { 5 }
        )
    );

    CHECK(
        state.get_quantity(1, "generic_consumable") ==
        fixed_point_t { 5 }
    );

    CHECK(
        state.get_target_shortfall(
            1,
            "generic_consumable"
        ) == fixed_point_t { 3 }
    );

    CHECK(
        state.get_remaining_capacity(
            1,
            "generic_consumable"
        ) == fixed_point_t { 5 }
    );
}

TEST_CASE(
    "005A28 receipt is capped by physical storage capacity",
    "[convergence][005a28][military][sustainment][receipt]"
) {
    SustainmentFixture fixture;
    MilitarySustainmentStockState state;

    REQUIRE(
        state.add_stock(
            fixture.runtime,
            1,
            "supply",
            fixed_point_t { 8 },
            fixed_point_t { 10 },
            fixed_point_t { 7 }
        )
    );

    fixed_point_t accepted;

    REQUIRE(
        state.receive(
            1,
            "supply",
            fixed_point_t { 6 },
            accepted
        )
    );

    CHECK(accepted == fixed_point_t { 3 });

    CHECK(
        state.get_quantity(1, "supply") ==
        fixed_point_t { 10 }
    );

    CHECK(
        state.get_remaining_capacity(1, "supply") ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A28 consumption cannot create negative stock",
    "[convergence][005a28][military][sustainment][consumption]"
) {
    SustainmentFixture fixture;
    MilitarySustainmentStockState state;

    REQUIRE(
        state.add_stock(
            fixture.runtime,
            1,
            "supply",
            fixed_point_t { 8 },
            fixed_point_t { 10 },
            fixed_point_t { 4 }
        )
    );

    fixed_point_t consumed;

    REQUIRE(
        state.consume(
            1,
            "supply",
            fixed_point_t { 7 },
            consumed
        )
    );

    CHECK(consumed == fixed_point_t { 4 });

    CHECK(
        state.get_quantity(1, "supply") ==
        fixed_point_t::_0
    );

    CHECK(
        state.get_target_shortfall(1, "supply") ==
        fixed_point_t { 8 }
    );
}

TEST_CASE(
    "005A28 sustainment item identity remains data defined",
    "[convergence][005a28][military][sustainment][generic]"
) {
    SustainmentFixture fixture;
    MilitarySustainmentStockState state;

    REQUIRE(
        state.add_stock(
            fixture.runtime,
            1,
            "scenario_defined_resource",
            fixed_point_t { 5 },
            fixed_point_t { 6 },
            fixed_point_t { 2 }
        )
    );

    CHECK(
        state.get_quantity(
            1,
            "scenario_defined_resource"
        ) == fixed_point_t { 2 }
    );

    CHECK_FALSE(
        state.add_stock(
            fixture.runtime,
            1,
            "scenario_defined_resource",
            fixed_point_t { 5 },
            fixed_point_t { 6 },
            fixed_point_t::_0
        )
    );
}
