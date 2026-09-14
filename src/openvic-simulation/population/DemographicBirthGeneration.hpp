#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006B1.3
 *
 * Deterministic birth generation from annual age-specific fertility rates.
 *
 * Rates are expressed as:
 *
 *     live births per 1,000 female person-years
 *
 * for the corresponding demographic age band.
 *
 * This mechanism deliberately does not:
 *
 * - use Victoria BASE_POPGROWTH;
 * - use province life rating;
 * - use generic population-growth modifiers;
 * - decide how socioeconomic or health conditions alter fertility;
 * - attach one simulation tick to one demographic year.
 *
 * A later calibrated producer may derive the effective ASFR schedule.
 * This substrate converts an explicit schedule into births.
 */
struct DemographicAgeSpecificFertilitySchedule final {
std::array<
uint32_t,
DEMOGRAPHIC_AGE_BAND_COUNT
> births_per_1000_female_years {};

[[nodiscard]] constexpr uint32_t const& get(
demographic_age_band_t age_band
) const {
return births_per_1000_female_years[
get_demographic_age_band_index(age_band)
];
}

[[nodiscard]] constexpr uint32_t& get(
demographic_age_band_t age_band
) {
return births_per_1000_female_years[
get_demographic_age_band_index(age_band)
];
}

bool operator==(
DemographicAgeSpecificFertilitySchedule const&
) const = default;
};

enum class demographic_birth_generation_status_t : uint8_t {
APPLIED,
INVALID_STARTING_STATE,
INVALID_FRACTIONAL_REMAINDER,
BIRTH_COUNT_OVERFLOW,
BIRTH_CELL_OVERFLOW
};

struct DemographicBirthGenerationResult final {
AgeSexCohortCount births {};

uint64_t total_births = 0;

/*
 * Cumulative fractional expected birth remainder with denominator 1,000.
 *
 * Returning this remainder allows a caller to preserve fractional
 * expectations across repeated demographic intervals rather than
 * systematically truncating small populations.
 */
uint16_t next_fractional_births_per_1000 = 0;

uint32_t male_births_per_1000_female_births = 0;

demographic_birth_generation_status_t status =
demographic_birth_generation_status_t::INVALID_STARTING_STATE;

[[nodiscard]] constexpr bool valid() const {
return
status
== demographic_birth_generation_status_t::APPLIED;
}

bool operator==(
DemographicBirthGenerationResult const&
) const = default;
};

[[nodiscard]] constexpr DemographicBirthGenerationResult
generate_demographic_births_from_annual_asfr(
PopulationAgeSexStructure const& population,
DemographicAgeSpecificFertilitySchedule const& fertility,
uint32_t male_births_per_1000_female_births,
uint16_t prior_fractional_births_per_1000 = 0
) {
DemographicBirthGenerationResult result {
.male_births_per_1000_female_births =
male_births_per_1000_female_births
};

if (!population.has_nonnegative_counts()) {
result.status =
demographic_birth_generation_status_t::
INVALID_STARTING_STATE;
return result;
}

if (prior_fractional_births_per_1000 >= 1000) {
result.status =
demographic_birth_generation_status_t::
INVALID_FRACTIONAL_REMAINDER;
return result;
}

/*
 * Each cell population is int32 and each rate is uint32, so one
 * cell's product fits in uint64.
 *
 * We divide each product before accumulation and carry only the
 * sub-1,000 remainder. This avoids requiring a wide aggregate
 * multiplication and produces the exact floor of the summed
 * rational expectation.
 */
uint64_t whole_births = 0;
uint64_t fractional = prior_fractional_births_per_1000;

constexpr uint64_t max_total_births =
static_cast<uint64_t>(
std::numeric_limits<int32_t>::max()
) * 2ULL;

for (size_t age_index = 0;
age_index < DEMOGRAPHIC_AGE_BAND_COUNT;
++age_index
) {
const int64_t female_population =
type_safe::get(
population.cohorts[age_index].female
);

const uint64_t rate =
fertility.births_per_1000_female_years[
age_index
];

const uint64_t numerator =
static_cast<uint64_t>(female_population)
* rate;

whole_births += numerator / 1000ULL;
fractional += numerator % 1000ULL;

whole_births += fractional / 1000ULL;
fractional %= 1000ULL;

if (whole_births > max_total_births) {
result.status =
demographic_birth_generation_status_t::
BIRTH_COUNT_OVERFLOW;
return result;
}
}

result.total_births = whole_births;
result.next_fractional_births_per_1000 =
static_cast<uint16_t>(fractional);

if (whole_births == 0) {
result.status =
demographic_birth_generation_status_t::APPLIED;
return result;
}

/*
 * Sex ratio at birth is represented as:
 *
 *     male births per 1,000 female births
 *
 * Therefore:
 *
 *     male share = ratio / (ratio + 1000)
 *
 * Round the male allocation to nearest integer, with exact total births
 * preserved by assigning the remainder to female births.
 */
const uint64_t sex_ratio =
male_births_per_1000_female_births;

const uint64_t sex_denominator =
sex_ratio + 1000ULL;

const uint64_t male_numerator =
whole_births * sex_ratio;

uint64_t male_births =
male_numerator / sex_denominator;

const uint64_t male_remainder =
male_numerator % sex_denominator;

if (
male_remainder * 2ULL
>= sex_denominator
) {
++male_births;
}

const uint64_t female_births =
whole_births - male_births;

constexpr uint64_t max_cell =
static_cast<uint64_t>(
std::numeric_limits<int32_t>::max()
);

if (
female_births > max_cell
|| male_births > max_cell
) {
result.status =
demographic_birth_generation_status_t::
BIRTH_CELL_OVERFLOW;
return result;
}

result.births.female =
pop_size_t {
static_cast<int32_t>(female_births)
};

result.births.male =
pop_size_t {
static_cast<int32_t>(male_births)
};

result.status =
demographic_birth_generation_status_t::APPLIED;

return result;
}

}