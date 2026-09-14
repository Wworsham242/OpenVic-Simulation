#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006B1.4
 *
 * Deterministic age/sex-specific baseline mortality generation.
 *
 * Mortality inputs are annual probabilities of death expressed per
 * 1,000,000 persons for each age/sex cell.
 *
 * This mechanism deliberately does not convert nutrition, disease,
 * conflict, temperature, healthcare access, or other exposures directly
 * into deaths. Those mechanisms may later produce or modify an effective
 * mortality schedule.
 */
inline constexpr uint32_t
DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR = 1'000'000;

struct AgeSexMortalityProbability final {
uint32_t female_per_million = 0;
uint32_t male_per_million = 0;

bool operator==(
AgeSexMortalityProbability const&
) const = default;
};

struct DemographicAgeSexMortalitySchedule final {
std::array<
AgeSexMortalityProbability,
DEMOGRAPHIC_AGE_BAND_COUNT
> probabilities {};

[[nodiscard]] constexpr
AgeSexMortalityProbability const& get(
demographic_age_band_t age_band
) const {
return probabilities[
get_demographic_age_band_index(age_band)
];
}

[[nodiscard]] constexpr
AgeSexMortalityProbability& get(
demographic_age_band_t age_band
) {
return probabilities[
get_demographic_age_band_index(age_band)
];
}

[[nodiscard]] constexpr bool valid() const {
for (auto const& probability : probabilities) {
if (
probability.female_per_million
> DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR
|| probability.male_per_million
> DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR
) {
return false;
}
}

return true;
}

bool operator==(
DemographicAgeSexMortalitySchedule const&
) const = default;
};

struct AgeSexMortalityRemainder final {
uint32_t female = 0;
uint32_t male = 0;

bool operator==(
AgeSexMortalityRemainder const&
) const = default;
};

struct DemographicMortalityRemainders final {
std::array<
AgeSexMortalityRemainder,
DEMOGRAPHIC_AGE_BAND_COUNT
> cells {};

[[nodiscard]] constexpr
AgeSexMortalityRemainder const& get(
demographic_age_band_t age_band
) const {
return cells[
get_demographic_age_band_index(age_band)
];
}

[[nodiscard]] constexpr
AgeSexMortalityRemainder& get(
demographic_age_band_t age_band
) {
return cells[
get_demographic_age_band_index(age_band)
];
}

[[nodiscard]] constexpr bool valid() const {
for (auto const& remainder : cells) {
if (
remainder.female
>= DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR
|| remainder.male
>= DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR
) {
return false;
}
}

return true;
}

bool operator==(
DemographicMortalityRemainders const&
) const = default;
};

enum class demographic_death_generation_status_t : uint8_t {
APPLIED,
INVALID_STARTING_STATE,
INVALID_MORTALITY_SCHEDULE,
INVALID_FRACTIONAL_REMAINDER,
ACCOUNTING_MISMATCH
};

struct DemographicDeathGenerationResult final {
PopulationAgeSexStructure deaths {};
DemographicMortalityRemainders next_remainders {};

int64_t total_deaths = 0;

demographic_death_generation_status_t status =
demographic_death_generation_status_t::
INVALID_STARTING_STATE;

[[nodiscard]] constexpr bool valid() const {
return
status
== demographic_death_generation_status_t::APPLIED;
}

bool operator==(
DemographicDeathGenerationResult const&
) const = default;
};

[[nodiscard]] constexpr DemographicDeathGenerationResult
generate_demographic_deaths_from_annual_mortality(
PopulationAgeSexStructure const& population,
DemographicAgeSexMortalitySchedule const& mortality,
DemographicMortalityRemainders const& prior_remainders = {}
) {
DemographicDeathGenerationResult result {};

if (!population.has_nonnegative_counts()) {
result.status =
demographic_death_generation_status_t::
INVALID_STARTING_STATE;
return result;
}

if (!mortality.valid()) {
result.status =
demographic_death_generation_status_t::
INVALID_MORTALITY_SCHEDULE;
return result;
}

if (!prior_remainders.valid()) {
result.status =
demographic_death_generation_status_t::
INVALID_FRACTIONAL_REMAINDER;
return result;
}

constexpr uint64_t denominator =
DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR;

int64_t total_deaths = 0;

for (size_t age_index = 0;
age_index < DEMOGRAPHIC_AGE_BAND_COUNT;
++age_index
) {
AgeSexCohortCount const& cohort =
population.cohorts[age_index];

AgeSexMortalityProbability const& probability =
mortality.probabilities[age_index];

AgeSexMortalityRemainder const& prior =
prior_remainders.cells[age_index];

AgeSexCohortCount& deaths =
result.deaths.cohorts[age_index];

AgeSexMortalityRemainder& next =
result.next_remainders.cells[age_index];

auto generate_cell = [](
pop_size_t population_count,
uint32_t probability_per_million,
uint32_t prior_remainder,
pop_size_t& death_count,
uint32_t& next_remainder
) {
const uint64_t population_value =
static_cast<uint64_t>(
type_safe::get(population_count)
);

const uint64_t numerator =
population_value
* probability_per_million
+ prior_remainder;

const uint64_t deaths_value =
numerator
/ DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR;

next_remainder =
static_cast<uint32_t>(
numerator
% DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR
);

death_count = pop_size_t {
static_cast<int32_t>(deaths_value)
};
};

generate_cell(
cohort.female,
probability.female_per_million,
prior.female,
deaths.female,
next.female
);

generate_cell(
cohort.male,
probability.male_per_million,
prior.male,
deaths.male,
next.male
);

if (
type_safe::get(deaths.female)
> type_safe::get(cohort.female)
|| type_safe::get(deaths.male)
> type_safe::get(cohort.male)
) {
result = {};
result.status =
demographic_death_generation_status_t::
ACCOUNTING_MISMATCH;
return result;
}

total_deaths +=
type_safe::get(deaths.female);

total_deaths +=
type_safe::get(deaths.male);
}

result.total_deaths = total_deaths;

if (
result.deaths.get_total_population()
!= total_deaths
) {
result = {};
result.status =
demographic_death_generation_status_t::
ACCOUNTING_MISMATCH;
return result;
}

result.status =
demographic_death_generation_status_t::APPLIED;

return result;
}

}