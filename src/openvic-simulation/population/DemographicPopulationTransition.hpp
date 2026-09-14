#pragma once

#include <cstdint>
#include <limits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006B1.1
 *
 * Pure demographic transition accounting.
 *
 * This substrate does not mutate Pop::size, ProvinceInstance aggregates,
 * employment, culture, religion, occupation, ideology, or any other
 * socioeconomic representation.
 *
 * Component order is explicit:
 *
 *     starting stock
 *       -> deaths
 *       -> births
 *       -> immigration
 *       -> emigration
 *       -> ending stock
 *
 * Cohort aging is deliberately not part of this increment.
 */
struct DemographicPopulationComponents final {
AgeSexCohortCount births {};
PopulationAgeSexStructure deaths {};
PopulationAgeSexStructure immigration {};
PopulationAgeSexStructure emigration {};

[[nodiscard]] constexpr bool has_nonnegative_counts() const {
return
type_safe::get(births.female) >= 0
&& type_safe::get(births.male) >= 0
&& deaths.has_nonnegative_counts()
&& immigration.has_nonnegative_counts()
&& emigration.has_nonnegative_counts();
}

[[nodiscard]] constexpr int64_t get_births() const {
return
static_cast<int64_t>(type_safe::get(births.female))
+ static_cast<int64_t>(type_safe::get(births.male));
}

[[nodiscard]] constexpr int64_t get_deaths() const {
return deaths.get_total_population();
}

[[nodiscard]] constexpr int64_t get_immigration() const {
return immigration.get_total_population();
}

[[nodiscard]] constexpr int64_t get_emigration() const {
return emigration.get_total_population();
}

bool operator==(DemographicPopulationComponents const&) const = default;
};

enum class demographic_population_transition_status_t : uint8_t {
APPLIED,
INVALID_STARTING_STATE,
INVALID_COMPONENT_STATE,
DEATHS_EXCEED_AVAILABLE,
EMIGRATION_EXCEEDS_AVAILABLE,
CELL_OVERFLOW,
ACCOUNTING_MISMATCH
};

struct DemographicPopulationTransitionResult final {
PopulationAgeSexStructure starting {};
PopulationAgeSexStructure ending {};
DemographicPopulationComponents components {};

int64_t starting_population = 0;
int64_t births = 0;
int64_t deaths = 0;
int64_t immigration = 0;
int64_t emigration = 0;
int64_t ending_population = 0;
int64_t net_population_change = 0;

demographic_population_transition_status_t status =
demographic_population_transition_status_t::INVALID_STARTING_STATE;

[[nodiscard]] constexpr bool valid() const {
return status == demographic_population_transition_status_t::APPLIED;
}

bool operator==(DemographicPopulationTransitionResult const&) const = default;
};

[[nodiscard]] constexpr DemographicPopulationTransitionResult
apply_demographic_population_components(
PopulationAgeSexStructure const& starting,
DemographicPopulationComponents const& components
) {
DemographicPopulationTransitionResult result {
.starting = starting,
.ending = starting,
.components = components,
.starting_population = starting.get_total_population(),
.births = components.get_births(),
.deaths = components.get_deaths(),
.immigration = components.get_immigration(),
.emigration = components.get_emigration()
};

if (!starting.has_nonnegative_counts()) {
result.status =
demographic_population_transition_status_t::INVALID_STARTING_STATE;
return result;
}

if (!components.has_nonnegative_counts()) {
result.status =
demographic_population_transition_status_t::INVALID_COMPONENT_STATE;
return result;
}

constexpr int64_t max_cell =
std::numeric_limits<int32_t>::max();

for (size_t age_index = 0;
age_index < DEMOGRAPHIC_AGE_BAND_COUNT;
++age_index
) {
auto apply_cell = [&](
pop_size_t starting_count,
pop_size_t death_count,
pop_size_t birth_count,
pop_size_t immigration_count,
pop_size_t emigration_count,
pop_size_t& ending_count
) -> bool {
const int64_t start =
type_safe::get(starting_count);

const int64_t deaths =
type_safe::get(death_count);

if (deaths > start) {
result.status =
demographic_population_transition_status_t::
DEATHS_EXCEED_AVAILABLE;
return false;
}

int64_t value = start - deaths;

const int64_t births =
type_safe::get(birth_count);

if (births > max_cell - value) {
result.status =
demographic_population_transition_status_t::
CELL_OVERFLOW;
return false;
}

value += births;

const int64_t immigration =
type_safe::get(immigration_count);

if (immigration > max_cell - value) {
result.status =
demographic_population_transition_status_t::
CELL_OVERFLOW;
return false;
}

value += immigration;

const int64_t emigration =
type_safe::get(emigration_count);

if (emigration > value) {
result.status =
demographic_population_transition_status_t::
EMIGRATION_EXCEEDS_AVAILABLE;
return false;
}

value -= emigration;

ending_count =
pop_size_t { static_cast<int32_t>(value) };

return true;
};

AgeSexCohortCount const& start =
starting.cohorts[age_index];

AgeSexCohortCount const& deaths =
components.deaths.cohorts[age_index];

AgeSexCohortCount const& immigration =
components.immigration.cohorts[age_index];

AgeSexCohortCount const& emigration =
components.emigration.cohorts[age_index];

AgeSexCohortCount births {};

if (
age_index
== get_demographic_age_band_index(
demographic_age_band_t::AGE_0_4
)
) {
births = components.births;
}

AgeSexCohortCount& ending =
result.ending.cohorts[age_index];

if (
!apply_cell(
start.female,
deaths.female,
births.female,
immigration.female,
emigration.female,
ending.female
)
|| !apply_cell(
start.male,
deaths.male,
births.male,
immigration.male,
emigration.male,
ending.male
)
) {
/*
 * Transactional failure: callers must never observe a
 * partially transitioned demographic stock as an applied
 * result.
 */
result.ending = starting;
return result;
}
}

result.ending_population =
result.ending.get_total_population();

const int64_t expected_ending =
result.starting_population
+ result.births
- result.deaths
+ result.immigration
- result.emigration;

if (
!result.ending.has_nonnegative_counts()
|| result.ending_population != expected_ending
) {
result.ending = starting;
result.ending_population =
result.starting_population;
result.net_population_change = 0;
result.status =
demographic_population_transition_status_t::ACCOUNTING_MISMATCH;
return result;
}

result.net_population_change =
result.ending_population
- result.starting_population;

result.status =
demographic_population_transition_status_t::APPLIED;

return result;
}

}