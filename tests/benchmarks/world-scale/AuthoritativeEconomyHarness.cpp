#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>


#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/economy/ProductiveSiteBinding.hpp"
#include "openvic-simulation/economy/production/AggregateProducer.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/misc/GameRulesManager.hpp"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

using namespace OpenVic;

namespace {

struct Tier {
    std::string_view name;
    std::size_t socioeconomic_records;
};

struct Profile {
    std::string_view name;
    std::uint32_t active_percent;
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

Profile parse_profile(std::string_view name) {
    // 006A2 benchmark workload controls for socioeconomic dirty/activity rate.
    // These are not empirical economic assumptions.
    if (name == "quiet") {
        return { "quiet", 1 };
    }
    if (name == "ordinary") {
        return { "ordinary", 5 };
    }
    if (name == "crisis") {
        return { "crisis", 15 };
    }

    std::cerr << "Unknown profile: " << name << "\n";
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

constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

std::uint64_t hash_u64(std::uint64_t seed, std::uint64_t value) {
    seed ^= value;
    seed *= FNV_PRIME;
    return seed;
}

std::uint64_t hash_string(std::uint64_t seed, std::string_view value) {
    for (unsigned char c : value) {
        seed ^= static_cast<std::uint64_t>(c);
        seed *= FNV_PRIME;
    }
    return seed;
}

std::size_t active_index(std::size_t ordinal, std::size_t record_count) {
    // 7919 is coprime to every 006A2 record count (all use only factors 2/5),
    // producing a deterministic non-contiguous permutation without duplicates.
    return (ordinal * 7'919u) % record_count;
}

} // namespace

int main(int argc, char** argv) {
    std::string_view tier_name = "smoke";
    std::string_view profile_name = "ordinary";

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--tier" && i + 1 < argc) {
            tier_name = argv[++i];
        } else if (arg == "--profile" && i + 1 < argc) {
            profile_name = argv[++i];
        } else if (arg == "--help") {
            std::cout
                << "Usage: openvic-simulation.world-scale-economy "
                << "--tier smoke|regional|large|world "
                << "--profile quiet|ordinary|crisis\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 2;
        }
    }

    Tier const tier = parse_tier(tier_name);
    Profile const profile = parse_profile(profile_name);

    GameRulesManager rules;
    rules.use_recommended_rules();

    GoodCategory category {
        "benchmark-category",
        good_category_index_t { 0 }
    };

    GoodDefinition output_good {
        "benchmark-output",
        colour_t {},
        good_index_t { 0 },
        category,
        fixed_point_t { 1 },
        true,
        true,
        false,
        false
    };

    ProductionType production_type {
        rules,
        "process",
        std::nullopt,
        memory::vector<Job> {},
        ProductionType::template_type_t::PROCESS,
        pop_size_t { 100 },
        fixed_point_map_t<GoodDefinition const*> {},
        output_good,
        fixed_point_t { 10 },
        memory::vector<ProductionType::bonus_t> {},
        fixed_point_map_t<GoodDefinition const*> {},
        false,
        false,
        false
    };

    std::uint64_t const memory_before = working_set_bytes();

    std::vector<AggregateProducer> producers;
    producers.reserve(tier.socioeconomic_records);

    std::vector<ProductiveSiteBinding> bindings;
    bindings.reserve(tier.socioeconomic_records);

    auto const create_begin = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < tier.socioeconomic_records; ++i) {
        std::string const suffix = std::to_string(i);

        fixed_point_t const capacity {
            static_cast<std::int32_t>(1 + (i % 10))
        };

        producers.emplace_back(
            std::string { "s" } + suffix,
            production_type,
            capacity,
            fixed_point_t::_1
        );

        // Enable the existing workforce-constraint path without invoking a
        // separate allocator. The workload supplies exactly installed need.
        producers.back().set_available_workforce(
            capacity * 100
        );

        bindings.push_back(ProductiveSiteBinding {
            .province_id = std::string { "p" } + suffix,
            .building_id = std::string { "b" } + suffix,
            .production_type_id = "process",
            .labor_scope = LaborPoolScope::ProvinceLocal
        });
    }

    auto const create_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_create = working_set_bytes();

    std::size_t const active_sites =
        (tier.socioeconomic_records * profile.active_percent) / 100;

    std::uint64_t aggregate_output_raw = 0;

    auto const production_begin = std::chrono::steady_clock::now();

    for (std::size_t ordinal = 0; ordinal < active_sites; ++ordinal) {
        std::size_t const index =
            active_index(ordinal, tier.socioeconomic_records);

        AggregateProductionResult const result =
            producers[index].produce();

        aggregate_output_raw += static_cast<std::uint64_t>(
            result.actual_output.get_raw_value()
        );
    }

    auto const production_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_production = working_set_bytes();

    std::uint64_t hash = FNV_OFFSET;
    std::uint64_t invalid_bindings = 0;
    std::uint64_t invalid_producers = 0;
    std::uint64_t produced_sites = 0;

    auto const validate_begin = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < tier.socioeconomic_records; ++i) {
        AggregateProducer const& producer = producers[i];
        ProductiveSiteBinding const& binding = bindings[i];

        if (
            binding.production_type_id !=
            producer.get_production_type().get_identifier()
        ) {
            ++invalid_bindings;
        }

        if (
            producer.get_capacity() < fixed_point_t::_0 ||
            producer.get_utilization() < fixed_point_t::_0 ||
            producer.get_utilization() > fixed_point_t::_1 ||
            !producer.is_workforce_constrained()
        ) {
            ++invalid_producers;
        }

        fixed_point_t const output_inventory =
            producer.get_inventory(output_good);

        if (output_inventory > fixed_point_t::_0) {
            ++produced_sites;
        }

        hash = hash_string(hash, producer.get_identifier());
        hash = hash_string(hash, binding.province_id);
        hash = hash_string(hash, binding.building_id);
        hash = hash_string(hash, binding.production_type_id);
        hash = hash_u64(
            hash,
            static_cast<std::uint64_t>(
                producer.get_capacity().get_raw_value()
            )
        );
        hash = hash_u64(
            hash,
            static_cast<std::uint64_t>(
                producer.get_available_workforce().get_raw_value()
            )
        );
        hash = hash_u64(
            hash,
            static_cast<std::uint64_t>(
                output_inventory.get_raw_value()
            )
        );
    }

    auto const validate_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_validate = working_set_bytes();

    std::cout
        << "{\n"
        << "  \"increment\": \"PROJECT-CONVERGENCE-006A2.3\",\n"
        << "  \"scope\": \"authoritative-aggregate-production-state\",\n"
        << "  \"tier\": \"" << tier.name << "\",\n"
        << "  \"profile\": \"" << profile.name << "\",\n"
        << "  \"counts\": {\n"
        << "    \"productive_sites\": "
        << tier.socioeconomic_records << ",\n"
        << "    \"productive_site_bindings\": "
        << bindings.size() << ",\n"
        << "    \"active_sites\": " << active_sites << ",\n"
        << "    \"produced_sites\": " << produced_sites << "\n"
        << "  },\n"
        << "  \"validation\": {\n"
        << "    \"invalid_bindings\": " << invalid_bindings << ",\n"
        << "    \"invalid_producers\": " << invalid_producers << ",\n"
        << "    \"aggregate_output_raw\": "
        << aggregate_output_raw << "\n"
        << "  },\n"
        << "  \"timing_ms\": {\n"
        << "    \"create\": "
        << elapsed_ms(create_begin, create_end) << ",\n"
        << "    \"production\": "
        << elapsed_ms(production_begin, production_end) << ",\n"
        << "    \"validate_and_hash\": "
        << elapsed_ms(validate_begin, validate_end) << "\n"
        << "  },\n"
        << "  \"memory_bytes\": {\n"
        << "    \"before\": " << memory_before << ",\n"
        << "    \"after_create\": " << memory_after_create << ",\n"
        << "    \"after_production\": "
        << memory_after_production << ",\n"
        << "    \"after_validate\": "
        << memory_after_validate << "\n"
        << "  },\n"
        << "  \"deterministic_hash\": " << hash << ",\n"
        << "  \"limitations\": [\n"
        << "    \"uses real AggregateProducer, ProductionType and ProductiveSiteBinding state\",\n"
        << "    \"uses the real AggregateProducer::produce path and output inventory mutation\",\n"
        << "    \"006A2 activity percentages are benchmark workload controls, not empirical economic assumptions\",\n"
        << "    \"does not certify market clearing, price formation, workforce allocation, input acquisition or inter-site trade\",\n"
        << "    \"one shared benchmark production definition isolates runtime site-state scale from content-catalog scale\"\n"
        << "  ]\n"
        << "}\n";

    return (
        invalid_bindings == 0 &&
        invalid_producers == 0 &&
        produced_sites == active_sites
    ) ? 0 : 4;
}