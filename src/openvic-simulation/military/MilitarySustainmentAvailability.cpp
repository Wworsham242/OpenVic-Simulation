#include "MilitarySustainmentAvailability.hpp"

#include <algorithm>

using namespace OpenVic;

fixed_point_t
MilitarySustainmentAvailabilityResult::
get_fulfillment_fraction(
    std::string_view item_id
) const {
    for (auto const& entry : entries) {
        if (entry.item_id == item_id) {
            return entry.fulfillment_fraction;
        }
    }

    return fixed_point_t::_0;
}

void
MilitarySustainmentAvailabilityResult::
add_entry(
    std::string_view item_id,
    fixed_point_t requested_quantity,
    fixed_point_t consumed_quantity,
    fixed_point_t fulfillment_fraction
) {
    entries.push_back(
        MilitarySustainmentAvailabilityEntry {
            .item_id = std::string { item_id },
            .requested_quantity = requested_quantity,
            .consumed_quantity = consumed_quantity,
            .fulfillment_fraction = fulfillment_fraction
        }
    );
}

bool
MilitarySustainmentAvailabilityDeriver::
derive(
    MilitarySustainmentConsumptionResult const& consumption,
    MilitarySustainmentAvailabilityResult& result
) {
    result.clear();

    fixed_point_t limiting = fixed_point_t::_1;
    bool has_positive_demand = false;

    for (auto const& entry : consumption.get_entries()) {
        if (
            entry.item_id.empty() ||
            entry.requested_quantity < fixed_point_t::_0 ||
            entry.consumed_quantity < fixed_point_t::_0 ||
            entry.consumed_quantity > entry.requested_quantity
        ) {
            return false;
        }

        fixed_point_t fulfillment = fixed_point_t::_1;

        if (entry.requested_quantity > fixed_point_t::_0) {
            has_positive_demand = true;

            fulfillment = std::clamp(
                entry.consumed_quantity /
                    entry.requested_quantity,
                fixed_point_t::_0,
                fixed_point_t::_1
            );

            limiting = std::min(
                limiting,
                fulfillment
            );
        }

        result.add_entry(
            entry.item_id,
            entry.requested_quantity,
            entry.consumed_quantity,
            fulfillment
        );
    }

    result.set_limiting_fraction(
        has_positive_demand
            ? limiting
            : fixed_point_t::_1
    );

    return true;
}
