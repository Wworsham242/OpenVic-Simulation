#pragma once

#include <array>
#include <cstdint>
#include <limits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/core/memory/SmartPtr.hpp"
#include "openvic-simulation/population/DemographicAgeSexProfile.hpp"
#include "openvic-simulation/population/PopSum.hpp"

namespace OpenVic {
struct ProvinceDemographicAgeSexInitializationResult {
PopulationAgeSexStructure structure {};
uint64_t profile_weight_sum = 0;
int64_t floor_assigned_population = 0;
int64_t remainder_assigned_population = 0;
bool cell_overflow = false;
bool valid = false;

bool operator==(
ProvinceDemographicAgeSexInitializationResult const&
) const = default;
};

struct ExactUnsignedMulDivResult {
uint64_t quotient = 0;
uint64_t remainder = 0;
bool valid = false;
};

/*
 * Computes:
 *
 *     floor(a * b / divisor)
 *     (a * b) % divisor
 *
 * without forming a*b directly.
 *
 * Preconditions:
 *
 *     divisor > 0
 *     b <= divisor
 *
 * The first quotient/remainder decomposition makes the large portion
 * safe. The remaining multiplication has lhs < divisor and a
 * 32-bit rhs, and is evaluated bit-by-bit while keeping every
 * intermediate remainder below divisor.
 */
[[nodiscard]] inline
ExactUnsignedMulDivResult exact_unsigned_mul_div(
uint64_t a,
uint32_t b,
uint64_t divisor
) {
ExactUnsignedMulDivResult result {};

if (
divisor == 0
|| static_cast<uint64_t>(b) > divisor
) {
return result;
}

const uint64_t whole =
a / divisor;

const uint64_t initial_remainder =
a % divisor;

/*
 * whole * b is safe because b <= divisor:
 *
 * whole * b <= whole * divisor <= a
 */
uint64_t quotient =
whole * static_cast<uint64_t>(b);

uint64_t fractional_quotient = 0;
uint64_t remainder = 0;

/*
 * Evaluate:
 *
 * initial_remainder * b
 *
 * one bit of b at a time.
 *
 * At every step remainder < divisor.
 * Since b is uint32_t, this requires exactly 32 iterations.
 */
for (int bit = 31; bit >= 0; --bit) {
fractional_quotient *= 2;

/*
 * 2 * remainder is safe here because profile weight totals
 * are bounded by 36 uint32_t cells, far below uint64_t.
 */
uint64_t expanded =
remainder * 2;

if (
(
static_cast<uint32_t>(b)
>> bit
) & 1U
) {
expanded += initial_remainder;
}

/*
 * expanded is less than 3 * divisor, so at most two
 * quotient units can be produced in one iteration.
 */
while (expanded >= divisor) {
expanded -= divisor;
++fractional_quotient;
}

remainder = expanded;
}

if (
fractional_quotient
> std::numeric_limits<uint64_t>::max()
- quotient
) {
return result;
}

result.quotient =
quotient + fractional_quotient;

result.remainder =
remainder;

result.valid = true;

return result;
}

/*
 * Reconcile an explicit relative age-sex profile against a
 * province's authoritative 64-bit population total.
 */
[[nodiscard]] inline
ProvinceDemographicAgeSexInitializationResult
materialize_province_demographic_age_sex_profile(
DemographicAgeSexProfile const& profile,
pop_sum_t population
) {
ProvinceDemographicAgeSexInitializationResult result {};

result.profile_weight_sum =
profile.get_total_weight();

const int64_t population_value =
type_safe::get(population);

if (population_value < 0) {
return result;
}

if (population_value == 0) {
result.valid = true;
return result;
}

if (result.profile_weight_sum == 0) {
return result;
}

std::array<
uint64_t,
DEMOGRAPHIC_AGE_SEX_CELL_COUNT
> remainders {};

int64_t assigned = 0;

constexpr int64_t max_cell_population =
std::numeric_limits<int32_t>::max();

for (
size_t cell_index = 0;
cell_index < DEMOGRAPHIC_AGE_SEX_CELL_COUNT;
++cell_index
) {
const uint32_t weight =
get_demographic_profile_cell_weight(
profile,
cell_index
);

const ExactUnsignedMulDivResult allocation =
exact_unsigned_mul_div(
static_cast<uint64_t>(
population_value
),
weight,
result.profile_weight_sum
);

if (!allocation.valid) {
return result;
}

if (
allocation.quotient
> static_cast<uint64_t>(
max_cell_population
)
) {
result.cell_overflow = true;
return result;
}

const int64_t floor_count =
static_cast<int64_t>(
allocation.quotient
);

remainders[cell_index] =
allocation.remainder;

set_demographic_structure_cell(
result.structure,
cell_index,
pop_size_t {
static_cast<int32_t>(
floor_count
)
}
);

assigned += floor_count;
}

result.floor_assigned_population =
assigned;

int64_t population_left =
population_value - assigned;

while (population_left > 0) {
size_t best_cell = 0;
uint64_t best_remainder = 0;
bool found = false;

for (
size_t cell_index = 0;
cell_index < DEMOGRAPHIC_AGE_SEX_CELL_COUNT;
++cell_index
) {
if (
!found
|| remainders[cell_index]
> best_remainder
) {
best_cell = cell_index;
best_remainder =
remainders[cell_index];
found = true;
}
}

if (!found || best_remainder == 0) {
return result;
}

const size_t age_index =
best_cell / 2;

const bool male =
(best_cell % 2) != 0;

AgeSexCohortCount& cohort =
result.structure.cohorts[
age_index
];

pop_size_t& count =
male
? cohort.male
: cohort.female;

const int64_t next_count =
static_cast<int64_t>(
type_safe::get(count)
) + 1;

if (next_count > max_cell_population) {
result.cell_overflow = true;
return result;
}

count =
pop_size_t {
static_cast<int32_t>(
next_count
)
};

remainders[best_cell] = 0;

--population_left;
++result.remainder_assigned_population;
}

result.valid =
result.structure.has_nonnegative_counts()
&& result.structure
.get_total_population()
== population_value;

return result;
}

struct ProvinceDemographicAgeSexState {
private:
memory::unique_ptr<
PopulationAgeSexStructure
> structure {};

public:
[[nodiscard]]
bool has_structure() const {
return structure != nullptr;
}

[[nodiscard]]
PopulationAgeSexStructure const*
get_structure_nullable() const {
return structure.get();
}

bool initialize(
DemographicAgeSexProfile const& profile,
pop_sum_t authoritative_population
) {
if (structure != nullptr) {
return false;
}

const auto result =
materialize_province_demographic_age_sex_profile(
profile,
authoritative_population
);

if (!result.valid) {
return false;
}

structure =
memory::make_unique<
PopulationAgeSexStructure
>(
result.structure
);

return true;
}

[[nodiscard]]
bool is_consistent_with_population(
pop_sum_t authoritative_population
) const {
return
structure != nullptr
&& structure
->has_nonnegative_counts()
&& structure
->get_total_population()
== type_safe::get(
authoritative_population
);
}
};
}
