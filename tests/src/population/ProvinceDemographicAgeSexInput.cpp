#include "openvic-simulation/population/ProvinceDemographicAgeSexInput.hpp"

#include <array>
#include <cstdint>
#include <span>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
ProvinceDemographicAgeSexInput make_input(
char const* source_identifier,
demographic_age_sex_source_kind_t source_kind,
demographic_age_sex_geography_basis_t geography_basis,
Date reference_date,
Date revision_date,
uint16_t authority_priority,
demographic_age_band_t age_band =
demographic_age_band_t::AGE_20_24
) {
ProvinceDemographicAgeSexInput input {};

input.source_identifier =
source_identifier;

input.source_kind =
source_kind;

input.geography_basis =
geography_basis;

input.reference_date =
reference_date;

input.revision_date =
revision_date;

input.authority_priority =
authority_priority;

input.profile.get(age_band) =
{ 1, 1 };

return input;
}
}

TEST_CASE(
"004A10 source kind does not secretly determine authority",
"[convergence][004a10][population][demography][input]"
) {
const auto observation =
make_input(
"official-observation",
demographic_age_sex_source_kind_t::
OBSERVED_ENUMERATION,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2025, 7, 1 },
Date { 2026, 1, 1 },
10
);

const auto scenario =
make_input(
"scenario-authority",
demographic_age_sex_source_kind_t::
SCENARIO,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2025, 7, 1 },
Date { 2025, 1, 1 },
20
);

const std::array inputs {
observation,
scenario
};

const auto result =
resolve_province_demographic_age_sex_input(
inputs,
Date { 2025, 7, 1 }
);

REQUIRE(result.has_selection());
REQUIRE(result.selected != nullptr);

CHECK(
result.selected->source_identifier
== "scenario-authority"
);
}

TEST_CASE(
"004A10 latest eligible reference date wins within equal authority",
"[convergence][004a10][population][demography][input]"
) {
const auto older =
make_input(
"older",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2023, 7, 1 },
Date { 2024, 1, 1 },
10
);

const auto current =
make_input(
"current",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2024, 7, 1 },
Date { 2025, 1, 1 },
10
);

const auto future =
make_input(
"future",
demographic_age_sex_source_kind_t::
PROJECTION,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2026, 7, 1 },
Date { 2025, 1, 1 },
10
);

const std::array inputs {
older,
current,
future
};

const auto result =
resolve_province_demographic_age_sex_input(
inputs,
Date { 2025, 1, 1 }
);

REQUIRE(result.has_selection());

CHECK(
result.selected->source_identifier
== "current"
);

CHECK(result.eligible_count == 2);
}

TEST_CASE(
"004A10 newer revision wins for same reference date and authority",
"[convergence][004a10][population][demography][input]"
) {
const auto old_vintage =
make_input(
"vintage-2024",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2023, 7, 1 },
Date { 2024, 6, 1 },
10
);

const auto new_vintage =
make_input(
"vintage-2025",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2023, 7, 1 },
Date { 2025, 6, 1 },
10
);

const std::array inputs {
old_vintage,
new_vintage
};

const auto result =
resolve_province_demographic_age_sex_input(
inputs,
Date { 2025, 1, 1 }
);

REQUIRE(result.has_selection());

CHECK(
result.selected->source_identifier
== "vintage-2025"
);
}

TEST_CASE(
"004A10 exact precedence tie is ambiguous rather than guessed",
"[convergence][004a10][population][demography][input]"
) {
const auto first =
make_input(
"alpha",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2025, 7, 1 },
Date { 2026, 1, 1 },
10
);

const auto second =
make_input(
"zulu",
demographic_age_sex_source_kind_t::
SCENARIO,
demographic_age_sex_geography_basis_t::
EXPLICIT_GEOGRAPHIC_DISAGGREGATION,
Date { 2025, 7, 1 },
Date { 2026, 1, 1 },
10
);

const std::array inputs {
first,
second
};

const auto result =
resolve_province_demographic_age_sex_input(
inputs,
Date { 2025, 7, 1 }
);

CHECK(
result.status
== province_demographic_input_resolution_status_t::
AMBIGUOUS
);

CHECK_FALSE(result.has_selection());
CHECK(result.selected == nullptr);
CHECK(result.eligible_count == 2);
}

TEST_CASE(
"004A10 invalid provenance metadata is not eligible",
"[convergence][004a10][population][demography][input]"
) {
ProvinceDemographicAgeSexInput invalid {};

invalid.profile.get(
demographic_age_band_t::AGE_20_24
) = { 1, 1 };

invalid.reference_date =
Date { 2025, 7, 1 };

invalid.revision_date =
Date { 2025, 7, 1 };

invalid.authority_priority = 100;

const std::array inputs {
invalid
};

const auto result =
resolve_province_demographic_age_sex_input(
inputs,
Date { 2025, 7, 1 }
);

CHECK(
result.status
== province_demographic_input_resolution_status_t::
NONE
);

CHECK(result.eligible_count == 0);
}

TEST_CASE(
"004A10 selected input initializes exact 004A9 province authority",
"[convergence][004a10][population][demography][input][integration]"
) {
auto input =
make_input(
"province-census",
demographic_age_sex_source_kind_t::
OBSERVED_ENUMERATION,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2025, 7, 1 },
Date { 2026, 1, 1 },
10
);

input.profile.get(
demographic_age_band_t::AGE_0_4
) = { 3, 4 };

input.profile.get(
demographic_age_band_t::AGE_65_69
) = { 2, 1 };

const std::array inputs {
input
};

ProvinceDemographicAgeSexState state {};

const auto result =
initialize_province_demographic_age_sex_state_from_inputs(
state,
pop_sum_t { int64_t { 1003 } },
inputs,
Date { 2025, 7, 1 }
);

REQUIRE(result.initialized());
REQUIRE(result.selected != nullptr);

CHECK(
result.selected->source_identifier
== "province-census"
);

REQUIRE(state.has_structure());

CHECK(
state.get_structure_nullable()
->get_total_population()
== 1003
);

CHECK(
state.is_consistent_with_population(
pop_sum_t {
int64_t { 1003 }
}
)
);
}

TEST_CASE(
"004A10 ambiguous input does not mutate province demographic authority",
"[convergence][004a10][population][demography][input][safety]"
) {
const auto first =
make_input(
"first",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2025, 7, 1 },
Date { 2026, 1, 1 },
10
);

const auto second =
make_input(
"second",
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE,
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE,
Date { 2025, 7, 1 },
Date { 2026, 1, 1 },
10
);

const std::array inputs {
first,
second
};

ProvinceDemographicAgeSexState state {};

const auto result =
initialize_province_demographic_age_sex_state_from_inputs(
state,
pop_sum_t { int64_t { 100 } },
inputs,
Date { 2025, 7, 1 }
);

CHECK(
result.status
== province_demographic_input_application_status_t::
AMBIGUOUS
);

CHECK_FALSE(state.has_structure());
CHECK(state.get_structure_nullable() == nullptr);
}
