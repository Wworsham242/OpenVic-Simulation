#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

using namespace OpenVic;

namespace {

struct Tier {
    std::string_view name;
    std::size_t network_points;
};

struct Profile {
    std::string_view name;
    std::uint32_t dirty_percent;
};

Tier parse_tier(std::string_view name) {
    if (name == "smoke") {
        return { "smoke", 1'000 };
    }
    if (name == "regional") {
        return { "regional", 10'000 };
    }
    if (name == "large") {
        return { "large", 50'000 };
    }
    if (name == "world") {
        return { "world", 100'000 };
    }

    std::cerr << "Unknown tier: " << name << "\n";
    std::exit(2);
}

Profile parse_profile(std::string_view name) {
    // 006A2 network dirty-work controls. These are workload controls,
    // not empirical infrastructure failure rates.
    if (name == "quiet") {
        return { "quiet", 2 };
    }
    if (name == "ordinary") {
        return { "ordinary", 5 };
    }
    if (name == "crisis") {
        return { "crisis", 20 };
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

std::size_t permuted_index(std::size_t ordinal, std::size_t count) {
    // 7919 is coprime to all 006A2 network-point tier sizes.
    return (ordinal * 7'919u) % count;
}

std::string primary_edge_id(std::size_t index) {
    return std::string { "p-" } + std::to_string(index);
}

std::string secondary_edge_id(std::size_t index) {
    return std::string { "s-" } + std::to_string(index);
}

market_node_index_t node_index(std::size_t index) {
    return index_from_count<market_node_index_t>(index);
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
                << "Usage: openvic-simulation.world-scale-logistics "
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

    std::uint64_t const memory_before = working_set_bytes();

    std::vector<LogisticsGraphEdge> edges;
    edges.reserve(tier.network_points * 2);

    auto const build_begin = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < tier.network_points; ++i) {
        std::size_t const primary_destination =
            (i + 1) % tier.network_points;
        std::size_t const secondary_destination =
            (i + 17) % tier.network_points;

        edges.push_back(LogisticsGraphEdge {
            .edge_id = primary_edge_id(i),
            .source = node_index(i),
            .destination = node_index(primary_destination),
            .leg = TransportLeg {
                .nominal_capacity = fixed_point_t { 100 },
                .availability_fraction = fixed_point_t::_1,
                .open = true,
                .unit_cost = fixed_point_t { 1 }
            }
        });

        edges.push_back(LogisticsGraphEdge {
            .edge_id = secondary_edge_id(i),
            .source = node_index(i),
            .destination = node_index(secondary_destination),
            .leg = TransportLeg {
                .nominal_capacity = fixed_point_t { 60 },
                .availability_fraction = fixed_point_t::_1,
                .open = true,
                .unit_cost = fixed_point_t { 2 }
            }
        });
    }

    auto const build_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_edge_build = working_set_bytes();

    LogisticsGraph graph;

    auto const configure_begin = std::chrono::steady_clock::now();
    bool const configured = graph.configure(std::move(edges));
    auto const configure_end = std::chrono::steady_clock::now();

    if (!configured) {
        std::cerr << "LogisticsGraph::configure failed\n";
        return 3;
    }

    std::uint64_t const memory_after_configure = working_set_bytes();

    std::size_t const dirty_points =
        (tier.network_points * profile.dirty_percent) / 100;

    std::size_t dirty_updates_applied = 0;

    auto const dirty_begin = std::chrono::steady_clock::now();

    for (std::size_t ordinal = 0; ordinal < dirty_points; ++ordinal) {
        std::size_t const point =
            permuted_index(ordinal, tier.network_points);

        if (graph.set_edge_open(primary_edge_id(point), false)) {
            ++dirty_updates_applied;
        }
    }

    auto const dirty_end = std::chrono::steady_clock::now();
    std::uint64_t const memory_after_dirty = working_set_bytes();

    // Routing is sampled at one route search per 100 dirty points. The
    // dirty-point percentage remains the 006A2 profile control; this bounded
    // sampling prevents the benchmark harness itself from inventing a rule
    // that every dirty point necessarily emits a route request.
    std::size_t const route_searches =
        std::max<std::size_t>(1, dirty_points / 100);

    std::size_t routes_found = 0;
    std::uint64_t route_edge_count = 0;
    std::uint64_t route_hash = FNV_OFFSET;

    auto const route_begin = std::chrono::steady_clock::now();

    for (std::size_t ordinal = 0; ordinal < route_searches; ++ordinal) {
        std::size_t const source =
            permuted_index(ordinal + 13, tier.network_points);
        std::size_t const destination =
            (source + 17) % tier.network_points;

        LogisticsGraphPath const path = graph.find_route(
            node_index(source),
            node_index(destination)
        );

        if (path.found) {
            ++routes_found;
        }

        route_edge_count += path.edge_ids.size();

        route_hash = hash_u64(route_hash, path.found ? 1u : 0u);
        route_hash = hash_u64(
            route_hash,
            static_cast<std::uint64_t>(
                path.bottleneck_capacity.get_raw_value()
            )
        );

        for (std::string const& edge_id : path.edge_ids) {
            route_hash = hash_string(route_hash, edge_id);
        }
    }

    auto const route_end = std::chrono::steady_clock::now();

    // Exercise the existing shared-capacity allocator with a bounded batch.
    // This is deliberately capped because the purpose is to expose the
    // allocator's current cost separately from graph-size routing cost.
    std::size_t const flow_requests =
        std::min<std::size_t>(route_searches, 64);

    std::vector<LogisticsGraphFlowRequest> requests;
    requests.reserve(flow_requests);

    for (std::size_t i = 0; i < flow_requests; ++i) {
        std::size_t const source =
            permuted_index(i + 101, tier.network_points);
        std::size_t const destination =
            (source + 17) % tier.network_points;

        requests.push_back(LogisticsGraphFlowRequest {
            .flow_id = std::string { "flow-" } + std::to_string(i),
            .source = node_index(source),
            .destination = node_index(destination),
            .requested = fixed_point_t { 25 }
        });
    }

    auto const allocation_begin = std::chrono::steady_clock::now();
    std::vector<LogisticsGraphFlowAllocation> const allocations =
        graph.allocate_flows(requests);
    auto const allocation_end = std::chrono::steady_clock::now();

    std::size_t allocations_found = 0;
    std::size_t rerouted_flows = 0;
    std::uint64_t allocated_raw = 0;

    for (LogisticsGraphFlowAllocation const& allocation : allocations) {
        if (allocation.path.found) {
            ++allocations_found;
        }
        if (allocation.rerouted_allocated > fixed_point_t::_0) {
            ++rerouted_flows;
        }

        allocated_raw += static_cast<std::uint64_t>(
            allocation.allocated.get_raw_value()
        );

        route_hash = hash_string(route_hash, allocation.flow_id);
        route_hash = hash_u64(
            route_hash,
            static_cast<std::uint64_t>(
                allocation.allocated.get_raw_value()
            )
        );
        route_hash = hash_u64(
            route_hash,
            static_cast<std::uint64_t>(
                allocation.rerouted_allocated.get_raw_value()
            )
        );
    }

    std::uint64_t const memory_after_work = working_set_bytes();

    std::uint64_t hash = FNV_OFFSET;
    hash = hash_string(hash, tier.name);
    hash = hash_string(hash, profile.name);
    hash = hash_u64(hash, tier.network_points);
    hash = hash_u64(hash, tier.network_points * 2);
    hash = hash_u64(hash, dirty_points);
    hash = hash_u64(hash, dirty_updates_applied);
    hash = hash_u64(hash, route_searches);
    hash = hash_u64(hash, routes_found);
    hash = hash_u64(hash, route_edge_count);
    hash = hash_u64(hash, flow_requests);
    hash = hash_u64(hash, allocations_found);
    hash = hash_u64(hash, rerouted_flows);
    hash = hash_u64(hash, allocated_raw);
    hash = hash_u64(hash, route_hash);

    std::cout
        << "{\n"
        << "  \"increment\": \"PROJECT-CONVERGENCE-006A2.4\",\n"
        << "  \"scope\": \"authoritative-logistics-graph-routing-capacity\",\n"
        << "  \"tier\": \"" << tier.name << "\",\n"
        << "  \"profile\": \"" << profile.name << "\",\n"
        << "  \"counts\": {\n"
        << "    \"network_points\": " << tier.network_points << ",\n"
        << "    \"graph_edges\": " << tier.network_points * 2 << ",\n"
        << "    \"dirty_points\": " << dirty_points << ",\n"
        << "    \"dirty_updates_applied\": "
        << dirty_updates_applied << ",\n"
        << "    \"route_searches\": " << route_searches << ",\n"
        << "    \"routes_found\": " << routes_found << ",\n"
        << "    \"route_edges_returned\": "
        << route_edge_count << ",\n"
        << "    \"flow_requests\": " << flow_requests << ",\n"
        << "    \"allocations_found\": "
        << allocations_found << ",\n"
        << "    \"rerouted_flows\": " << rerouted_flows << "\n"
        << "  },\n"
        << "  \"validation\": {\n"
        << "    \"configured\": true,\n"
        << "    \"all_dirty_updates_applied\": "
        << (dirty_updates_applied == dirty_points ? "true" : "false")
        << ",\n"
        << "    \"all_routes_found\": "
        << (routes_found == route_searches ? "true" : "false")
        << ",\n"
        << "    \"all_allocations_routed\": "
        << (allocations_found == flow_requests ? "true" : "false")
        << ",\n"
        << "    \"allocated_raw\": " << allocated_raw << "\n"
        << "  },\n"
        << "  \"timing_ms\": {\n"
        << "    \"edge_build\": "
        << elapsed_ms(build_begin, build_end) << ",\n"
        << "    \"configure\": "
        << elapsed_ms(configure_begin, configure_end) << ",\n"
        << "    \"dirty_capacity_updates\": "
        << elapsed_ms(dirty_begin, dirty_end) << ",\n"
        << "    \"route_search\": "
        << elapsed_ms(route_begin, route_end) << ",\n"
        << "    \"flow_allocation\": "
        << elapsed_ms(allocation_begin, allocation_end) << "\n"
        << "  },\n"
        << "  \"memory_bytes\": {\n"
        << "    \"before\": " << memory_before << ",\n"
        << "    \"after_edge_build\": "
        << memory_after_edge_build << ",\n"
        << "    \"after_configure\": "
        << memory_after_configure << ",\n"
        << "    \"after_dirty\": "
        << memory_after_dirty << ",\n"
        << "    \"after_work\": "
        << memory_after_work << "\n"
        << "  },\n"
        << "  \"deterministic_hash\": " << hash << ",\n"
        << "  \"limitations\": [\n"
        << "    \"uses real LogisticsGraph edges, capacity mutation, route finding and shared-flow allocation\",\n"
        << "    \"network points are implicit market-node indices because LogisticsGraph currently owns edges rather than node records\",\n"
        << "    \"route searches are sampled at one per 100 dirty points and are a benchmark workload control, not an empirical traffic model\",\n"
        << "    \"flow allocation batch is capped at 64 to separate allocator cost from graph-size routing cost\",\n"
        << "    \"does not certify persistent shipment advancement, transport execution reservation, market delivery or military replenishment\"\n"
        << "  ]\n"
        << "}\n";

    return (
        configured &&
        dirty_updates_applied == dirty_points &&
        routes_found == route_searches &&
        allocations_found == flow_requests
    ) ? 0 : 4;
}