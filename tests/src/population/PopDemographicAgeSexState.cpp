#include "openvic-simulation/population/PopDemographicAgeSexState.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
"004A8 demographic state is absent by default",
"[convergence][004a8][population][demography]"
) {
PopDemographicAgeSexState state {};

CHECK_FALSE(state.has_structure());
CHECK(state.get_structure_nullable() == nullptr);
}

TEST_CASE(
"004A8 explicit profile preserves authoritative total",
"[convergence][004a8][population][demography]"
) {
PopDemographicAgeSexState state {};
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
pop_size_t { 1003 }
)
);

REQUIRE(state.has_structure());
REQUIRE(state.get_structure_nullable() != nullptr);

CHECK(
state.get_structure_nullable()->get_total_population()
== 1003
);

CHECK(
state.is_consistent_with_population(
pop_size_t { 1003 }
)
);
}

TEST_CASE(
"004A8 invalid positive-population profile stays absent",
"[convergence][004a8][population][demography]"
) {
PopDemographicAgeSexState state {};
DemographicAgeSexProfile profile {};

CHECK_FALSE(
state.initialize(
profile,
pop_size_t { 100 }
)
);

CHECK_FALSE(state.has_structure());
}

TEST_CASE(
"004A8 initialization is one shot",
"[convergence][004a8][population][demography]"
) {
PopDemographicAgeSexState state {};

DemographicAgeSexProfile first {};
first.get(
demographic_age_band_t::AGE_20_24
) = { 2, 3 };

DemographicAgeSexProfile second {};
second.get(
demographic_age_band_t::AGE_65_69
) = { 4, 1 };

REQUIRE(
state.initialize(
first,
pop_size_t { 100 }
)
);

PopulationAgeSexStructure const original =
*state.get_structure_nullable();

CHECK_FALSE(
state.initialize(
second,
pop_size_t { 100 }
)
);

CHECK(
*state.get_structure_nullable()
== original
);
}

TEST_CASE(
"004A8 zero population may initialize empty structure",
"[convergence][004a8][population][demography]"
) {
PopDemographicAgeSexState state {};
DemographicAgeSexProfile profile {};

REQUIRE(
state.initialize(
profile,
pop_size_t { 0 }
)
);

REQUIRE(state.has_structure());

CHECK(
state.get_structure_nullable()->get_total_population()
== 0
);
}

TEST_CASE(
"004A8 remains deterministic",
"[convergence][004a8][population][demography]"
) {
DemographicAgeSexProfile profile {};

profile.get(
demographic_age_band_t::AGE_0_4
) = { 1, 1 };

profile.get(
demographic_age_band_t::AGE_5_9
) = { 1, 1 };

PopDemographicAgeSexState first {};
PopDemographicAgeSexState second {};

REQUIRE(
first.initialize(
profile,
pop_size_t { 3 }
)
);

REQUIRE(
second.initialize(
profile,
pop_size_t { 3 }
)
);

CHECK(
*first.get_structure_nullable()
== *second.get_structure_nullable()
);
}
