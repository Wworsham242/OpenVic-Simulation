#include "openvic-simulation/population/DemographicMigrationTransfer.hpp"

#include <cstdint>
#include <limits>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {

PopulationAgeSexStructure make_origin() {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 100 },
pop_size_t { 120 }
};

population.get(
demographic_age_band_t::AGE_30_34
) = {
pop_size_t { 80 },
pop_size_t { 70 }
};

return population;
}

PopulationAgeSexStructure make_destination() {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 40 },
pop_size_t { 50 }
};

population.get(
demographic_age_band_t::AGE_30_34
) = {
pop_size_t { 30 },
pop_size_t { 20 }
};

return population;
}

}

TEST_CASE(
"006B1.5 zero migration transfer is identity",
"[convergence][006b1][006b1.5][population][demography][migration]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer const transfer {};

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(result.valid());

CHECK(result.migrants == 0);
CHECK(result.origin_ending == origin);
CHECK(result.destination_ending == destination);

CHECK(
result.combined_ending_population
== result.combined_starting_population
);
}

TEST_CASE(
"006B1.5 transfer preserves exact age sex cohort identity",
"[convergence][006b1][006b1.5][population][demography][migration]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 7 },
pop_size_t { 9 }
};

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.origin_ending.get(
demographic_age_band_t::AGE_20_24
).female
) == 93
);

CHECK(
type_safe::get(
result.origin_ending.get(
demographic_age_band_t::AGE_20_24
).male
) == 111
);

CHECK(
type_safe::get(
result.destination_ending.get(
demographic_age_band_t::AGE_20_24
).female
) == 47
);

CHECK(
type_safe::get(
result.destination_ending.get(
demographic_age_band_t::AGE_20_24
).male
) == 59
);
}

TEST_CASE(
"006B1.5 origin loss exactly equals destination gain",
"[convergence][006b1][006b1.5][population][demography][migration][conservation]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 10 },
pop_size_t { 11 }
};

transfer.migrants.get(
demographic_age_band_t::AGE_30_34
) = {
pop_size_t { 5 },
pop_size_t { 6 }
};

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(result.valid());

CHECK(result.migrants == 32);

CHECK(
origin.get_total_population()
- result.origin_ending.get_total_population()
== 32
);

CHECK(
result.destination_ending.get_total_population()
- destination.get_total_population()
== 32
);
}

TEST_CASE(
"006B1.5 migration conserves combined population exactly",
"[convergence][006b1][006b1.5][population][demography][migration][conservation]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_30_34
) = {
pop_size_t { 40 },
pop_size_t { 30 }
};

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(result.valid());

CHECK(
result.combined_starting_population
== result.combined_ending_population
);
}

TEST_CASE(
"006B1.5 rejects transfer larger than origin cohort",
"[convergence][006b1][006b1.5][population][demography][migration][validation]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 101 };

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_migration_transfer_status_t::
ORIGIN_TRANSFER_REJECTED
);

CHECK(result.origin_ending == origin);
CHECK(result.destination_ending == destination);
}

TEST_CASE(
"006B1.5 rejects negative migration transfer",
"[convergence][006b1][006b1.5][population][demography][migration][validation]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
).male = pop_size_t { -1 };

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_migration_transfer_status_t::
INVALID_TRANSFER
);
}

TEST_CASE(
"006B1.5 destination overflow rejects the entire transfer atomically",
"[convergence][006b1][006b1.5][population][demography][migration][overflow]"
) {
auto const origin = make_origin();

PopulationAgeSexStructure destination {};

destination.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t {
std::numeric_limits<int32_t>::max()
};

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1 };

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_migration_transfer_status_t::
DESTINATION_TRANSFER_REJECTED
);

CHECK(result.origin_ending == origin);
CHECK(result.destination_ending == destination);
}

TEST_CASE(
"006B1.5 migration transfer is geography type agnostic",
"[convergence][006b1][006b1.5][population][demography][migration][general]"
) {
/*
 * The substrate accepts demographic stocks and a transfer only.
 * It has no country, colony, state, province, or player-specific branch.
 */
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 4 };

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(result.valid());
CHECK(result.migrants == 4);
}

TEST_CASE(
"006B1.5 mobility pressure does not itself move population",
"[convergence][006b1][006b1.5][population][demography][migration][boundary]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

/*
 * No transfer means no migration. Upstream mobility pressure belongs
 * to a later decision/flow producer, not this authority seam.
 */
DemographicMigrationTransfer const transfer {};

auto const result =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(result.valid());
CHECK(result.origin_ending == origin);
CHECK(result.destination_ending == destination);
}

TEST_CASE(
"006B1.5 identical transfers produce identical results",
"[convergence][006b1][006b1.5][population][demography][migration][determinism]"
) {
auto const origin = make_origin();
auto const destination = make_destination();

DemographicMigrationTransfer transfer {};

transfer.migrants.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 17 },
pop_size_t { 19 }
};

transfer.migrants.get(
demographic_age_band_t::AGE_30_34
) = {
pop_size_t { 3 },
pop_size_t { 7 }
};

auto const first =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

auto const second =
apply_demographic_migration_transfer(
origin,
destination,
transfer
);

REQUIRE(first.valid());
REQUIRE(second.valid());

CHECK(first == second);
}