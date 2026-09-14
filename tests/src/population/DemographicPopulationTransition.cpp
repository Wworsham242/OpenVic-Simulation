#include "openvic-simulation/population/DemographicPopulationTransition.hpp"

#include <cstdint>
#include <limits>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {

PopulationAgeSexStructure make_starting_structure() {
PopulationAgeSexStructure structure {};

structure.get(
demographic_age_band_t::AGE_0_4
) = { pop_size_t { 50 }, pop_size_t { 55 } };

structure.get(
demographic_age_band_t::AGE_20_24
) = { pop_size_t { 100 }, pop_size_t { 90 } };

structure.get(
demographic_age_band_t::AGE_70_74
) = { pop_size_t { 25 }, pop_size_t { 20 } };

return structure;
}

}

TEST_CASE(
"006B1.1 zero demographic components preserve starting population",
"[convergence][006b1][006b1.1][population][demography]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents const components {};

auto const result =
apply_demographic_population_components(
starting,
components
);

REQUIRE(result.valid());

CHECK(result.starting == starting);
CHECK(result.ending == starting);
CHECK(result.starting_population == 340);
CHECK(result.ending_population == 340);
CHECK(result.net_population_change == 0);
CHECK(result.births == 0);
CHECK(result.deaths == 0);
CHECK(result.immigration == 0);
CHECK(result.emigration == 0);
}

TEST_CASE(
"006B1.1 births enter only the zero to four cohort",
"[convergence][006b1][006b1.1][population][demography][births]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};
components.births = {
pop_size_t { 8 },
pop_size_t { 7 }
};

auto const result =
apply_demographic_population_components(
starting,
components
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_0_4
).female
) == 58
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_0_4
).male
) == 62
);

CHECK(
result.ending.get(
demographic_age_band_t::AGE_20_24
)
== starting.get(
demographic_age_band_t::AGE_20_24
)
);

CHECK(result.births == 15);
CHECK(result.ending_population == 355);
CHECK(result.net_population_change == 15);
}

TEST_CASE(
"006B1.1 deaths remove only explicitly identified cohort population",
"[convergence][006b1][006b1.1][population][demography][deaths]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.deaths.get(
demographic_age_band_t::AGE_70_74
) = {
pop_size_t { 5 },
pop_size_t { 4 }
};

auto const result =
apply_demographic_population_components(
starting,
components
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_70_74
).female
) == 20
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_70_74
).male
) == 16
);

CHECK(result.deaths == 9);
CHECK(result.ending_population == 331);
CHECK(result.net_population_change == -9);
}

TEST_CASE(
"006B1.1 immigration and emigration preserve explicit cohort identity",
"[convergence][006b1][006b1.1][population][demography][migration]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.immigration.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 20 },
pop_size_t { 10 }
};

components.emigration.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 5 },
pop_size_t { 3 }
};

auto const result =
apply_demographic_population_components(
starting,
components
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_20_24
).female
) == 115
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_20_24
).male
) == 97
);

CHECK(result.immigration == 30);
CHECK(result.emigration == 8);
CHECK(result.net_population_change == 22);
}

TEST_CASE(
"006B1.1 mixed components satisfy exact population identity",
"[convergence][006b1][006b1.1][population][demography][accounting]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.births = {
pop_size_t { 6 },
pop_size_t { 5 }
};

components.deaths.get(
demographic_age_band_t::AGE_70_74
) = {
pop_size_t { 3 },
pop_size_t { 2 }
};

components.immigration.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 12 },
pop_size_t { 8 }
};

components.emigration.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 4 },
pop_size_t { 6 }
};

auto const result =
apply_demographic_population_components(
starting,
components
);

REQUIRE(result.valid());

CHECK(result.starting_population == 340);
CHECK(result.births == 11);
CHECK(result.deaths == 5);
CHECK(result.immigration == 20);
CHECK(result.emigration == 10);

CHECK(
result.ending_population
== result.starting_population
+ result.births
- result.deaths
+ result.immigration
- result.emigration
);

CHECK(result.ending_population == 356);
CHECK(result.net_population_change == 16);
}

TEST_CASE(
"006B1.1 rejects deaths larger than starting cohort population atomically",
"[convergence][006b1][006b1.1][population][demography][validation]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.deaths.get(
demographic_age_band_t::AGE_70_74
).female = pop_size_t { 26 };

auto const result =
apply_demographic_population_components(
starting,
components
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_population_transition_status_t::
DEATHS_EXCEED_AVAILABLE
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.1 rejects emigration larger than post-component available cohort",
"[convergence][006b1][006b1.1][population][demography][validation]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.emigration.get(
demographic_age_band_t::AGE_20_24
).male = pop_size_t { 91 };

auto const result =
apply_demographic_population_components(
starting,
components
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_population_transition_status_t::
EMIGRATION_EXCEEDS_AVAILABLE
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.1 rejects negative demographic components",
"[convergence][006b1][006b1.1][population][demography][validation]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.immigration.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { -1 };

auto const result =
apply_demographic_population_components(
starting,
components
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_population_transition_status_t::
INVALID_COMPONENT_STATE
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.1 rejects cell overflow atomically",
"[convergence][006b1][006b1.1][population][demography][overflow]"
) {
PopulationAgeSexStructure starting {};

starting.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t {
std::numeric_limits<int32_t>::max()
};

DemographicPopulationComponents components {};

components.immigration.get(
demographic_age_band_t::AGE_20_24
).female = pop_size_t { 1 };

auto const result =
apply_demographic_population_components(
starting,
components
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_population_transition_status_t::
CELL_OVERFLOW
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.1 identical inputs produce identical transition results",
"[convergence][006b1][006b1.1][population][demography][determinism]"
) {
PopulationAgeSexStructure const starting =
make_starting_structure();

DemographicPopulationComponents components {};

components.births = {
pop_size_t { 4 },
pop_size_t { 3 }
};

components.deaths.get(
demographic_age_band_t::AGE_70_74
) = {
pop_size_t { 2 },
pop_size_t { 1 }
};

components.immigration.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 10 },
pop_size_t { 9 }
};

components.emigration.get(
demographic_age_band_t::AGE_20_24
) = {
pop_size_t { 3 },
pop_size_t { 2 }
};

auto const first =
apply_demographic_population_components(
starting,
components
);

auto const second =
apply_demographic_population_components(
starting,
components
);

REQUIRE(first.valid());
REQUIRE(second.valid());

CHECK(first == second);
}