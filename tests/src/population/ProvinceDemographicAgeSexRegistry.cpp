#include "openvic-simulation/population/ProvinceDemographicAgeSexRegistry.hpp"

#include <cstdint>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {
ProvinceDemographicAgeSexInput make_registry_input(
char const* identifier,
uint16_t priority,
Date reference_date =
Date { 2025, 7, 1 },
Date revision_date =
Date { 2026, 1, 1 }
) {
ProvinceDemographicAgeSexInput input {};

input.source_identifier =
identifier;

input.source_kind =
demographic_age_sex_source_kind_t::
OFFICIAL_ESTIMATE;

input.geography_basis =
demographic_age_sex_geography_basis_t::
DIRECT_PROVINCE;

input.reference_date =
reference_date;

input.revision_date =
revision_date;

input.authority_priority =
priority;

input.profile.get(
demographic_age_band_t::AGE_20_24
) = { 1, 1 };

return input;
}
}

TEST_CASE(
"004A11 registry stores candidates by typed province index",
"[convergence][004a11][population][demography][registry]"
) {
ProvinceDemographicAgeSexRegistry registry { 3 };

CHECK(registry.get_province_capacity() == 3);
CHECK_FALSE(registry.is_locked());

CHECK(
registry.add_input(
province_index_t { 1 },
make_registry_input(
"province-one",
10
)
)
== province_demographic_registry_add_status_t::
ADDED
);

CHECK(
registry.get_candidate_count(
province_index_t { 1 }
)
== 1
);

CHECK(
registry.get_candidate_count(
province_index_t { 0 }
)
== 0
);
}

TEST_CASE(
"004A11 registry rejects invalid province index",
"[convergence][004a11][population][demography][registry][validation]"
) {
ProvinceDemographicAgeSexRegistry registry { 2 };

CHECK(
registry.add_input(
province_index_t { 2 },
make_registry_input(
"invalid",
10
)
)
== province_demographic_registry_add_status_t::
INVALID_PROVINCE
);

CHECK(
registry.get_candidate_count(
province_index_t { 2 }
)
== 0
);
}

TEST_CASE(
"004A11 candidate spans are unavailable until registry lock",
"[convergence][004a11][population][demography][registry][lifetime]"
) {
ProvinceDemographicAgeSexRegistry registry { 1 };

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"candidate",
10
)
)
== province_demographic_registry_add_status_t::
ADDED
);

CHECK(
registry.get_inputs(
province_index_t { 0 }
).empty()
);

registry.lock();

REQUIRE(registry.is_locked());

CHECK(
registry.get_inputs(
province_index_t { 0 }
).size()
== 1
);
}

TEST_CASE(
"004A11 locked registry refuses further mutation",
"[convergence][004a11][population][demography][registry][authority]"
) {
ProvinceDemographicAgeSexRegistry registry { 1 };

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"first",
10
)
)
== province_demographic_registry_add_status_t::
ADDED
);

registry.lock();

CHECK(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"second",
20
)
)
== province_demographic_registry_add_status_t::
LOCKED
);

CHECK(
registry.get_candidate_count(
province_index_t { 0 }
)
== 1
);
}

TEST_CASE(
"004A11 registry preserves multiple provenance candidates",
"[convergence][004a11][population][demography][registry][provenance]"
) {
ProvinceDemographicAgeSexRegistry registry { 1 };

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"older",
10,
Date { 2024, 7, 1 }
)
)
== province_demographic_registry_add_status_t::
ADDED
);

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"preferred",
20,
Date { 2025, 7, 1 }
)
)
== province_demographic_registry_add_status_t::
ADDED
);

registry.lock();

CHECK(
registry.get_inputs(
province_index_t { 0 }
).size()
== 2
);

const auto result =
registry.resolve(
province_index_t { 0 },
Date { 2025, 7, 1 }
);

REQUIRE(result.has_selection());
REQUIRE(result.selected != nullptr);

CHECK(
result.selected->source_identifier
== "preferred"
);
}

TEST_CASE(
"004A11 exact candidate tie remains ambiguous",
"[convergence][004a11][population][demography][registry][safety]"
) {
ProvinceDemographicAgeSexRegistry registry { 1 };

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"alpha",
10
)
)
== province_demographic_registry_add_status_t::
ADDED
);

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"beta",
10
)
)
== province_demographic_registry_add_status_t::
ADDED
);

registry.lock();

const auto result =
registry.resolve(
province_index_t { 0 },
Date { 2025, 7, 1 }
);

CHECK(
result.status
== province_demographic_input_resolution_status_t::
AMBIGUOUS
);

CHECK_FALSE(result.has_selection());
}

TEST_CASE(
"004A11 import record feeds locked province registry",
"[convergence][004a11][population][demography][registry][import]"
) {
ProvinceDemographicAgeSexRegistry registry { 2 };

ProvinceDemographicAgeSexImportRecord record {};

record.province_index =
province_index_t { 1 };

record.input =
make_registry_input(
"imported-profile",
30
);

REQUIRE(
registry.add_import_record(
std::move(record)
)
== province_demographic_registry_add_status_t::
ADDED
);

registry.lock();

const auto result =
registry.resolve(
province_index_t { 1 },
Date { 2025, 7, 1 }
);

REQUIRE(result.has_selection());

CHECK(
result.selected->source_identifier
== "imported-profile"
);
}

TEST_CASE(
"004A11 locked registry initializes exact province demographic state",
"[convergence][004a11][population][demography][registry][integration]"
) {
ProvinceDemographicAgeSexRegistry registry { 1 };

auto input =
make_registry_input(
"province-demographic-source",
20
);

input.profile.get(
demographic_age_band_t::AGE_0_4
) = { 3, 4 };

input.profile.get(
demographic_age_band_t::AGE_65_69
) = { 2, 1 };

REQUIRE(
registry.add_input(
province_index_t { 0 },
std::move(input)
)
== province_demographic_registry_add_status_t::
ADDED
);

registry.lock();

ProvinceDemographicAgeSexState state {};

const auto result =
registry.initialize_state(
province_index_t { 0 },
state,
pop_sum_t {
int64_t { 1003 }
},
Date { 2025, 7, 1 }
);

REQUIRE(result.initialized());
REQUIRE(result.selected != nullptr);
REQUIRE(state.has_structure());

CHECK(
result.selected->source_identifier
== "province-demographic-source"
);

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
"004A11 unresolved unlocked registry does not mutate authority",
"[convergence][004a11][population][demography][registry][lock]"
) {
ProvinceDemographicAgeSexRegistry registry { 1 };

REQUIRE(
registry.add_input(
province_index_t { 0 },
make_registry_input(
"candidate",
10
)
)
== province_demographic_registry_add_status_t::
ADDED
);

ProvinceDemographicAgeSexState state {};

const auto result =
registry.initialize_state(
province_index_t { 0 },
state,
pop_sum_t {
int64_t { 100 }
},
Date { 2025, 7, 1 }
);

CHECK_FALSE(result.initialized());
CHECK_FALSE(state.has_structure());
}
