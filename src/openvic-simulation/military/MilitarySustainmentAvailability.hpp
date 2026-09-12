#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/military/MilitarySustainmentConsumption.hpp"

namespace OpenVic {

/*
 * Derived fulfillment signal for one sustainment item.
 *
 * This is not stock, readiness, combat power, or a modifier.
 * It records how much of physical sustainment demand was met.
 */
struct MilitarySustainmentAvailabilityEntry {
    std::string item_id;

    fixed_point_t requested_quantity = fixed_point_t::_0;
    fixed_point_t consumed_quantity = fixed_point_t::_0;

    fixed_point_t fulfillment_fraction = fixed_point_t::_1;
};

class MilitarySustainmentAvailabilityResult final {
private:
    std::vector<MilitarySustainmentAvailabilityEntry> entries;

    fixed_point_t limiting_fraction = fixed_point_t::_1;

public:
    [[nodiscard]]
    std::span<MilitarySustainmentAvailabilityEntry const>
    get_entries() const {
        return entries;
    }

    [[nodiscard]]
    fixed_point_t get_fulfillment_fraction(
        std::string_view item_id
    ) const;

    /*
     * Conservative diagnostic signal.
     *
     * This is the minimum fulfillment fraction among resources
     * with positive demand.
     *
     * It is NOT automatically applied to readiness or capability.
     */
    [[nodiscard]]
    fixed_point_t get_limiting_fraction() const {
        return limiting_fraction;
    }

    void clear() {
        entries.clear();
        limiting_fraction = fixed_point_t::_1;
    }

private:
    friend class MilitarySustainmentAvailabilityDeriver;

    void add_entry(
        std::string_view item_id,
        fixed_point_t requested_quantity,
        fixed_point_t consumed_quantity,
        fixed_point_t fulfillment_fraction
    );

    void set_limiting_fraction(
        fixed_point_t new_limiting_fraction
    ) {
        limiting_fraction = new_limiting_fraction;
    }
};

class MilitarySustainmentAvailabilityDeriver final {
public:
    /*
     * Derive availability from a completed physical
     * consumption result.
     *
     * For positive demand:
     *
     * fulfillment = consumed / requested
     *
     * bounded to [0,1].
     *
     * Zero-demand resources are fully satisfied by definition
     * and do not constrain the limiting fraction.
     */
    static bool derive(
        MilitarySustainmentConsumptionResult const& consumption,
        MilitarySustainmentAvailabilityResult& result
    );
};

}
