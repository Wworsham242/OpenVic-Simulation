#include "openvic-simulation/population/DemographicCohortAging.hpp"

#include <cstdint>
#include <limits>

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

namespace {

PopulationAgeSexStructure make_aging_start() {
PopulationAgeSexStructure structure {};

structure.get(
demographic_age_band_t::AGE_0_4
) = { pop_size_t { 50 }, pop_size_t { 60 } };

structure.get(
demographic_age_band_t::AGE_5_9
) = { pop_size_t { 40 }, pop_size_t { 45 } };

structure.get(
demographic_age_band_t::AGE_80_84
) = { pop_size_t { 12 }, pop_size_t { 8 } };

structure.get(
demographic_age_band_t::AGE_85_PLUS
) = { pop_size_t { 20 }, pop_size_t { 10 } };

return structure;
}

}

TEST_CASE(
"006B1.2 zero aging flow preserves population exactly",
"[convergence][006b1][006b1.2][population][demography][aging]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow const flow {};

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

REQUIRE(result.valid());

CHECK(result.ending == starting);
CHECK(
result.starting_population
== result.ending_population
);
CHECK(result.get_total_aged() == 0);
}

TEST_CASE(
"006B1.2 explicit aging moves population to exactly the next band",
"[convergence][006b1][006b1.2][population][demography][aging]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
) = {
pop_size_t { 7 },
pop_size_t { 9 }
};

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_0_4
).female
) == 43
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_0_4
).male
) == 51
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_5_9
).female
) == 47
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_5_9
).male
) == 54
);

CHECK(result.female_aged == 7);
CHECK(result.male_aged == 9);
CHECK(
result.ending_population
== result.starting_population
);
}

TEST_CASE(
"006B1.2 simultaneous adjacent aging uses original source stocks",
"[convergence][006b1][006b1.2][population][demography][aging]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
) = {
pop_size_t { 10 },
pop_size_t { 12 }
};

flow.outflow.get(
demographic_age_band_t::AGE_5_9
) = {
pop_size_t { 8 },
pop_size_t { 9 }
};

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_5_9
).female
) == 42
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_5_9
).male
) == 48
);

CHECK(result.female_aged == 18);
CHECK(result.male_aged == 21);
}

TEST_CASE(
"006B1.2 age eighty to eighty four flows into terminal eighty five plus",
"[convergence][006b1][006b1.2][population][demography][aging][terminal]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_80_84
) = {
pop_size_t { 5 },
pop_size_t { 3 }
};

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

REQUIRE(result.valid());

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_80_84
).female
) == 7
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_80_84
).male
) == 5
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_85_PLUS
).female
) == 25
);

CHECK(
type_safe::get(
result.ending.get(
demographic_age_band_t::AGE_85_PLUS
).male
) == 13
);

CHECK(
result.ending_population
== result.starting_population
);
}

TEST_CASE(
"006B1.2 terminal cohort cannot age out",
"[convergence][006b1][006b1.2][population][demography][aging][validation]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_85_PLUS
).female = pop_size_t { 1 };

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_cohort_aging_status_t::
TERMINAL_OUTFLOW_NOT_ALLOWED
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.2 rejects aging outflow larger than available source cohort",
"[convergence][006b1][006b1.2][population][demography][aging][validation]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
).female = pop_size_t { 51 };

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_cohort_aging_status_t::
OUTFLOW_EXCEEDS_AVAILABLE
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.2 rejects negative aging flow",
"[convergence][006b1][006b1.2][population][demography][aging][validation]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
).male = pop_size_t { -1 };

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_cohort_aging_status_t::
INVALID_FLOW_STATE
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.2 rejects destination cell overflow atomically",
"[convergence][006b1][006b1.2][population][demography][aging][overflow]"
) {
PopulationAgeSexStructure starting {};

starting.get(
demographic_age_band_t::AGE_0_4
).female = pop_size_t { 1 };

starting.get(
demographic_age_band_t::AGE_5_9
).female = pop_size_t {
std::numeric_limits<int32_t>::max()
};

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
).female = pop_size_t { 1 };

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

CHECK_FALSE(result.valid());

CHECK(
result.status
== demographic_cohort_aging_status_t::
CELL_OVERFLOW
);

CHECK(result.ending == starting);
}

TEST_CASE(
"006B1.2 aging conserves female and male populations separately",
"[convergence][006b1][006b1.2][population][demography][aging][accounting]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
) = {
pop_size_t { 7 },
pop_size_t { 6 }
};

flow.outflow.get(
demographic_age_band_t::AGE_5_9
) = {
pop_size_t { 5 },
pop_size_t { 4 }
};

flow.outflow.get(
demographic_age_band_t::AGE_80_84
) = {
pop_size_t { 3 },
pop_size_t { 2 }
};

auto const result =
apply_demographic_cohort_aging(
starting,
flow
);

REQUIRE(result.valid());

CHECK(
result.ending.get_female_population()
== starting.get_female_population()
);

CHECK(
result.ending.get_male_population()
== starting.get_male_population()
);

CHECK(
result.ending.get_total_population()
== starting.get_total_population()
);
}

TEST_CASE(
"006B1.2 identical aging inputs produce identical results",
"[convergence][006b1][006b1.2][population][demography][aging][determinism]"
) {
auto const starting = make_aging_start();

DemographicCohortAgingFlow flow {};

flow.outflow.get(
demographic_age_band_t::AGE_0_4
) = {
pop_size_t { 9 },
pop_size_t { 8 }
};

flow.outflow.get(
demographic_age_band_t::AGE_80_84
) = {
pop_size_t { 4 },
pop_size_t { 2 }
};

auto const first =
apply_demographic_cohort_aging(
starting,
flow
);

auto const second =
apply_demographic_cohort_aging(
starting,
flow
);

REQUIRE(first.valid());
REQUIRE(second.valid());

CHECK(first == second);
}