#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/population/PopSize.hpp"

namespace OpenVic {
	/*
	 * Demographic age bands for cohort-component population models.
	 *
	 * Closed groups are five years wide. The terminal 85+ group is
	 * open-ended, matching a common demographic aggregation.
	 */
	enum struct demographic_age_band_t : uint8_t {
		AGE_0_4,
		AGE_5_9,
		AGE_10_14,
		AGE_15_19,
		AGE_20_24,
		AGE_25_29,
		AGE_30_34,
		AGE_35_39,
		AGE_40_44,
		AGE_45_49,
		AGE_50_54,
		AGE_55_59,
		AGE_60_64,
		AGE_65_69,
		AGE_70_74,
		AGE_75_79,
		AGE_80_84,
		AGE_85_PLUS,
		COUNT
	};

	inline constexpr size_t DEMOGRAPHIC_AGE_BAND_COUNT =
		static_cast<size_t>(demographic_age_band_t::COUNT);

	inline constexpr std::array<uint8_t, DEMOGRAPHIC_AGE_BAND_COUNT>
	DEMOGRAPHIC_AGE_BAND_LOWER_BOUNDS {
		0, 5, 10, 15, 20, 25, 30, 35, 40,
		45, 50, 55, 60, 65, 70, 75, 80, 85
	};

	[[nodiscard]] constexpr size_t get_demographic_age_band_index(
		demographic_age_band_t age_band
	) {
		return static_cast<size_t>(age_band);
	}

	[[nodiscard]] constexpr demographic_age_band_t get_demographic_age_band_for_age(
		uint8_t age
	) {
		if (age >= 85) {
			return demographic_age_band_t::AGE_85_PLUS;
		}

		return static_cast<demographic_age_band_t>(age / 5);
	}

	/*
	 * Female/male here are demographic accounting categories used by
	 * fertility and mortality models. They do not model social gender
	 * identity, which belongs to a separate social representation layer.
	 */
	struct AgeSexCohortCount {
		pop_size_t female = pop_size_t { 0 };
		pop_size_t male = pop_size_t { 0 };

		bool operator==(AgeSexCohortCount const&) const = default;
	};

	struct PopulationAgeSexStructure {
		std::array<AgeSexCohortCount, DEMOGRAPHIC_AGE_BAND_COUNT> cohorts {};

		[[nodiscard]] constexpr AgeSexCohortCount const& get(
			demographic_age_band_t age_band
		) const {
			return cohorts[get_demographic_age_band_index(age_band)];
		}

		[[nodiscard]] constexpr AgeSexCohortCount& get(
			demographic_age_band_t age_band
		) {
			return cohorts[get_demographic_age_band_index(age_band)];
		}

		[[nodiscard]] constexpr bool has_nonnegative_counts() const {
			for (AgeSexCohortCount const& cohort : cohorts) {
				if (
					type_safe::get(cohort.female) < 0
					|| type_safe::get(cohort.male) < 0
				) {
					return false;
				}
			}

			return true;
		}

		[[nodiscard]] constexpr int64_t get_female_population() const {
			int64_t total = 0;

			for (AgeSexCohortCount const& cohort : cohorts) {
				total += type_safe::get(cohort.female);
			}

			return total;
		}

		[[nodiscard]] constexpr int64_t get_male_population() const {
			int64_t total = 0;

			for (AgeSexCohortCount const& cohort : cohorts) {
				total += type_safe::get(cohort.male);
			}

			return total;
		}

		[[nodiscard]] constexpr int64_t get_total_population() const {
			return get_female_population() + get_male_population();
		}

		[[nodiscard]] constexpr bool is_consistent_with_population(
			pop_size_t population
		) const {
			return
				has_nonnegative_counts()
				&& get_total_population() == type_safe::get(population);
		}

		bool operator==(PopulationAgeSexStructure const&) const = default;
	};
}
