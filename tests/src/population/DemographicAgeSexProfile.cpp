#include "openvic-simulation/population/DemographicAgeSexProfile.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"004A7 profile weights remain non-authoritative proportions",
	"[convergence][004a7][population][demography][profile]"
) {
	DemographicAgeSexProfile profile {};

	profile.get(demographic_age_band_t::AGE_0_4) = { 20, 21 };
	profile.get(demographic_age_band_t::AGE_20_24) = { 30, 29 };

	CHECK(profile.get_total_weight() == 100);
	CHECK(profile.has_population_shape());
}

TEST_CASE(
	"004A7 materialization preserves exact authoritative population total",
	"[convergence][004a7][population][demography][accounting]"
) {
	DemographicAgeSexProfile profile {};

	profile.get(demographic_age_band_t::AGE_0_4) = { 1, 1 };
	profile.get(demographic_age_band_t::AGE_5_9) = { 1, 1 };
	profile.get(demographic_age_band_t::AGE_10_14) = { 1, 1 };

	const auto result =
		materialize_demographic_age_sex_profile(
			profile,
			pop_size_t { 1003 }
		);

	REQUIRE(result.valid);
	CHECK(result.profile_weight_sum == 6);
	CHECK(result.structure.get_total_population() == 1003);
	CHECK(
		result.structure.is_consistent_with_population(
			pop_size_t { 1003 }
		)
	);
	CHECK(
		result.floor_assigned_population
		+ result.remainder_assigned_population
		== 1003
	);
}

TEST_CASE(
	"004A7 largest remainder allocation is deterministic under ties",
	"[convergence][004a7][population][demography][determinism]"
) {
	DemographicAgeSexProfile profile {};

	profile.get(demographic_age_band_t::AGE_0_4) = { 1, 1 };
	profile.get(demographic_age_band_t::AGE_5_9) = { 1, 1 };

	const auto first =
		materialize_demographic_age_sex_profile(
			profile,
			pop_size_t { 3 }
		);

	const auto second =
		materialize_demographic_age_sex_profile(
			profile,
			pop_size_t { 3 }
		);

	REQUIRE(first.valid);
	CHECK(first == second);

	CHECK(
		type_safe::get(
			first.structure.get(
				demographic_age_band_t::AGE_0_4
			).female
		) == 1
	);

	CHECK(
		type_safe::get(
			first.structure.get(
				demographic_age_band_t::AGE_0_4
			).male
		) == 1
	);

	CHECK(
		type_safe::get(
			first.structure.get(
				demographic_age_band_t::AGE_5_9
			).female
		) == 1
	);

	CHECK(
		type_safe::get(
			first.structure.get(
				demographic_age_band_t::AGE_5_9
			).male
		) == 0
	);
}

TEST_CASE(
	"004A7 zero weight profile cannot initialize positive population",
	"[convergence][004a7][population][demography][validation]"
) {
	DemographicAgeSexProfile profile {};

	const auto result =
		materialize_demographic_age_sex_profile(
			profile,
			pop_size_t { 100 }
		);

	CHECK_FALSE(result.valid);
	CHECK(result.structure.get_total_population() == 0);
}

TEST_CASE(
	"004A7 zero population materializes without inventing people",
	"[convergence][004a7][population][demography][validation]"
) {
	DemographicAgeSexProfile profile {};

	const auto result =
		materialize_demographic_age_sex_profile(
			profile,
			pop_size_t { 0 }
		);

	CHECK(result.valid);
	CHECK(result.structure.get_total_population() == 0);
}

TEST_CASE(
	"004A7 negative authoritative population is rejected",
	"[convergence][004a7][population][demography][validation]"
) {
	DemographicAgeSexProfile profile {};
	profile.get(demographic_age_band_t::AGE_20_24) = { 1, 1 };

	const auto result =
		materialize_demographic_age_sex_profile(
			profile,
			pop_size_t { -1 }
		);

	CHECK_FALSE(result.valid);
	CHECK(result.structure.get_total_population() == 0);
}

TEST_CASE(
	"004A7 profile scale does not change resulting demographic structure",
	"[convergence][004a7][population][demography][scale]"
) {
	DemographicAgeSexProfile first_profile {};
	DemographicAgeSexProfile second_profile {};

	first_profile.get(demographic_age_band_t::AGE_20_24) = { 2, 3 };
	first_profile.get(demographic_age_band_t::AGE_25_29) = { 4, 1 };

	second_profile.get(demographic_age_band_t::AGE_20_24) = { 20, 30 };
	second_profile.get(demographic_age_band_t::AGE_25_29) = { 40, 10 };

	const auto first =
		materialize_demographic_age_sex_profile(
			first_profile,
			pop_size_t { 1000 }
		);

	const auto second =
		materialize_demographic_age_sex_profile(
			second_profile,
			pop_size_t { 1000 }
		);

	REQUIRE(first.valid);
	REQUIRE(second.valid);
	CHECK(first.structure == second.structure);
}
