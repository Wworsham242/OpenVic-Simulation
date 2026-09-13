#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#include <type_safe/strong_typedef.hpp>

#include "openvic-simulation/ecs/ChecksumTraits.hpp"
#include "openvic-simulation/population/DemographicAgeSexProfile.hpp"
#include "openvic-simulation/population/PopulationAgeSexStructure.hpp"

using namespace OpenVic;

namespace {

struct Tier {
    std::string_view name;
    std::size_t socioeconomic_records;
};

Tier parse_tier(std::string_view name) {
    if (name == "smoke") {
        return { "smoke", 10'000 };
    }
    if (name == "regional") {
        return { "regional", 100'000 };
    }
    if (name == "large") {
        return { "large", 500'000 };
    }
    if (name == "world") {
        return { "world", 1'000'000 };
    }

    std::cerr << "Unknown tier: " << name << "\n";
    std::exit(2);
}

std::uint64_t working_set_bytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters {};
    counters.cb = sizeof(counters);

    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters)
        )) {
        return static_cast<std::uint64_t>(counters.WorkingSetSize);
    }
#endif
    return 0;
}

double elapsed_ms(
    std::chrono::steady_clock::time_point begin,
    std::chrono::steady_clock::time_point end
) {
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

DemographicAgeSexProfile make_reference_profile() {
    DemographicAgeSexProfile profile {};

    // Benchmark initialization weights only. These are deliberately not
    // empirical fertility/mortality assumptions and are not gameplay data.
    for (std::size_t i = 0; i < DEMOGRAPHIC_AGE_BAND_COUNT; ++i) {
        profile.cohorts[i].female = static_cast<std::uint32_t>(100 + i * 3);
        profile.cohorts[i].male = static_cast<std::uint32_t>(101 + i * 3);
    }

    return profile;
}

pop_size_t authoritative_population_for(std::size_t index) {
    // Deterministic, bounded authoritative total for benchmark state.
    // The value is generated from record identity; no parallel population
    // ledger is stored.
    return pop_size_t {
        static_cast<std::int32_t>(1'000 + (index % 100'000))
    };
}

std::uint64_t hash_structure(
    PopulationAgeSexStructure const& structure,
    std::uint64_t seed
) {
    std::uint64_t h = seed;

    for (AgeSexCohortCount const& cohort : structure.cohorts) {
        auto const female = static_cast<std::int64_t>(
            type_safe::get(cohort.female)
        );
        auto const male = static_cast<std::int64_t>(
            type_safe::get(cohort.male)
        );

        h = OpenVic::ecs::fold_uint64(
            static_cast<std::uint64_t>(female),
            h
        );
        h = OpenVic::ecs::fold_uint64(
            static_cast<std::uint64_t>(male),
            h
        );
    }

    return h;
}

} // namespace

int main(int argc, char** argv) {
    std::string_view tier_name = "smoke";

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--tier" && i + 1 < argc) {
            tier_name = argv[++i];
        } else if (arg == "--help") {
            std::cout
                << "Usage: openvic-simulation.world-scale-demographics "
                << "--tier smoke|regional|large|world\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 2;
        }
    }

    Tier const tier = parse_tier(tier_name);
    DemographicAgeSexProfile const profile = make_reference_profile();

    std::uint64_t const memory_before = working_set_bytes();

    std::vector<PopulationAgeSexStructure> structures;
    structures.reserve(tier.socioeconomic_records);

    auto const materialize_begin = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < tier.socioeconomic_records; ++i) {
        pop_size_t const authoritative_population =
            authoritative_population_for(i);

        DemographicAgeSexInitializationResult const result =
            materialize_demographic_age_sex_profile(
                profile,
                authoritative_population
            );

        if (!result.valid) {
            std::cerr << "Demographic materialization failed at record "
                      << i << "\n";
            return 3;
        }

        structures.push_back(result.structure);
    }

    auto const materialize_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_create = working_set_bytes();

    std::uint64_t hash = OpenVic::ecs::CHECKSUM_SEED;
    std::uint64_t inconsistent_records = 0;
    std::uint64_t total_population = 0;

    auto const validate_begin = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < structures.size(); ++i) {
        PopulationAgeSexStructure const& structure = structures[i];
        pop_size_t const authoritative_population =
            authoritative_population_for(i);

        if (!structure.is_consistent_with_population(
                authoritative_population
            )) {
            ++inconsistent_records;
        }

        total_population += static_cast<std::uint64_t>(
            structure.get_total_population()
        );

        hash = hash_structure(structure, hash);
    }

    auto const validate_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_validate = working_set_bytes();

    std::cout
        << "{\n"
        << "  \"increment\": \"PROJECT-CONVERGENCE-006A2.2\",\n"
        << "  \"scope\": \"authoritative-demographic-age-sex-state\",\n"
        << "  \"tier\": \"" << tier.name << "\",\n"
        << "  \"counts\": {\n"
        << "    \"socioeconomic_records\": "
        << tier.socioeconomic_records << ",\n"
        << "    \"age_sex_cells\": "
        << tier.socioeconomic_records * DEMOGRAPHIC_AGE_SEX_CELL_COUNT
        << "\n"
        << "  },\n"
        << "  \"validation\": {\n"
        << "    \"inconsistent_records\": " << inconsistent_records << ",\n"
        << "    \"aggregate_population\": " << total_population << "\n"
        << "  },\n"
        << "  \"timing_ms\": {\n"
        << "    \"materialize\": "
        << elapsed_ms(materialize_begin, materialize_end) << ",\n"
        << "    \"validate_and_hash\": "
        << elapsed_ms(validate_begin, validate_end) << "\n"
        << "  },\n"
        << "  \"memory_bytes\": {\n"
        << "    \"before\": " << memory_before << ",\n"
        << "    \"after_materialize\": " << memory_after_create << ",\n"
        << "    \"after_validate\": " << memory_after_validate << "\n"
        << "  },\n"
        << "  \"deterministic_hash\": " << hash << ",\n"
        << "  \"limitations\": [\n"
        << "    \"uses real PopulationAgeSexStructure and real deterministic profile materialization\",\n"
        << "    \"does not yet model births, deaths, migration, fertility or mortality transitions\",\n"
        << "    \"activity profiles are intentionally not fabricated for static demographic initialization\",\n"
        << "    \"authoritative population totals are generated from record identity and are not stored as a second ledger\"\n"
        << "  ]\n"
        << "}\n";

    return inconsistent_records == 0 ? 0 : 4;
}