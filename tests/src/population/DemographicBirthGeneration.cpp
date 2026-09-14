#include "openvic-simulation/population/DemographicBirthGeneration.hpp"
#include "openvic-simulation/population/DemographicPopulationTransition.hpp"

#include <cstdint>
#include <limits>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
"006B1.3 zero fertility rates generate no births",
"[convergence][006b1][006b1.3][population][demography][fertility]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 1000 },
pop_size_t { 1000 }
};

DemographicAgeSpecificFertilitySchedule fertility {};

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

REQUIRE(result.valid());

CHECK(result.total_births == 0);
CHECK(type_safe::get(result.births.female) == 0);
CHECK(type_safe::get(result.births.male) == 0);
CHECK(
result.next_fractional_births_per_1000
== 0
);
}

TEST_CASE(
"006B1.3 one age-specific fertility rate produces expected births",
"[convergence][006b1][006b1.3][population][demography][fertility]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1000 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 100;

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1000
);

REQUIRE(result.valid());

CHECK(result.total_births == 100);
CHECK(
type_safe::get(result.births.female)
+ type_safe::get(result.births.male)
== 100
);
}

TEST_CASE(
"006B1.3 male population does not contribute to fertility exposure",
"[convergence][006b1][006b1.3][population][demography][fertility]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).male = pop_size_t { 100000 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 500;

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

REQUIRE(result.valid());
CHECK(result.total_births == 0);
}

TEST_CASE(
"006B1.3 fertility contributions sum across age bands",
"[convergence][006b1][006b1.3][population][demography][fertility]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 500 };

population.get(
demographic_age_band_t::AGE_25_29
).female = pop_size_t { 1000 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 100;

fertility.get(
demographic_age_band_t::AGE_25_29
) = 50;

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

REQUIRE(result.valid());
CHECK(result.total_births == 100);
}

TEST_CASE(
"006B1.3 fractional expected births carry deterministically across intervals",
"[convergence][006b1][006b1.3][population][demography][fertility][remainder]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 500;

auto const first =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1000
);

REQUIRE(first.valid());
CHECK(first.total_births == 0);
CHECK(
first.next_fractional_births_per_1000
== 500
);

auto const second =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1000,
first.next_fractional_births_per_1000
);

REQUIRE(second.valid());
CHECK(second.total_births == 1);
CHECK(
second.next_fractional_births_per_1000
== 0
);
}

TEST_CASE(
"006B1.3 explicit sex ratio divides total births deterministically",
"[convergence][006b1][006b1.3][population][demography][fertility][sex-ratio]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 2050 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 1000;

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

REQUIRE(result.valid());

CHECK(result.total_births == 2050);
CHECK(type_safe::get(result.births.female) == 1000);
CHECK(type_safe::get(result.births.male) == 1050);
}

TEST_CASE(
"006B1.3 engine does not hard code a specific reproductive age window",
"[convergence][006b1][006b1.3][population][demography][fertility][general]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_50_54
).female = pop_size_t { 1000 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_50_54
) = 10;

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

REQUIRE(result.valid());
CHECK(result.total_births == 10);
}

TEST_CASE(
"006B1.3 rejects negative starting demographic state",
"[convergence][006b1][006b1.3][population][demography][fertility][validation]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { -1 };

DemographicAgeSpecificFertilitySchedule fertility {};

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_birth_generation_status_t::
INVALID_STARTING_STATE
);
}

TEST_CASE(
"006B1.3 rejects invalid fractional carry",
"[convergence][006b1][006b1.3][population][demography][fertility][validation]"
) {
PopulationAgeSexStructure population {};
DemographicAgeSpecificFertilitySchedule fertility {};

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050,
1000
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_birth_generation_status_t::
INVALID_FRACTIONAL_REMAINDER
);
}

TEST_CASE(
"006B1.3 rejects birth totals beyond demographic cell capacity",
"[convergence][006b1][006b1.3][population][demography][fertility][overflow]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t {
std::numeric_limits<int32_t>::max()
};

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = std::numeric_limits<uint32_t>::max();

auto const result =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_birth_generation_status_t::
BIRTH_COUNT_OVERFLOW
);
}

TEST_CASE(
"006B1.3 generated births feed the 006B1.1 transition contract",
"[convergence][006b1][006b1.3][population][demography][fertility][integration]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1000 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 100;

auto const generated =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1000
);

REQUIRE(generated.valid());

DemographicPopulationComponents components {};
components.births = generated.births;

auto const transitioned =
apply_demographic_population_components(
population,
components
);

REQUIRE(transitioned.valid());

CHECK(
transitioned.ending_population
== population.get_total_population()
+ 100
);

CHECK(
type_safe::get(
transitioned.ending.get(
demographic_age_band_t::AGE_0_4
).female
)
== type_safe::get(
generated.births.female
)
);

CHECK(
type_safe::get(
transitioned.ending.get(
demographic_age_band_t::AGE_0_4
).male
)
== type_safe::get(
generated.births.male
)
);
}

TEST_CASE(
"006B1.3 identical fertility inputs produce identical results",
"[convergence][006b1][006b1.3][population][demography][fertility][determinism]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1234 };

population.get(
demographic_age_band_t::AGE_30_34
).female = pop_size_t { 987 };

DemographicAgeSpecificFertilitySchedule fertility {};

fertility.get(
demographic_age_band_t::AGE_20_24
) = 73;

fertility.get(
demographic_age_band_t::AGE_30_34
) = 121;

auto const first =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050,
317
);

auto const second =
generate_demographic_births_from_annual_asfr(
population,
fertility,
1050,
317
);

REQUIRE(first.valid());
REQUIRE(second.valid());

CHECK(first == second);
}