#include "TransportExecution.hpp"

#include <algorithm>

using namespace OpenVic;

fixed_point_t
TransportExecutionResource::
calculate_effective_capacity() const {
    if (
        !enabled ||
        nominal_capacity <= fixed_point_t::_0 ||
        availability_fraction <= fixed_point_t::_0
    ) {
        return fixed_point_t::_0;
    }

    fixed_point_t const bounded =
        std::clamp(
            availability_fraction,
            fixed_point_t::_0,
            fixed_point_t::_1
        );

    return
        nominal_capacity * bounded;
}

bool
TransportExecutionState::
configure_resources(
    std::vector<TransportExecutionResource> new_resources
) {
    for (auto const& resource : new_resources) {
        if (
            resource.resource_id.empty() ||
            resource.nominal_capacity <= fixed_point_t::_0 ||
            resource.availability_fraction < fixed_point_t::_0 ||
            resource.availability_fraction > fixed_point_t::_1
        ) {
            return false;
        }
    }

    std::ranges::sort(
        new_resources,
        [](auto const& lhs, auto const& rhs) {
            return lhs.resource_id < rhs.resource_id;
        }
    );

    for (size_t i = 1; i < new_resources.size(); ++i) {
        if (
            new_resources[i - 1].resource_id ==
            new_resources[i].resource_id
        ) {
            return false;
        }
    }

    resources = std::move(new_resources);
    return true;
}

TransportExecutionReservation const*
TransportExecutionState::
get_reservation_by_unique_id(
    unique_id_t reservation_unique_id
) const {
    auto const it = std::find_if(
        reservations.begin(),
        reservations.end(),
        [reservation_unique_id](auto const& reservation) {
            return
                reservation.reservation_unique_id ==
                reservation_unique_id;
        }
    );

    return
        it != reservations.end()
            ? &*it
            : nullptr;
}

fixed_point_t
TransportExecutionState::
get_effective_capacity(
    std::string_view resource_id
) const {
    auto const it = std::find_if(
        resources.begin(),
        resources.end(),
        [resource_id](auto const& resource) {
            return resource.resource_id == resource_id;
        }
    );

    return
        it != resources.end()
            ? it->calculate_effective_capacity()
            : fixed_point_t::_0;
}

fixed_point_t
TransportExecutionState::
get_reserved_capacity(
    std::string_view resource_id
) const {
    fixed_point_t total = fixed_point_t::_0;

    for (auto const& reservation : reservations) {
        if (!reservation.active) {
            continue;
        }

        for (auto const& requirement : reservation.requirements) {
            if (requirement.resource_id == resource_id) {
                total += requirement.required_capacity;
            }
        }
    }

    return total;
}

fixed_point_t
TransportExecutionState::
get_available_capacity(
    std::string_view resource_id
) const {
    return std::max(
        fixed_point_t::_0,
        get_effective_capacity(resource_id) -
            get_reserved_capacity(resource_id)
    );
}

bool
TransportExecutionState::
set_resource_availability(
    std::string_view resource_id,
    fixed_point_t availability_fraction
) {
    if (
        availability_fraction < fixed_point_t::_0 ||
        availability_fraction > fixed_point_t::_1
    ) {
        return false;
    }

    for (auto& resource : resources) {
        if (resource.resource_id == resource_id) {
            resource.availability_fraction = availability_fraction;
            return true;
        }
    }

    return false;
}

bool
TransportExecutionState::
set_resource_enabled(
    std::string_view resource_id,
    bool enabled
) {
    for (auto& resource : resources) {
        if (resource.resource_id == resource_id) {
            resource.enabled = enabled;
            return true;
        }
    }

    return false;
}

bool
TransportExecutionState::
can_reserve(
    std::span<TransportExecutionRequirement const> requirements,
    Timespan occupation_time
) const {
    if (
        requirements.empty() ||
        occupation_time <= Timespan { 0 }
    ) {
        return false;
    }

    std::vector<TransportExecutionRequirement> ordered {
        requirements.begin(),
        requirements.end()
    };

    std::ranges::sort(
        ordered,
        [](auto const& lhs, auto const& rhs) {
            return lhs.resource_id < rhs.resource_id;
        }
    );

    for (size_t i = 0; i < ordered.size(); ++i) {
        auto const& requirement = ordered[i];

        if (
            requirement.resource_id.empty() ||
            requirement.required_capacity <= fixed_point_t::_0
        ) {
            return false;
        }

        if (
            i > 0 &&
            ordered[i - 1].resource_id == requirement.resource_id
        ) {
            return false;
        }

        auto const resource = std::find_if(
            resources.begin(),
            resources.end(),
            [&requirement](auto const& candidate) {
                return
                    candidate.resource_id ==
                    requirement.resource_id;
            }
        );

        if (resource == resources.end()) {
            return false;
        }

        if (
            get_available_capacity(requirement.resource_id) <
            requirement.required_capacity
        ) {
            return false;
        }
    }

    return true;
}

bool
TransportExecutionState::
reserve_planned(
    std::span<TransportExecutionRequirement const> requirements,
    Date start_date,
    Timespan occupation_time,
    unique_id_t* created_reservation_unique_id
) {
    if (!can_reserve(requirements, occupation_time)) {
        return false;
    }

    std::vector<TransportExecutionRequirement> ordered {
        requirements.begin(),
        requirements.end()
    };

    std::ranges::sort(
        ordered,
        [](auto const& lhs, auto const& rhs) {
            return lhs.resource_id < rhs.resource_id;
        }
    );

    unique_id_t const reservation_id =
        next_reservation_unique_id++;

    reservations.push_back(
        TransportExecutionReservation {
            .reservation_unique_id = reservation_id,
            .shipment_unique_id = 0,
            .start_date = start_date,
            .release_date = start_date + occupation_time,
            .requirements = std::move(ordered),
            .active = true
        }
    );

    if (created_reservation_unique_id != nullptr) {
        *created_reservation_unique_id = reservation_id;
    }

    return true;
}

bool
TransportExecutionState::
bind_reservation_to_shipment(
    unique_id_t reservation_unique_id,
    unique_id_t shipment_unique_id
) {
    if (shipment_unique_id == 0) {
        return false;
    }

    auto const it = std::find_if(
        reservations.begin(),
        reservations.end(),
        [reservation_unique_id](auto const& reservation) {
            return
                reservation.reservation_unique_id ==
                reservation_unique_id;
        }
    );

    if (
        it == reservations.end() ||
        !it->active ||
        it->shipment_unique_id != 0
    ) {
        return false;
    }

    it->shipment_unique_id = shipment_unique_id;
    return true;
}

bool
TransportExecutionState::
cancel_reservation(
    unique_id_t reservation_unique_id
) {
    auto const it = std::find_if(
        reservations.begin(),
        reservations.end(),
        [reservation_unique_id](auto const& reservation) {
            return
                reservation.reservation_unique_id ==
                reservation_unique_id;
        }
    );

    if (
        it == reservations.end() ||
        !it->active
    ) {
        return false;
    }

    it->active = false;
    return true;
}

bool
TransportExecutionState::
reserve(
    unique_id_t shipment_unique_id,
    std::span<TransportExecutionRequirement const> requirements,
    Date start_date,
    Timespan occupation_time,
    unique_id_t* created_reservation_unique_id
) {
    if (
        shipment_unique_id == 0 ||
        !can_reserve(requirements, occupation_time)
    ) {
        return false;
    }

    std::vector<TransportExecutionRequirement> ordered {
        requirements.begin(),
        requirements.end()
    };

    std::ranges::sort(
        ordered,
        [](auto const& lhs, auto const& rhs) {
            return lhs.resource_id < rhs.resource_id;
        }
    );

    unique_id_t const reservation_id =
        next_reservation_unique_id++;

    reservations.push_back(
        TransportExecutionReservation {
            .reservation_unique_id = reservation_id,
            .shipment_unique_id = shipment_unique_id,
            .start_date = start_date,
            .release_date = start_date + occupation_time,
            .requirements = std::move(ordered),
            .active = true
        }
    );

    if (created_reservation_unique_id != nullptr) {
        *created_reservation_unique_id = reservation_id;
    }

    return true;
}

void
TransportExecutionState::
advance_to(Date current_date) {
    for (auto& reservation : reservations) {
        if (
            reservation.active &&
            current_date >= reservation.release_date
        ) {
            reservation.active = false;
        }
    }
}
