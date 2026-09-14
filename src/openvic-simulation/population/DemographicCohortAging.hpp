#pragma once

#include <cstdint>
#include <limits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006B1.2
 *
 * Explicit cohort-boundary aging accounting.
 *
 * PopulationAgeSexStructure stores five-year age bands but does not encode
 * exact within-band ages. Therefore this mechanism does not infer aging
 * outflow as one-fifth of each cohort or attach demographic years to engine
 * ticks.
 *
 * The caller supplies the number of female/male people crossing each closed
 * age-band boundary during the owning demographic interval.
 *
 * outflow[AGE_0_4]   moves AGE_0_4   -> AGE_5_9
 * ...
 * outflow[AGE_80_84] moves AGE_80_84 -> AGE_85_PLUS
 *
 * AGE_85_PLUS is terminal/open-ended and has no aging outflow.
 */
struct DemographicCohortAgingFlow final {
PopulationAgeSexStructure outflow {};

[[nodiscard]] constexpr bool has_nonnegative_counts() const {
return outflow.has_nonnegative_counts();
}

bool operator==(DemographicCohortAgingFlow const&) const = default;
};

enum class demographic_cohort_aging_status_t : uint8_t {
APPLIED,
INVALID_STARTING_STATE,
INVALID_FLOW_STATE,
TERMINAL_OUTFLOW_NOT_ALLOWED,
OUTFLOW_EXCEEDS_AVAILABLE,
CELL_OVERFLOW,
ACCOUNTING_MISMATCH
};

struct DemographicCohortAgingResult final {
PopulationAgeSexStructure starting {};
PopulationAgeSexStructure ending {};
DemographicCohortAgingFlow flow {};

int64_t starting_population = 0;
int64_t ending_population = 0;
int64_t female_aged = 0;
int64_t male_aged = 0;

demographic_cohort_aging_status_t status =
demographic_cohort_aging_status_t::INVALID_STARTING_STATE;

[[nodiscard]] constexpr bool valid() const {
return status == demographic_cohort_aging_status_t::APPLIED;
}

[[nodiscard]] constexpr int64_t get_total_aged() const {
return female_aged + male_aged;
}

bool operator==(DemographicCohortAgingResult const&) const = default;
};

[[nodiscard]] constexpr DemographicCohortAgingResult
apply_demographic_cohort_aging(
PopulationAgeSexStructure const& starting,
DemographicCohortAgingFlow const& flow
) {
DemographicCohortAgingResult result {
.starting = starting,
.ending = starting,
.flow = flow,
.starting_population = starting.get_total_population()
};

if (!starting.has_nonnegative_counts()) {
result.status =
demographic_cohort_aging_status_t::INVALID_STARTING_STATE;
return result;
}

if (!flow.has_nonnegative_counts()) {
result.status =
demographic_cohort_aging_status_t::INVALID_FLOW_STATE;
return result;
}

constexpr size_t terminal_index =
get_demographic_age_band_index(
demographic_age_band_t::AGE_85_PLUS
);

AgeSexCohortCount const& terminal_outflow =
flow.outflow.cohorts[terminal_index];

if (
type_safe::get(terminal_outflow.female) != 0
|| type_safe::get(terminal_outflow.male) != 0
) {
result.status =
demographic_cohort_aging_status_t::
TERMINAL_OUTFLOW_NOT_ALLOWED;
return result;
}

/*
 * Validate all source outflows before mutating the result so failure is
 * transactional and cannot expose a partially aged structure.
 */
for (size_t source_index = 0;
source_index < terminal_index;
++source_index
) {
AgeSexCohortCount const& available =
starting.cohorts[source_index];

AgeSexCohortCount const& aging =
flow.outflow.cohorts[source_index];

if (
type_safe::get(aging.female)
> type_safe::get(available.female)
|| type_safe::get(aging.male)
> type_safe::get(available.male)
) {
result.status =
demographic_cohort_aging_status_t::
OUTFLOW_EXCEEDS_AVAILABLE;
return result;
}
}

/*
 * Calculate every destination in 64-bit intermediates from the original
 * starting stock:
 *
 * ending[i] =
 *     starting[i]
 *     - outflow[i]
 *     + outflow[i - 1]
 *
 * AGE_0_4 has no incoming aging flow.
 * AGE_85_PLUS receives AGE_80_84 outflow and has no terminal outflow.
 */
constexpr int64_t max_cell =
std::numeric_limits<int32_t>::max();

for (size_t destination_index = 0;
destination_index <= terminal_index;
++destination_index
) {
AgeSexCohortCount const& start =
starting.cohorts[destination_index];

AgeSexCohortCount const outgoing =
destination_index < terminal_index
? flow.outflow.cohorts[destination_index]
: AgeSexCohortCount {};

AgeSexCohortCount const incoming =
destination_index > 0
? flow.outflow.cohorts[destination_index - 1]
: AgeSexCohortCount {};

auto compute_cell = [&](
pop_size_t start_count,
pop_size_t outgoing_count,
pop_size_t incoming_count,
pop_size_t& ending_count
) -> bool {
int64_t const start_value =
type_safe::get(start_count);

int64_t const outgoing_value =
type_safe::get(outgoing_count);

int64_t const incoming_value =
type_safe::get(incoming_count);

int64_t const remaining =
start_value - outgoing_value;

if (
incoming_value
> max_cell - remaining
) {
result.status =
demographic_cohort_aging_status_t::
CELL_OVERFLOW;
return false;
}

ending_count = pop_size_t {
static_cast<int32_t>(
remaining + incoming_value
)
};

return true;
};

AgeSexCohortCount& ending =
result.ending.cohorts[destination_index];

if (
!compute_cell(
start.female,
outgoing.female,
incoming.female,
ending.female
)
|| !compute_cell(
start.male,
outgoing.male,
incoming.male,
ending.male
)
) {
result.ending = starting;
return result;
}
}

for (size_t source_index = 0;
source_index < terminal_index;
++source_index
) {
result.female_aged +=
type_safe::get(
flow.outflow.cohorts[source_index].female
);

result.male_aged +=
type_safe::get(
flow.outflow.cohorts[source_index].male
);
}

result.ending_population =
result.ending.get_total_population();

if (
!result.ending.has_nonnegative_counts()
|| result.ending_population
!= result.starting_population
|| result.ending.get_female_population()
!= starting.get_female_population()
|| result.ending.get_male_population()
!= starting.get_male_population()
) {
result.ending = starting;
result.ending_population =
result.starting_population;
result.female_aged = 0;
result.male_aged = 0;
result.status =
demographic_cohort_aging_status_t::
ACCOUNTING_MISMATCH;
return result;
}

result.status =
demographic_cohort_aging_status_t::APPLIED;

return result;
}

}