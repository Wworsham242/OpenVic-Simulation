#include "openvic-simulation/ecs/EntityID.hpp"
#include "openvic-simulation/ecs/World.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#	include <windows.h>
#	include <psapi.h>
#elif defined(__linux__)
#	include <sys/resource.h>
#	include <unistd.h>
#endif

using OpenVic::ecs::EntityID;
using OpenVic::ecs::World;

struct SyntheticActorLoad {
	std::uint64_t treasury = 0;
	std::uint32_t authority_scope = 0;
	std::uint32_t flags = 0;
};

struct SyntheticNetworkPointLoad {
	std::uint64_t capacity = 0;
	std::uint32_t owner = 0;
	std::uint32_t neighbor_count = 0;
	std::uint8_t dirty = 0;
};

struct SyntheticSocioeconomicLoad {
	std::uint64_t population = 0;
	std::uint64_t physical_stock = 0;
	std::uint64_t productive_capacity = 0;
	std::uint32_t owner = 0;
	std::uint8_t dirty = 0;
};

struct SyntheticFormationLoad {
	std::uint64_t sustainment_stock = 0;
	std::uint32_t owner = 0;
	std::uint32_t location = 0;
	std::uint8_t active = 0;
};

// These are benchmark-only workload records, deliberately NOT engine domain models.
// 006A2.1 measures the real ECS/container substrate and work-amplification behavior.
// Later 006A2 slices replace each surrogate with the current authoritative domain type
// where such a type already exists.

inline std::uint64_t ecs_checksum(
    SyntheticActorLoad const& value,
    std::uint64_t seed
) {
    seed = OpenVic::ecs::fold_uint64(value.treasury, seed);
    seed = OpenVic::ecs::fold_uint64(value.authority_scope, seed);
    seed = OpenVic::ecs::fold_uint64(value.flags, seed);
    return seed;
}

inline std::uint64_t ecs_checksum(
    SyntheticNetworkPointLoad const& value,
    std::uint64_t seed
) {
    seed = OpenVic::ecs::fold_uint64(value.capacity, seed);
    seed = OpenVic::ecs::fold_uint64(value.owner, seed);
    seed = OpenVic::ecs::fold_uint64(value.neighbor_count, seed);
    seed = OpenVic::ecs::fold_uint64(value.dirty, seed);
    return seed;
}

inline std::uint64_t ecs_checksum(
    SyntheticSocioeconomicLoad const& value,
    std::uint64_t seed
) {
    seed = OpenVic::ecs::fold_uint64(value.population, seed);
    seed = OpenVic::ecs::fold_uint64(value.physical_stock, seed);
    seed = OpenVic::ecs::fold_uint64(value.productive_capacity, seed);
    seed = OpenVic::ecs::fold_uint64(value.owner, seed);
    seed = OpenVic::ecs::fold_uint64(value.dirty, seed);
    return seed;
}

inline std::uint64_t ecs_checksum(
    SyntheticFormationLoad const& value,
    std::uint64_t seed
) {
    seed = OpenVic::ecs::fold_uint64(value.sustainment_stock, seed);
    seed = OpenVic::ecs::fold_uint64(value.owner, seed);
    seed = OpenVic::ecs::fold_uint64(value.location, seed);
    seed = OpenVic::ecs::fold_uint64(value.active, seed);
    return seed;
}
ECS_COMPONENT(SyntheticActorLoad, "006A2::SyntheticActorLoad")
ECS_COMPONENT(SyntheticNetworkPointLoad, "006A2::SyntheticNetworkPointLoad")
ECS_COMPONENT(SyntheticSocioeconomicLoad, "006A2::SyntheticSocioeconomicLoad")
ECS_COMPONENT(SyntheticFormationLoad, "006A2::SyntheticFormationLoad")

namespace {

enum class Tier : std::uint8_t { Smoke, Regional, Large, World };
enum class ActivityProfile : std::uint8_t { Quiet, Ordinary, Crisis };

struct Workload {
	std::size_t actors;
	std::size_t network_points;
	std::size_t socioeconomic_records;
	std::size_t formations;
};

struct ActivityRates {
	std::uint32_t formation_per_10000;
	std::uint32_t network_dirty_per_10000;
	std::uint32_t socioeconomic_dirty_per_10000;
};

constexpr Workload workload_for(Tier tier) {
	switch (tier) {
	case Tier::Smoke:
		return { 10, 1'000, 10'000, 500 };
	case Tier::Regional:
		return { 40, 10'000, 100'000, 5'000 };
	case Tier::Large:
		return { 100, 50'000, 500'000, 25'000 };
	case Tier::World:
		return { 200, 100'000, 1'000'000, 50'000 };
	}
	return {};
}

constexpr ActivityRates rates_for(ActivityProfile profile) {
	switch (profile) {
	case ActivityProfile::Quiet:
		return { 100, 200, 100 }; // 1%, 2%, 1%
	case ActivityProfile::Ordinary:
		return { 1'000, 500, 500 }; // 10%, 5%, 5%
	case ActivityProfile::Crisis:
		return { 3'500, 2'000, 1'500 }; // 35%, 20%, 15%
	}
	return {};
}

constexpr std::uint64_t mix64(std::uint64_t x) {
	x += 0x9e3779b97f4a7c15ULL;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

bool selected(std::uint64_t seed, std::uint64_t index, std::uint32_t per_10000) {
	return (mix64(seed ^ index) % 10'000ULL) < per_10000;
}

std::uint64_t fnv1a(std::uint64_t hash, std::uint64_t value) {
	constexpr std::uint64_t prime = 1099511628211ULL;
	for (int i = 0; i < 8; ++i) {
		hash ^= (value >> (i * 8)) & 0xffULL;
		hash *= prime;
	}
	return hash;
}

std::size_t working_set_bytes() {
#if defined(_WIN32)
	PROCESS_MEMORY_COUNTERS_EX counters {};
	counters.cb = sizeof(counters);
	if (GetProcessMemoryInfo(
			GetCurrentProcess(),
			reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
			sizeof(counters)
		)) {
		return static_cast<std::size_t>(counters.WorkingSetSize);
	}
	return 0;
#elif defined(__linux__)
	long pages = 0;
	FILE* f = std::fopen("/proc/self/statm", "r");
	if (f != nullptr) {
		long ignored = 0;
		if (std::fscanf(f, "%ld %ld", &ignored, &pages) != 2) {
			pages = 0;
		}
		std::fclose(f);
	}
	return pages > 0
		? static_cast<std::size_t>(pages) * static_cast<std::size_t>(::sysconf(_SC_PAGESIZE))
		: 0;
#else
	return 0;
#endif
}

Tier parse_tier(std::string_view value) {
	if (value == "0" || value == "smoke") return Tier::Smoke;
	if (value == "1" || value == "regional") return Tier::Regional;
	if (value == "2" || value == "large") return Tier::Large;
	if (value == "3" || value == "world") return Tier::World;
	std::cerr << "unknown tier: " << value << "\n";
	std::exit(2);
}

ActivityProfile parse_profile(std::string_view value) {
	if (value == "quiet") return ActivityProfile::Quiet;
	if (value == "ordinary") return ActivityProfile::Ordinary;
	if (value == "crisis") return ActivityProfile::Crisis;
	std::cerr << "unknown profile: " << value << "\n";
	std::exit(2);
}

char const* tier_name(Tier tier) {
	switch (tier) {
	case Tier::Smoke: return "smoke";
	case Tier::Regional: return "regional";
	case Tier::Large: return "large";
	case Tier::World: return "world";
	}
	return "unknown";
}

char const* profile_name(ActivityProfile profile) {
	switch (profile) {
	case ActivityProfile::Quiet: return "quiet";
	case ActivityProfile::Ordinary: return "ordinary";
	case ActivityProfile::Crisis: return "crisis";
	}
	return "unknown";
}

template<typename Component, typename Builder>
std::vector<EntityID> create_batch(
	World& world,
	std::size_t count,
	Builder&& builder
) {
	std::vector<Component> data;
	std::vector<EntityID> ids(count);
	data.reserve(count);

	for (std::size_t i = 0; i < count; ++i) {
		data.push_back(builder(i));
	}

	if (!world.create_entities<Component>(
		count,
		std::span<EntityID> { ids },
		std::span<Component> { data }
	)) {
		std::cerr << "bulk ECS creation failed\n";
		std::exit(3);
	}

	return ids;
}

struct Counters {
	std::uint64_t records_present = 0;
	std::uint64_t records_scanned = 0;
	std::uint64_t records_changed = 0;
	std::uint64_t actor_records = 0;
	std::uint64_t network_records = 0;
	std::uint64_t socioeconomic_records = 0;
	std::uint64_t formation_records = 0;
};

std::uint64_t run_sparse_work(World& world, Counters& counters) {
	std::uint64_t hash = 1469598103934665603ULL;

	world.for_each<SyntheticNetworkPointLoad>([&](SyntheticNetworkPointLoad& point) {
		++counters.records_scanned;
		if (point.dirty != 0) {
			point.capacity += 1;
			++counters.records_changed;
		}
		hash = fnv1a(hash, point.capacity);
		hash = fnv1a(hash, point.owner);
	});

	world.for_each<SyntheticSocioeconomicLoad>([&](SyntheticSocioeconomicLoad& record) {
		++counters.records_scanned;
		if (record.dirty != 0) {
			const std::uint64_t produced = (record.productive_capacity / 128ULL) + 1ULL;
			record.physical_stock += produced;
			++counters.records_changed;
		}
		hash = fnv1a(hash, record.population);
		hash = fnv1a(hash, record.physical_stock);
	});

	world.for_each<SyntheticFormationLoad>([&](SyntheticFormationLoad& formation) {
		++counters.records_scanned;
		if (formation.active != 0 && formation.sustainment_stock > 0) {
			--formation.sustainment_stock;
			++counters.records_changed;
		}
		hash = fnv1a(hash, formation.sustainment_stock);
		hash = fnv1a(hash, formation.location);
	});

	return hash;
}

} // namespace

int main(int argc, char** argv) {
	Tier tier = Tier::Smoke;
	ActivityProfile profile = ActivityProfile::Ordinary;
	std::uint64_t seed = 0x006A2ULL;

	for (int i = 1; i < argc; ++i) {
		const std::string_view arg { argv[i] };
		if (arg == "--tier" && i + 1 < argc) {
			tier = parse_tier(argv[++i]);
		} else if (arg == "--profile" && i + 1 < argc) {
			profile = parse_profile(argv[++i]);
		} else if (arg == "--seed" && i + 1 < argc) {
			seed = std::strtoull(argv[++i], nullptr, 10);
		} else if (arg == "--help") {
			std::cout
				<< "usage: openvic-simulation.world-scale "
				   "[--tier smoke|regional|large|world] "
				   "[--profile quiet|ordinary|crisis] "
				   "[--seed N]\n";
			return 0;
		} else {
			std::cerr << "unknown argument: " << arg << "\n";
			return 2;
		}
	}

	const Workload workload = workload_for(tier);
	const ActivityRates rates = rates_for(profile);

	const std::size_t memory_before = working_set_bytes();
	const auto create_start = std::chrono::steady_clock::now();

	World world;

	auto actor_ids = create_batch<SyntheticActorLoad>(
		world, workload.actors,
		[&](std::size_t i) {
			return SyntheticActorLoad {
				.treasury = 1'000'000ULL + mix64(seed ^ i) % 10'000'000ULL,
				.authority_scope = static_cast<std::uint32_t>(i),
				.flags = 0
			};
		}
	);

	auto network_ids = create_batch<SyntheticNetworkPointLoad>(
		world, workload.network_points,
		[&](std::size_t i) {
			return SyntheticNetworkPointLoad {
				.capacity = 1'000ULL + mix64(seed ^ (0x10000000ULL + i)) % 100'000ULL,
				.owner = static_cast<std::uint32_t>(i % std::max<std::size_t>(1, workload.actors)),
				.neighbor_count = static_cast<std::uint32_t>(2 + (i % 7)),
				.dirty = static_cast<std::uint8_t>(
					selected(seed ^ 0xA11CEULL, i, rates.network_dirty_per_10000)
				)
			};
		}
	);

	auto socioeconomic_ids = create_batch<SyntheticSocioeconomicLoad>(
		world, workload.socioeconomic_records,
		[&](std::size_t i) {
			return SyntheticSocioeconomicLoad {
				.population = 50ULL + mix64(seed ^ (0x20000000ULL + i)) % 5'000ULL,
				.physical_stock = 100ULL + mix64(seed ^ (0x21000000ULL + i)) % 50'000ULL,
				.productive_capacity = 100ULL + mix64(seed ^ (0x22000000ULL + i)) % 100'000ULL,
				.owner = static_cast<std::uint32_t>(i % std::max<std::size_t>(1, workload.actors)),
				.dirty = static_cast<std::uint8_t>(
					selected(seed ^ 0xEC0ULL, i, rates.socioeconomic_dirty_per_10000)
				)
			};
		}
	);

	auto formation_ids = create_batch<SyntheticFormationLoad>(
		world, workload.formations,
		[&](std::size_t i) {
			return SyntheticFormationLoad {
				.sustainment_stock = 100ULL + mix64(seed ^ (0x30000000ULL + i)) % 10'000ULL,
				.owner = static_cast<std::uint32_t>(i % std::max<std::size_t>(1, workload.actors)),
				.location = static_cast<std::uint32_t>(i % std::max<std::size_t>(1, workload.network_points)),
				.active = static_cast<std::uint8_t>(
					selected(seed ^ 0xF04AULL, i, rates.formation_per_10000)
				)
			};
		}
	);

	const auto create_end = std::chrono::steady_clock::now();
	const std::size_t memory_after_create = working_set_bytes();

	Counters counters {
		.records_present = static_cast<std::uint64_t>(
			workload.actors + workload.network_points
			+ workload.socioeconomic_records + workload.formations
		),
		.actor_records = workload.actors,
		.network_records = workload.network_points,
		.socioeconomic_records = workload.socioeconomic_records,
		.formation_records = workload.formations
	};

	const auto work_start = std::chrono::steady_clock::now();
	const std::uint64_t hash = run_sparse_work(world, counters);
	const auto work_end = std::chrono::steady_clock::now();

	const std::size_t memory_after_work = working_set_bytes();

	const auto creation_ms =
		std::chrono::duration<double, std::milli>(create_end - create_start).count();
	const auto work_ms =
		std::chrono::duration<double, std::milli>(work_end - work_start).count();

	// Keep ids observably live and validate the expected count without relying on optimizer behavior.
	const std::uint64_t id_count =
		static_cast<std::uint64_t>(
			actor_ids.size() + network_ids.size()
			+ socioeconomic_ids.size() + formation_ids.size()
		);

	std::cout
		<< "{\n"
		<< "  \"increment\": \"PROJECT-CONVERGENCE-006A2.1\",\n"
		<< "  \"scope\": \"ecs-substrate-surrogate-baseline\",\n"
		<< "  \"tier\": \"" << tier_name(tier) << "\",\n"
		<< "  \"profile\": \"" << profile_name(profile) << "\",\n"
		<< "  \"seed\": " << seed << ",\n"
		<< "  \"counts\": {\n"
		<< "    \"actors\": " << workload.actors << ",\n"
		<< "    \"network_points\": " << workload.network_points << ",\n"
		<< "    \"socioeconomic_records\": " << workload.socioeconomic_records << ",\n"
		<< "    \"formations\": " << workload.formations << ",\n"
		<< "    \"entity_ids\": " << id_count << "\n"
		<< "  },\n"
		<< "  \"work\": {\n"
		<< "    \"records_present\": " << counters.records_present << ",\n"
		<< "    \"records_scanned\": " << counters.records_scanned << ",\n"
		<< "    \"records_changed\": " << counters.records_changed << "\n"
		<< "  },\n"
		<< "  \"timing_ms\": {\n"
		<< "    \"create\": " << creation_ms << ",\n"
		<< "    \"sparse_work\": " << work_ms << "\n"
		<< "  },\n"
		<< "  \"memory_bytes\": {\n"
		<< "    \"before\": " << memory_before << ",\n"
		<< "    \"after_create\": " << memory_after_create << ",\n"
		<< "    \"after_work\": " << memory_after_work << "\n"
		<< "  },\n"
		<< "  \"deterministic_hash\": " << hash << ",\n"
		<< "  \"limitations\": [\n"
		<< "    \"006A2.1 uses real OpenVic ECS World storage but benchmark-only surrogate components\",\n"
		<< "    \"it does not certify economy, geography, population, logistics or military domain scale\",\n"
		<< "    \"later 006A2 slices must replace surrogates with authoritative domain objects and phase instrumentation\"\n"
		<< "  ]\n"
		<< "}\n";

	return 0;
}
