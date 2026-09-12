#include "MilitarySustainmentConsumption.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

using namespace OpenVic;

fixed_point_t
MilitarySustainmentConsumptionResult::
get_requested_quantity(
    std::string_view item_id
) const {
    for (auto const& entry : entries) {
        if (entry.item_id == item_id) {
            return entry.requested_quantity;
        }
    }

    return fixed_point_t::_0;
}

fixed_point_t
MilitarySustainmentConsumptionResult::
get_consumed_quantity(
    std::string_view item_id
) const {
    for (auto const& entry : entries) {
        if (entry.item_id == item_id) {
            return entry.consumed_quantity;
        }
    }

    return fixed_point_t::_0;
}

fixed_point_t
MilitarySustainmentConsumptionResult::
get_unmet_quantity(
    std::string_view item_id
) const {
    for (auto const& entry : entries) {
        if (entry.item_id == item_id) {
            return entry.unmet_quantity;
        }
    }

    return fixed_point_t::_0;
}

void
MilitarySustainmentConsumptionResult::
add_entry(
    std::string_view item_id,
    fixed_point_t requested_quantity,
    fixed_point_t consumed_quantity
) {
    entries.push_back(
        MilitarySustainmentConsumptionResultEntry {
            .item_id = std::string { item_id },
            .requested_quantity = requested_quantity,
            .consumed_quantity = consumed_quantity,
            .unmet_quantity = std::max(
                requested_quantity - consumed_quantity,
                fixed_point_t::_0
            )
        }
    );
}

bool
MilitarySustainmentConsumer::
validate_profile(
    MilitarySustainmentConsumptionProfile const& profile
) {
    if (
        profile.activity_id.empty() ||
        profile.factors.empty()
    ) {
        return false;
    }

    std::vector<std::string> ids;
    ids.reserve(profile.factors.size());

    for (auto const& factor : profile.factors) {
        if (
            factor.item_id.empty() ||
            factor.quantity_per_day < fixed_point_t::_0
        ) {
            return false;
        }

        ids.push_back(factor.item_id);
    }

    std::ranges::sort(ids);

    for (size_t i = 1; i < ids.size(); ++i) {
        if (ids[i - 1] == ids[i]) {
            return false;
        }
    }

    return true;
}

bool
MilitarySustainmentConsumer::
consume_for_activity(
    unique_id_t formation_unique_id,
    MilitarySustainmentConsumptionProfile const& profile,
    Timespan elapsed_time,
    MilitarySustainmentStockState& stock_state,
    MilitarySustainmentConsumptionResult& result
) {
    result.clear();

    if (
        formation_unique_id == 0 ||
        elapsed_time <= Timespan { 0 } ||
        !validate_profile(profile)
    ) {
        return false;
    }

    Timespan::day_t const elapsed_day_count =
        elapsed_time.to_int();

    if (
        elapsed_day_count >
            static_cast<Timespan::day_t>(
                std::numeric_limits<int32_t>::max()
            )
    ) {
        return false;
    }

    fixed_point_t const elapsed_days {
        static_cast<int32_t>(elapsed_day_count)
    };

    for (auto const& factor : profile.factors) {
        fixed_point_t const requested =
            factor.quantity_per_day * elapsed_days;

        if (requested == fixed_point_t::_0) {
            result.add_entry(
                factor.item_id,
                fixed_point_t::_0,
                fixed_point_t::_0
            );

            continue;
        }

        fixed_point_t consumed = fixed_point_t::_0;

        if (
            !stock_state.consume(
                formation_unique_id,
                factor.item_id,
                requested,
                consumed
            )
        ) {
            /*
             * Missing stock authority is not silently treated
             * as successful zero consumption.
             */
            return false;
        }

        result.add_entry(
            factor.item_id,
            requested,
            consumed
        );
    }

    return true;
}
