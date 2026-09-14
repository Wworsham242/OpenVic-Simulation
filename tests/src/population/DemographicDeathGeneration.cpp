#include "openvic-simulation/population/DemographicDeathGeneration.hpp"
#include "openvic-simulation/population/DemographicPopulationTransition.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
"006B1.4 zero mortality produces zero deaths",
"[convergence][006b1][006b1.4][population][demography][mortality]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 1000 },
pop_size_t { 1000 }
};

DemographicAgeSexMortalitySchedule mortality {};

auto const result =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

REQUIRE(result.valid());
CHECK(result.total_deaths == 0);
CHECK(result.deaths.get_total_population() == 0);
}

TEST_CASE(
"006B1.4 mortality is age and sex specific",
"[convergence][006b1][006b1.4][population][demography][mortality]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 1000 },
pop_size_t { 1000 }
};

population.get(
demographic_age_band_t::AGE_80_84
) = {
pop_size_t { 1000 },
pop_size_t { 1000 }
};

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_20_24
) = {
1000,
2000
};

mortality.get(
demographic_age_band_t::AGE_80_84
) = {
100000,
200000
};

auto const result =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.deaths.get(
demographic_age_band_t::AGE_20_24
).female
) == 1
);

CHECK(
type_safe::get(
result.deaths.get(
demographic_age_band_t::AGE_20_24
).male
) == 2
);

CHECK(
type_safe::get(
result.deaths.get(
demographic_age_band_t::AGE_80_84
).female
) == 100
);

CHECK(
type_safe::get(
result.deaths.get(
demographic_age_band_t::AGE_80_84
).male
) == 200
);

CHECK(result.total_deaths == 303);
}

TEST_CASE(
"006B1.4 fractional mortality remainder stays with its age sex cell",
"[convergence][006b1][006b1.4][population][demography][mortality][remainder]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1 };

population.get(
demographic_age_band_t::AGE_70_74
).male = pop_size_t { 1 };

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_20_24
).female_per_million = 500000;

auto const first =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

REQUIRE(first.valid());
CHECK(first.total_deaths == 0);

CHECK(
first.next_remainders.get(
demographic_age_band_t::AGE_20_24
).female == 500000
);

CHECK(
first.next_remainders.get(
demographic_age_band_t::AGE_70_74
).male == 0
);

auto const second =
generate_demographic_deaths_from_annual_mortality(
population,
mortality,
first.next_remainders
);

REQUIRE(second.valid());

CHECK(
type_safe::get(
second.deaths.get(
demographic_age_band_t::AGE_20_24
).female
) == 1
);

CHECK(
type_safe::get(
second.deaths.get(
demographic_age_band_t::AGE_70_74
).male
) == 0
);
}

TEST_CASE(
"006B1.4 certain annual mortality cannot kill more than the cohort",
"[convergence][006b1][006b1.4][population][demography][mortality][bounds]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_85_PLUS
) = {
pop_size_t { 17 },
pop_size_t { 13 }
};

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_85_PLUS
) = {
DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR,
DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR
};

auto const result =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

REQUIRE(result.valid());

CHECK(result.total_deaths == 30);
CHECK(
result.deaths.get(
demographic_age_band_t::AGE_85_PLUS
)
== population.get(
demographic_age_band_t::AGE_85_PLUS
)
);
}

TEST_CASE(
"006B1.4 rejects mortality probability greater than one",
"[convergence][006b1][006b1.4][population][demography][mortality][validation]"
) {
PopulationAgeSexStructure population {};

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_20_24
).female_per_million =
DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR + 1;

auto const result =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_death_generation_status_t::
INVALID_MORTALITY_SCHEDULE
);
}

TEST_CASE(
"006B1.4 rejects invalid cell fractional remainder",
"[convergence][006b1][006b1.4][population][demography][mortality][validation]"
) {
PopulationAgeSexStructure population {};

DemographicAgeSexMortalitySchedule mortality {};
DemographicMortalityRemainders remainders {};

remainders.get(
demographic_age_band_t::AGE_20_24
).female =
DEMOGRAPHIC_MORTALITY_PROBABILITY_DENOMINATOR;

auto const result =
generate_demographic_deaths_from_annual_mortality(
population,
mortality,
remainders
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_death_generation_status_t::
INVALID_FRACTIONAL_REMAINDER
);
}

TEST_CASE(
"006B1.4 generated deaths feed the 006B1.1 transition contract",
"[convergence][006b1][006b1.4][population][demography][mortality][integration]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_70_74
) = {
pop_size_t { 100 },
pop_size_t { 100 }
};

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_70_74
) = {
100000,
200000
};

auto const generated =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

REQUIRE(generated.valid());
CHECK(generated.total_deaths == 30);

DemographicPopulationComponents components {};
components.deaths = generated.deaths;

auto const transitioned =
apply_demographic_population_components(
population,
components
);

REQUIRE(transitioned.valid());

CHECK(
transitioned.ending_population
== population.get_total_population() - 30
);
}

TEST_CASE(
"006B1.4 mortality generator does not consume nutrition health burden directly",
"[convergence][006b1][006b1.4][population][demography][mortality][boundary]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_40_44
).female = pop_size_t { 1000 };

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_40_44
).female_per_million = 10000;

auto const result =
generate_demographic_deaths_from_annual_mortality(
population,
mortality
);

REQUIRE(result.valid());
CHECK(result.total_deaths == 10);
}

TEST_CASE(
"006B1.4 identical mortality inputs produce identical results",
"[convergence][006b1][006b1.4][population][demography][mortality][determinism]"
) {
PopulationAgeSexStructure population {};

population.get(
demographic_age_band_t::AGE_60_64
) = {
pop_size_t { 1234 },
pop_size_t { 1100 }
};

DemographicAgeSexMortalitySchedule mortality {};

mortality.get(
demographic_age_band_t::AGE_60_64
) = {
23117,
31891
};

DemographicMortalityRemainders remainders {};

remainders.get(
demographic_age_band_t::AGE_60_64
) = {
127811,
773201
};

auto const first =
generate_demographic_deaths_from_annual_mortality(
population,
mortality,
remainders
);

auto const second =
generate_demographic_deaths_from_annual_mortality(
population,
mortality,
remainders
);

REQUIRE(first.valid());
REQUIRE(second.valid());

CHECK(first == second);
}