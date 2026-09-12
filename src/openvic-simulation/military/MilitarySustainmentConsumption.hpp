#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/military/MilitarySustainmentStock.hpp"
#include "openvic-simulation/types/Date.hpp"

namespace OpenVic {

/*
 * One aggregate daily consumption factor for one
 * data-defined sustainment item.
 *
 * quantity_per_day is expressed in the same aggregate
 * physical units used by MilitarySustainmentStock.
 */
struct MilitarySustainmentConsumptionFactor {
    std::string item_id;
    fixed_point_t quantity_per_day = fixed_point_t::_0;
};

/*
 * Shared activity profile.
 *
 * activity_id is intentionally opaque to core.
 *
 * Examples may include idle, movement, training, combat,
 * flight operations, naval patrol, siege, caravan travel,
 * or scenario-defined activities from any era.
 */
struct MilitarySustainmentConsumptionProfile {
    std::string activity_id;
    std::vector<MilitarySustainmentConsumptionFactor> factors;
};

struct MilitarySustainmentConsumptionResultEntry {
    std::string item_id;

    fixed_point_t requested_quantity = fixed_point_t::_0;
    fixed_point_t consumed_quantity = fixed_point_t::_0;
    fixed_point_t unmet_quantity = fixed_point_t::_0;
};

struct MilitarySustainmentConsumptionResult {
private:
    std::vector<MilitarySustainmentConsumptionResultEntry> entries;

public:
    [[nodiscard]]
    std::span<MilitarySustainmentConsumptionResultEntry const>
    get_entries() const {
        return entries;
    }

    [[nodiscard]]
    fixed_point_t get_requested_quantity(
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_consumed_quantity(
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_unmet_quantity(
        std::string_view item_id
    ) const;

    void clear() {
        entries.clear();
    }

private:
    friend class MilitarySustainmentConsumer;

    void add_entry(
        std::string_view item_id,
        fixed_point_t requested_quantity,
        fixed_point_t consumed_quantity
    );
};

class MilitarySustainmentConsumer final {
public:
    [[nodiscard]]
    static bool validate_profile(
        MilitarySustainmentConsumptionProfile const& profile
    );

    /*
     * Apply one aggregate activity interval.
     *
     * requested = quantity_per_day * elapsed_days
     *
     * Physical stock authority remains
     * MilitarySustainmentStockState.
     *
     * Shortages are reported rather than converted into
     * readiness effects here.
     */
    static bool consume_for_activity(
        unique_id_t formation_unique_id,
        MilitarySustainmentConsumptionProfile const& profile,
        Timespan elapsed_time,
        MilitarySustainmentStockState& stock_state,
        MilitarySustainmentConsumptionResult& result
    );
};

}
