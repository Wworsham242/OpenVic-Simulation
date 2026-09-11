#include "snitch/snitch.hpp"

#include "openvic-simulation/military/MilitaryDomain.hpp"

using namespace OpenVic;

TEST_CASE(
    "005A8 military domains are data-defined beyond LAND and NAVAL",
    "[convergence][005a8][military][domain]"
) {
    MilitaryDomainManager domains;

    REQUIRE(domains.setup_legacy_domains());

    REQUIRE(domains.add_military_domain("air"));
    REQUIRE(domains.add_military_domain("space"));

    CHECK(domains.get_military_domain_count() == 4);

    MilitaryDomainDefinition const* land =
        domains.get_military_domain_by_identifier("land");

    MilitaryDomainDefinition const* naval =
        domains.get_military_domain_by_identifier("naval");

    MilitaryDomainDefinition const* air =
        domains.get_military_domain_by_identifier("air");

    MilitaryDomainDefinition const* space =
        domains.get_military_domain_by_identifier("space");

    REQUIRE(land != nullptr);
    REQUIRE(naval != nullptr);
    REQUIRE(air != nullptr);
    REQUIRE(space != nullptr);

    CHECK(land->get_legacy_land_compatible());
    CHECK_FALSE(land->get_legacy_naval_compatible());

    CHECK(naval->get_legacy_naval_compatible());
    CHECK_FALSE(naval->get_legacy_land_compatible());

    CHECK_FALSE(air->get_legacy_land_compatible());
    CHECK_FALSE(air->get_legacy_naval_compatible());

    CHECK_FALSE(space->get_legacy_land_compatible());
    CHECK_FALSE(space->get_legacy_naval_compatible());
}

TEST_CASE(
    "005A8 legacy branches map onto generic domains",
    "[convergence][005a8][military][compatibility]"
) {
    MilitaryDomainManager domains;

    REQUIRE(domains.setup_legacy_domains());

    MilitaryDomainDefinition const* land =
        domains.get_domain_for_legacy_branch(
            unit_branch_t::LAND
        );

    MilitaryDomainDefinition const* naval =
        domains.get_domain_for_legacy_branch(
            unit_branch_t::NAVAL
        );

    REQUIRE(land != nullptr);
    REQUIRE(naval != nullptr);

    CHECK(land->get_identifier() == "land");
    CHECK(naval->get_identifier() == "naval");

    CHECK(
        domains.get_domain_for_legacy_branch(
            unit_branch_t::INVALID_BRANCH
        ) == nullptr
    );
}

TEST_CASE(
    "005A8 domain registry rejects duplicate identity",
    "[convergence][005a8][military][registry]"
) {
    MilitaryDomainManager domains;

    REQUIRE(domains.add_military_domain("air"));

    CHECK_FALSE(
        domains.add_military_domain("air")
    );

    CHECK(domains.get_military_domain_count() == 1);
}

TEST_CASE(
    "005A8 legacy compatibility ownership is unique",
    "[convergence][005a8][military][compatibility]"
) {
    MilitaryDomainManager domains;

    REQUIRE(
        domains.add_military_domain(
            "ground",
            true,
            false
        )
    );

    CHECK_FALSE(
        domains.add_military_domain(
            "land",
            true,
            false
        )
    );

    REQUIRE(
        domains.add_military_domain(
            "sea",
            false,
            true
        )
    );

    CHECK_FALSE(
        domains.add_military_domain(
            "naval",
            false,
            true
        )
    );
}

TEST_CASE(
    "005A8 one domain cannot masquerade as both legacy branches",
    "[convergence][005a8][military][validation]"
) {
    MilitaryDomainManager domains;

    CHECK_FALSE(
        domains.add_military_domain(
            "invalid",
            true,
            true
        )
    );

    CHECK(domains.get_military_domain_count() == 0);
}
