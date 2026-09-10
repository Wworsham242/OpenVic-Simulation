#include "openvic-simulation/population/ProvinceDemographicAgeSexState.hpp"

#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
"004A9 province demographic state is absent by default",
"[convergence][004a9][population][demography][province]"
) {
ProvinceDemographicAgeSexState state {};

CHECK_FALSE(state.has_structure());
CHECK(state.get_structure_nullable() == nullptr);
}

TEST_CASE(
"004A9 province profile preserves authoritative aggregate total",
"[convergence][004a9][population][demography][province]"
) {
ProvinceDemographicAgeSexState state {};
DemographicAgeSexProfile profile {};

profile.get(
demographic_age_band_t::AGE_0_4
) = { 12, 13 };

profile.get(
demographic_age_band_t::AGE_20_24
) = { 30, 28 };

profile.get(
demographic_age_band_t::AGE_85_PLUS
) = { 10, 7 };

REQUIRE(
state.initialize(
profile,
pop_sum_t { int64_t { 1003 } }
)
);

REQUIRE(state.has_structure());
REQUIRE(
state.get_structure_nullable()
!= nullptr
);

CHECK(
state.get_structure_nullable()
->get_total_population()
== 1003
);

CHECK(
state.is_consistent_with_population(
pop_sum_t { int64_t { 1003 } }
)
);

CHECK_FALSE(
state.is_consistent_with_population(
pop_sum_t { int64_t { 1004 } }
)
);
}

TEST_CASE(
"004A9 province reconciliation supports totals above single POP range",
"[convergence][004a9][population][demography][province][large]"
) {
DemographicAgeSexProfile profile {};

profile.get(
demographic_age_band_t::AGE_20_24
) = { 1, 1 };

const auto result =
materialize_province_demographic_age_sex_profile(
profile,
pop_sum_t {
int64_t { 3000000000LL }
}
);

REQUIRE(result.valid);
CHECK_FALSE(result.cell_overflow);

CHECK(
result.structure.get_total_population()
== int64_t { 3000000000LL }
);

CHECK(
type_safe::get(
result.structure.get(
demographic_age_band_t::AGE_20_24
).female
)
== 1500000000
);

CHECK(
type_safe::get(
result.structure.get(
demographic_age_band_t::AGE_20_24
).male
)
== 1500000000
);
}

TEST_CASE(
"004A9 rejects province profile when one cohort cell would overflow",
"[convergence][004a9][population][demography][province][overflow]"
) {
DemographicAgeSexProfile profile {};

profile.get(
demographic_age_band_t::AGE_20_24
) = { 1, 0 };

const auto result =
materialize_province_demographic_age_sex_profile(
profile,
pop_sum_t {
int64_t { 3000000000LL }
}
);

CHECK_FALSE(result.valid);
CHECK(result.cell_overflow);
}

TEST_CASE(
"004A9 positive province population requires explicit demographic shape",
"[convergence][004a9][population][demography][province][validation]"
) {
ProvinceDemographicAgeSexState state {};
DemographicAgeSexProfile profile {};

CHECK_FALSE(
state.initialize(
profile,
pop_sum_t { int64_t { 100 } }
)
);

CHECK_FALSE(state.has_structure());
}

TEST_CASE(
"004A9 zero province population can initialize empty demographic structure",
"[convergence][004a9][population][demography][province][zero]"
) {
ProvinceDemographicAgeSexState state {};
DemographicAgeSexProfile profile {};

REQUIRE(
state.initialize(
profile,
pop_sum_t { int64_t { 0 } }
)
);

REQUIRE(state.has_structure());

CHECK(
state.get_structure_nullable()
->get_total_population()
== 0
);
}

TEST_CASE(
"004A9 province demographic initialization is one shot",
"[convergence][004a9][population][demography][province][authority]"
) {
ProvinceDemographicAgeSexState state {};

DemographicAgeSexProfile first {};
first.get(
demographic_age_band_t::AGE_20_24
) = { 1, 1 };

DemographicAgeSexProfile second {};
second.get(
demographic_age_band_t::AGE_65_69
) = { 1, 1 };

REQUIRE(
state.initialize(
first,
pop_sum_t { int64_t { 100 } }
)
);

PopulationAgeSexStructure const original =
*state.get_structure_nullable();

CHECK_FALSE(
state.initialize(
second,
pop_sum_t { int64_t { 100 } }
)
);

CHECK(
*state.get_structure_nullable()
== original
);
}
