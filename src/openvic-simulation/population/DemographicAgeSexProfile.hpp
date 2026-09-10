#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

namespace OpenVic {
	/*
	 * Relative age-sex profile used to initialize authoritative
	 * demographic counts.
	 *
	 * Weights are proportions only. They are not population counts and
	 * therefore do not constitute a second population ledger.
	 */
	struct AgeSexCohortWeight {
		uint32_t female = 0;
		uint32_t male = 0;

		bool operator==(AgeSexCohortWeight const&) const = default;
	};

	struct DemographicAgeSexProfile {
		std::array<AgeSexCohortWeight, DEMOGRAPHIC_AGE_BAND_COUNT> cohorts {};

		[[nodiscard]] constexpr AgeSexCohortWeight const& get(
			demographic_age_band_t age_band
		) const {
			return cohorts[get_demographic_age_band_index(age_band)];
		}

		[[nodiscard]] constexpr AgeSexCohortWeight& get(
			demographic_age_band_t age_band
		) {
			return cohorts[get_demographic_age_band_index(age_band)];
		}

		[[nodiscard]] constexpr uint64_t get_total_weight() const {
			uint64_t total = 0;

			for (AgeSexCohortWeight const& cohort : cohorts) {
				total += cohort.female;
				total += cohort.male;
			}

			return total;
		}

		[[nodiscard]] constexpr bool has_population_shape() const {
			return get_total_weight() > 0;
		}

		bool operator==(DemographicAgeSexProfile const&) const = default;
	};

	inline constexpr size_t DEMOGRAPHIC_AGE_SEX_CELL_COUNT =
		DEMOGRAPHIC_AGE_BAND_COUNT * 2;

	[[nodiscard]] constexpr uint32_t get_demographic_profile_cell_weight(
		DemographicAgeSexProfile const& profile,
		size_t cell_index
	) {
		const size_t age_index = cell_index / 2;
		const bool male = (cell_index % 2) != 0;

		return male
			? profile.cohorts[age_index].male
			: profile.cohorts[age_index].female;
	}

	constexpr void set_demographic_structure_cell(
		PopulationAgeSexStructure& structure,
		size_t cell_index,
		pop_size_t count
	) {
		const size_t age_index = cell_index / 2;
		const bool male = (cell_index % 2) != 0;

		if (male) {
			structure.cohorts[age_index].male = count;
		} else {
			structure.cohorts[age_index].female = count;
		}
	}

	struct DemographicAgeSexInitializationResult {
		PopulationAgeSexStructure structure {};
		uint64_t profile_weight_sum = 0;
		int64_t floor_assigned_population = 0;
		int64_t remainder_assigned_population = 0;
		bool valid = false;

		bool operator==(DemographicAgeSexInitializationResult const&) const = default;
	};

	/*
	 * Materialize relative profile weights into exact integer cohort
	 * counts while preserving the authoritative population total.
	 *
	 * Integer largest-remainder apportionment is used solely as an
	 * accounting/rounding mechanism. It is not a demographic model.
	 *
	 * Tie-breaking is deterministic: lower flattened cell index wins.
	 * Cell order is female then male within each ascending age band.
	 */
	[[nodiscard]] constexpr DemographicAgeSexInitializationResult
	materialize_demographic_age_sex_profile(
		DemographicAgeSexProfile const& profile,
		pop_size_t population
	) {
		DemographicAgeSexInitializationResult result {};
		result.profile_weight_sum = profile.get_total_weight();

		const int64_t population_value = type_safe::get(population);

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

		std::array<uint64_t, DEMOGRAPHIC_AGE_SEX_CELL_COUNT> remainders {};

		int64_t assigned = 0;

		for (size_t cell_index = 0;
			cell_index < DEMOGRAPHIC_AGE_SEX_CELL_COUNT;
			++cell_index
		) {
			const uint64_t weight =
				get_demographic_profile_cell_weight(profile, cell_index);

			const uint64_t numerator =
				static_cast<uint64_t>(population_value) * weight;

			const uint64_t floor_count =
				numerator / result.profile_weight_sum;

			remainders[cell_index] =
				numerator % result.profile_weight_sum;

			set_demographic_structure_cell(
				result.structure,
				cell_index,
				pop_size_t { static_cast<int32_t>(floor_count) }
			);

			assigned += static_cast<int64_t>(floor_count);
		}

		result.floor_assigned_population = assigned;

		int64_t population_left = population_value - assigned;

		while (population_left > 0) {
			size_t best_cell = 0;
			uint64_t best_remainder = 0;
			bool found = false;

			for (size_t cell_index = 0;
				cell_index < DEMOGRAPHIC_AGE_SEX_CELL_COUNT;
				++cell_index
			) {
				if (
					!found
					|| remainders[cell_index] > best_remainder
				) {
					best_cell = cell_index;
					best_remainder = remainders[cell_index];
					found = true;
				}
			}

			if (!found) {
				return result;
			}

			const size_t age_index = best_cell / 2;
			const bool male = (best_cell % 2) != 0;

			AgeSexCohortCount& cohort =
				result.structure.cohorts[age_index];

			pop_size_t& count = male ? cohort.male : cohort.female;

			count = pop_size_t {
				static_cast<int32_t>(type_safe::get(count) + 1)
			};

			remainders[best_cell] = 0;
			--population_left;
			++result.remainder_assigned_population;
		}

		result.valid =
			result.structure.is_consistent_with_population(population);

		return result;
	}
}
