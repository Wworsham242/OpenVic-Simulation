#pragma once

#include <string>
#include <vector>

#include "openvic-simulation/economy/trading/TransportExecution.hpp"

namespace OpenVic {

/*
 * One aggregate resource-demand rule.
 *
 * fixed_capacity is paid once by the movement batch.
 * capacity_per_quantity scales with physical cargo quantity.
 *
 * Neither field implies individual vehicles or personnel.
 */
struct TransportDemandFactor {
    std::string resource_id;

    fixed_point_t fixed_capacity = fixed_point_t::_0;
    fixed_point_t capacity_per_quantity = fixed_point_t::_0;
};

/*
 * Shared data-defined movement profile.
 *
 * Examples might represent road freight, rail freight,
 * sealift, airlift, animal transport, pipeline support,
 * or scenario-specific movement systems.
 *
 * profile_id is opaque to core.
 */
struct TransportDemandProfile {
    std::string profile_id;

    std::vector<TransportDemandFactor> factors;

    /*
     * Lower bound on how long the execution resources remain
     * committed even when route-derived occupation is shorter.
     *
     * This may represent minimum turnaround, loading, recovery,
     * staging, crew-cycle, or other aggregate commitment time.
     */
    Timespan minimum_occupation_time { 1 };
};

struct TransportDemandResult {
    std::vector<TransportExecutionRequirement> requirements;

    Timespan occupation_time {};
};

class TransportDemandDeriver final {
public:
    [[nodiscard]]
    static bool validate_profile(
        TransportDemandProfile const& profile
    );

    /*
     * Derive aggregate execution demand for one movement batch.
     *
     * route_occupation_time is supplied by a higher-level
     * route/mode/time model. This function deliberately does not
     * enumerate vehicles or route segments.
     */
    [[nodiscard]]
    static bool derive(
        TransportDemandProfile const& profile,
        fixed_point_t physical_quantity,
        Timespan route_occupation_time,
        TransportDemandResult& result
    );
};

}
