#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"004A6 age lookup maps ages into five-year demographic cohorts",
	"[convergence][004a6][population][demography][age]"
) {
	CHECK(
		get_demographic_age_band_for_age(0)
		== demographic_age_band_t::AGE_0_4
	);

	CHECK(
		get_demographic_age_band_for_age(4)
		== demographic_age_band_t::AGE_0_4
	);

	CHECK(
		get_demographic_age_band_for_age(5)
		== demographic_age_band_t::AGE_5_9
	);

	CHECK(
		get_demographic_age_band_for_age(84)
		== demographic_age_band_t::AGE_80_84
	);

	CHECK(
		get_demographic_age_band_for_age(85)
		== demographic_age_band_t::AGE_85_PLUS
	);

	CHECK(
		get_demographic_age_band_for_age(120)
		== demographic_age_band_t::AGE_85_PLUS
	);
}

TEST_CASE(
	"004A6 demographic structure preserves female male and total accounting",
	"[convergence][004a6][population][demography][accounting]"
) {
	PopulationAgeSexStructure structure {};

	structure.get(demographic_age_band_t::AGE_0_4) = {
		pop_size_t { 12 },
		pop_size_t { 13 }
	};

	structure.get(demographic_age_band_t::AGE_20_24) = {
		pop_size_t { 30 },
		pop_size_t { 28 }
	};

	structure.get(demographic_age_band_t::AGE_85_PLUS) = {
		pop_size_t { 10 },
		pop_size_t { 7 }
	};

	CHECK(structure.get_female_population() == 52);
	CHECK(structure.get_male_population() == 48);
	CHECK(structure.get_total_population() == 100);
	CHECK(structure.has_nonnegative_counts());
	CHECK(structure.is_consistent_with_population(pop_size_t { 100 }));
	CHECK_FALSE(structure.is_consistent_with_population(pop_size_t { 101 }));
}

TEST_CASE(
	"004A6 negative cohort counts invalidate demographic accounting",
	"[convergence][004a6][population][demography][validation]"
) {
	PopulationAgeSexStructure structure {};

	structure.get(demographic_age_band_t::AGE_30_34).female =
		pop_size_t { -1 };

	CHECK_FALSE(structure.has_nonnegative_counts());
	CHECK_FALSE(
		structure.is_consistent_with_population(pop_size_t { -1 })
	);
}

TEST_CASE(
	"004A6 empty structure is valid only for zero population",
	"[convergence][004a6][population][demography][validation]"
) {
	PopulationAgeSexStructure structure {};

	CHECK(structure.has_nonnegative_counts());
	CHECK(structure.get_total_population() == 0);
	CHECK(structure.is_consistent_with_population(pop_size_t { 0 }));
	CHECK_FALSE(structure.is_consistent_with_population(pop_size_t { 1 }));
}

TEST_CASE(
	"004A6 cohort structure is deterministic value state",
	"[convergence][004a6][population][demography][determinism]"
) {
	PopulationAgeSexStructure first {};
	PopulationAgeSexStructure second {};

	first.get(demographic_age_band_t::AGE_65_69) = {
		pop_size_t { 41 },
		pop_size_t { 37 }
	};

	second.get(demographic_age_band_t::AGE_65_69) = {
		pop_size_t { 41 },
		pop_size_t { 37 }
	};

	CHECK(first == second);
	CHECK(first.get_total_population() == 78);
}
