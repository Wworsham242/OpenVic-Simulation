#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A9 generic formations compose domain and capabilities",
    "[convergence][005a9][military][formation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(domains.setup_legacy_domains());
    REQUIRE(
        domains.add_military_domain("air")
    );
    REQUIRE(
        domains.add_military_domain("space")
    );

    MilitaryFormationManager formations;

    REQUIRE(
        formations.add_military_capability(
            "mobility"
        )
    );

    REQUIRE(
        formations.add_military_capability(
            "strike"
        )
    );

    REQUIRE(
        formations.add_military_capability(
            "sensing"
        )
    );

    auto const* air =
        domains.get_military_domain_by_identifier(
            "air"
        );

    auto const* space =
        domains.get_military_domain_by_identifier(
            "space"
        );

    auto const* mobility =
        formations.
            get_military_capability_by_identifier(
                "mobility"
            );

    auto const* strike =
        formations.
            get_military_capability_by_identifier(
                "strike"
            );

    auto const* sensing =
        formations.
            get_military_capability_by_identifier(
                "sensing"
            );

    REQUIRE(air != nullptr);
    REQUIRE(space != nullptr);
    REQUIRE(mobility != nullptr);
    REQUIRE(strike != nullptr);
    REQUIRE(sensing != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        2
    > air_capabilities {
        mobility,
        strike
    };

    std::array<
        MilitaryCapabilityDefinition const*,
        1
    > space_capabilities {
        sensing
    };

    REQUIRE(
        formations.add_military_formation(
            "test_air_formation",
            *air,
            air_capabilities
        )
    );

    REQUIRE(
        formations.add_military_formation(
            "test_space_formation",
            *space,
            space_capabilities
        )
    );

    auto const* air_formation =
        formations.
            get_military_formation_by_identifier(
                "test_air_formation"
            );

    auto const* space_formation =
        formations.
            get_military_formation_by_identifier(
                "test_space_formation"
            );

    REQUIRE(air_formation != nullptr);
    REQUIRE(space_formation != nullptr);

    CHECK(
        air_formation->get_domain().
            get_identifier() ==
        "air"
    );

    CHECK(
        space_formation->get_domain().
            get_identifier() ==
        "space"
    );

    CHECK(
        air_formation->has_capability(
            *mobility
        )
    );

    CHECK(
        air_formation->has_capability(
            *strike
        )
    );

    CHECK_FALSE(
        air_formation->has_capability(
            *sensing
        )
    );

    CHECK(
        space_formation->has_capability(
            *sensing
        )
    );

    CHECK_FALSE(
        space_formation->has_capability(
            *strike
        )
    );
}

TEST_CASE(
    "005A9 capability identity is not domain specific",
    "[convergence][005a9][military][capability]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "domain_a"
        )
    );

    REQUIRE(
        domains.add_military_domain(
            "domain_b"
        )
    );

    MilitaryFormationManager formations;

    REQUIRE(
        formations.add_military_capability(
            "shared_capability"
        )
    );

    auto const* domain_a =
        domains.get_military_domain_by_identifier(
            "domain_a"
        );

    auto const* domain_b =
        domains.get_military_domain_by_identifier(
            "domain_b"
        );

    auto const* capability =
        formations.
            get_military_capability_by_identifier(
                "shared_capability"
            );

    REQUIRE(domain_a != nullptr);
    REQUIRE(domain_b != nullptr);
    REQUIRE(capability != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        1
    > capabilities {
        capability
    };

    REQUIRE(
        formations.add_military_formation(
            "formation_a",
            *domain_a,
            capabilities
        )
    );

    REQUIRE(
        formations.add_military_formation(
            "formation_b",
            *domain_b,
            capabilities
        )
    );

    CHECK(
        formations.
            get_military_formation_by_identifier(
                "formation_a"
            )->has_capability(*capability)
    );

    CHECK(
        formations.
            get_military_formation_by_identifier(
                "formation_b"
            )->has_capability(*capability)
    );
}

TEST_CASE(
    "005A9 formation rejects duplicate capability references",
    "[convergence][005a9][military][validation]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "test_domain"
        )
    );

    MilitaryFormationManager formations;

    REQUIRE(
        formations.add_military_capability(
            "test_capability"
        )
    );

    auto const* domain =
        domains.get_military_domain_by_identifier(
            "test_domain"
        );

    auto const* capability =
        formations.
            get_military_capability_by_identifier(
                "test_capability"
            );

    REQUIRE(domain != nullptr);
    REQUIRE(capability != nullptr);

    std::array<
        MilitaryCapabilityDefinition const*,
        2
    > duplicate_capabilities {
        capability,
        capability
    };

    CHECK_FALSE(
        formations.add_military_formation(
            "invalid_formation",
            *domain,
            duplicate_capabilities
        )
    );

    CHECK(
        formations.
            get_military_formation_count() ==
        0
    );
}

TEST_CASE(
    "005A9 generic formation requires no legacy unit branch",
    "[convergence][005a9][military][legacy-independence]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "nonlegacy"
        )
    );

    MilitaryFormationManager formations;

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
        formations.add_military_formation(
            "nonlegacy_formation",
            *domain,
            no_capabilities
        )
    );

    auto const* formation =
        formations.
            get_military_formation_by_identifier(
                "nonlegacy_formation"
            );

    REQUIRE(formation != nullptr);

    CHECK(
        formation->get_domain().
            get_identifier() ==
        "nonlegacy"
    );

    CHECK(
        formation->get_capabilities().
            empty()
    );
}
