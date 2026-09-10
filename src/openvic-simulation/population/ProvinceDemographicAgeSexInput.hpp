#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/population/ProvinceDemographicAgeSexState.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {
enum class demographic_age_sex_source_kind_t : uint8_t {
UNSPECIFIED = 0,
OBSERVED_ENUMERATION,
OFFICIAL_ESTIMATE,
PROJECTION,
SCENARIO,
MODEL_DERIVED
};

enum class demographic_age_sex_geography_basis_t : uint8_t {
UNSPECIFIED = 0,
DIRECT_PROVINCE,
EXPLICIT_GEOGRAPHIC_DISAGGREGATION,
EXPLICIT_PARENT_GEOGRAPHY_PROXY
};

/*
 * One explicitly supplied candidate for initializing a province's
 * demographic age-sex authority.
 *
 * source_kind and geography_basis are provenance. They do not
 * silently determine precedence.
 *
 * authority_priority is explicit scenario/data policy.
 */
struct ProvinceDemographicAgeSexInput {
DemographicAgeSexProfile profile {};

demographic_age_sex_source_kind_t source_kind =
demographic_age_sex_source_kind_t::UNSPECIFIED;

demographic_age_sex_geography_basis_t geography_basis =
demographic_age_sex_geography_basis_t::UNSPECIFIED;

Date reference_date {};
Date revision_date {};

uint16_t authority_priority = 0;

memory::string source_identifier {};

[[nodiscard]]
bool has_valid_metadata() const {
return
source_kind
!= demographic_age_sex_source_kind_t::UNSPECIFIED
&& geography_basis
!= demographic_age_sex_geography_basis_t::UNSPECIFIED
&& !source_identifier.empty();
}

[[nodiscard]]
bool is_eligible_for(Date target_date) const {
return
has_valid_metadata()
&& reference_date <= target_date;
}
};

enum class province_demographic_input_resolution_status_t : uint8_t {
NONE = 0,
SELECTED,
AMBIGUOUS
};

struct ProvinceDemographicAgeSexInputResolution {
ProvinceDemographicAgeSexInput const* selected = nullptr;

province_demographic_input_resolution_status_t status =
province_demographic_input_resolution_status_t::NONE;

size_t eligible_count = 0;

[[nodiscard]]
bool has_selection() const {
return
status
== province_demographic_input_resolution_status_t::SELECTED
&& selected != nullptr;
}
};

/*
 * Candidate precedence:
 *
 * 1. greater explicit authority_priority
 * 2. later reference_date
 * 3. later revision_date
 *
 * If candidates remain tied after those three explicit dimensions,
 * resolution is ambiguous. We do not use source kind, geography
 * kind, source identifier, pointer address, or input order as a
 * hidden tie-break.
 */
[[nodiscard]]
inline ProvinceDemographicAgeSexInputResolution
resolve_province_demographic_age_sex_input(
std::span<
ProvinceDemographicAgeSexInput const
> inputs,
Date target_date
) {
ProvinceDemographicAgeSexInputResolution result {};

ProvinceDemographicAgeSexInput const* best = nullptr;
bool ambiguous = false;

for (
ProvinceDemographicAgeSexInput const& candidate :
inputs
) {
if (!candidate.is_eligible_for(target_date)) {
continue;
}

++result.eligible_count;

if (best == nullptr) {
best = &candidate;
ambiguous = false;
continue;
}

if (
candidate.authority_priority
> best->authority_priority
) {
best = &candidate;
ambiguous = false;
continue;
}

if (
candidate.authority_priority
< best->authority_priority
) {
continue;
}

if (
candidate.reference_date
> best->reference_date
) {
best = &candidate;
ambiguous = false;
continue;
}

if (
candidate.reference_date
< best->reference_date
) {
continue;
}

if (
candidate.revision_date
> best->revision_date
) {
best = &candidate;
ambiguous = false;
continue;
}

if (
candidate.revision_date
< best->revision_date
) {
continue;
}

/*
 * Exact precedence tie.
 *
 * Deliberately refuse to choose based on source label,
 * input order, enum value, or address.
 */
ambiguous = true;
}

if (best == nullptr) {
result.status =
province_demographic_input_resolution_status_t::NONE;

return result;
}

if (ambiguous) {
result.status =
province_demographic_input_resolution_status_t::AMBIGUOUS;

return result;
}

result.selected = best;

result.status =
province_demographic_input_resolution_status_t::SELECTED;

return result;
}

enum class province_demographic_input_application_status_t : uint8_t {
NONE = 0,
AMBIGUOUS,
ALREADY_INITIALIZED,
MATERIALIZATION_FAILED,
INITIALIZED
};

struct ProvinceDemographicAgeSexInputApplicationResult {
province_demographic_input_application_status_t status =
province_demographic_input_application_status_t::NONE;

ProvinceDemographicAgeSexInput const* selected = nullptr;

size_t eligible_count = 0;

[[nodiscard]]
bool initialized() const {
return
status
== province_demographic_input_application_status_t::INITIALIZED;
}
};

/*
 * Explicit bridge:
 *
 * provenance-tagged candidate set
 *     -> deterministic resolution
 *     -> 004A9 authoritative province demographic state
 *
 * The selected input remains returned to the caller so its
 * provenance can be retained/exposed by the eventual data registry.
 */
[[nodiscard]]
inline ProvinceDemographicAgeSexInputApplicationResult
initialize_province_demographic_age_sex_state_from_inputs(
ProvinceDemographicAgeSexState& state,
pop_sum_t authoritative_population,
std::span<
ProvinceDemographicAgeSexInput const
> inputs,
Date target_date
) {
ProvinceDemographicAgeSexInputApplicationResult result {};

if (state.has_structure()) {
result.status =
province_demographic_input_application_status_t::
ALREADY_INITIALIZED;

return result;
}

const ProvinceDemographicAgeSexInputResolution resolution =
resolve_province_demographic_age_sex_input(
inputs,
target_date
);

result.eligible_count =
resolution.eligible_count;

if (
resolution.status
== province_demographic_input_resolution_status_t::NONE
) {
result.status =
province_demographic_input_application_status_t::NONE;

return result;
}

if (
resolution.status
== province_demographic_input_resolution_status_t::
AMBIGUOUS
) {
result.status =
province_demographic_input_application_status_t::
AMBIGUOUS;

return result;
}

result.selected =
resolution.selected;

if (
!state.initialize(
result.selected->profile,
authoritative_population
)
) {
result.status =
province_demographic_input_application_status_t::
MATERIALIZATION_FAILED;

return result;
}

result.status =
province_demographic_input_application_status_t::
INITIALIZED;

return result;
}
}
