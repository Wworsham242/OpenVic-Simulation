#include "TransportDemand.hpp"

#include <algorithm>

using namespace OpenVic;

bool
TransportDemandDeriver::
validate_profile(
    TransportDemandProfile const& profile
) {
    if (
        profile.profile_id.empty() ||
        profile.factors.empty() ||
        profile.minimum_occupation_time <= Timespan { 0 }
    ) {
        return false;
    }

    std::vector<std::string> ids;
    ids.reserve(profile.factors.size());

    for (auto const& factor : profile.factors) {
        if (
            factor.resource_id.empty() ||
            factor.fixed_capacity < fixed_point_t::_0 ||
            factor.capacity_per_quantity < fixed_point_t::_0 ||
            (
                factor.fixed_capacity == fixed_point_t::_0 &&
                factor.capacity_per_quantity == fixed_point_t::_0
            )
        ) {
            return false;
        }

        ids.push_back(factor.resource_id);
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
TransportDemandDeriver::
derive(
    TransportDemandProfile const& profile,
    fixed_point_t physical_quantity,
    Timespan route_occupation_time,
    TransportDemandResult& result
) {
    result = {};

    if (
        !validate_profile(profile) ||
        physical_quantity <= fixed_point_t::_0 ||
        route_occupation_time <= Timespan { 0 }
    ) {
        return false;
    }

    result.requirements.reserve(profile.factors.size());

    for (auto const& factor : profile.factors) {
        fixed_point_t const requirement =
            factor.fixed_capacity +
            factor.capacity_per_quantity * physical_quantity;

        if (requirement <= fixed_point_t::_0) {
            continue;
        }

        result.requirements.push_back(
            TransportExecutionRequirement {
                .resource_id = factor.resource_id,
                .required_capacity = requirement
            }
        );
    }

    if (result.requirements.empty()) {
        return false;
    }

    std::ranges::sort(
        result.requirements,
        [](auto const& lhs, auto const& rhs) {
            return lhs.resource_id < rhs.resource_id;
        }
    );

    result.occupation_time =
        std::max(
            route_occupation_time,
            profile.minimum_occupation_time
        );

    return true;
}
