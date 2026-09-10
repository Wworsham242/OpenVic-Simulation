#pragma once

#include <cstddef>
#include <span>
#include <utility>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/population/ProvinceDemographicAgeSexInput.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {
struct ProvinceDemographicAgeSexImportRecord {
province_index_t province_index {};
ProvinceDemographicAgeSexInput input {};
};

enum class province_demographic_registry_add_status_t : uint8_t {
ADDED = 0,
INVALID_PROVINCE,
LOCKED
};

/*
 * Load-time registry for province demographic input candidates.
 *
 * The registry has two phases:
 *
 *   mutable import phase
 *       -> lock()
 *       -> immutable resolution/application phase
 *
 * Locking is required before candidate spans are exposed for
 * authoritative resolution. This protects the lifetime of pointers
 * returned by the 004A10 resolver.
 */
struct ProvinceDemographicAgeSexRegistry {
private:
memory::vector<
memory::vector<
ProvinceDemographicAgeSexInput
>
> candidates_by_province {};

bool locked = false;

[[nodiscard]]
bool is_valid_province_index(
province_index_t province_index
) const {
return
static_cast<size_t>(
type_safe::get(province_index)
)
< candidates_by_province.size();
}

public:
explicit ProvinceDemographicAgeSexRegistry(
size_t province_count
) : candidates_by_province {
province_count
} {}

[[nodiscard]]
size_t get_province_capacity() const {
return candidates_by_province.size();
}

[[nodiscard]]
bool is_locked() const {
return locked;
}

void lock() {
locked = true;
}

[[nodiscard]]
province_demographic_registry_add_status_t
add_input(
province_index_t province_index,
ProvinceDemographicAgeSexInput input
) {
if (locked) {
return
province_demographic_registry_add_status_t::
LOCKED;
}

if (!is_valid_province_index(province_index)) {
return
province_demographic_registry_add_status_t::
INVALID_PROVINCE;
}

candidates_by_province[
type_safe::get(province_index)
].push_back(
std::move(input)
);

return
province_demographic_registry_add_status_t::
ADDED;
}

[[nodiscard]]
province_demographic_registry_add_status_t
add_import_record(
ProvinceDemographicAgeSexImportRecord record
) {
return add_input(
record.province_index,
std::move(record.input)
);
}

[[nodiscard]]
size_t get_candidate_count(
province_index_t province_index
) const {
if (!is_valid_province_index(province_index)) {
return 0;
}

return candidates_by_province[
type_safe::get(province_index)
].size();
}

/*
 * Candidate spans are exposed only once loading is complete.
 *
 * Before lock(), returning an empty span prevents a caller from
 * resolving against storage that may later reallocate.
 */
[[nodiscard]]
std::span<
ProvinceDemographicAgeSexInput const
>
get_inputs(
province_index_t province_index
) const {
if (
!locked
|| !is_valid_province_index(
province_index
)
) {
return {};
}

auto const& inputs =
candidates_by_province[
type_safe::get(
province_index
)
];

return {
inputs.data(),
inputs.size()
};
}

[[nodiscard]]
ProvinceDemographicAgeSexInputResolution
resolve(
province_index_t province_index,
Date target_date
) const {
if (!locked) {
return {};
}

return
resolve_province_demographic_age_sex_input(
get_inputs(province_index),
target_date
);
}

[[nodiscard]]
ProvinceDemographicAgeSexInputApplicationResult
initialize_state(
province_index_t province_index,
ProvinceDemographicAgeSexState& state,
pop_sum_t authoritative_population,
Date target_date
) const {
if (!locked) {
return {};
}

return
initialize_province_demographic_age_sex_state_from_inputs(
state,
authoritative_population,
get_inputs(province_index),
target_date
);
}
};
}
