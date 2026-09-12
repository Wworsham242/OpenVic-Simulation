#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/*
 * One generic scarce logistics-execution resource pool.
 *
 * Examples may include truck lift, drivers, rail capacity,
 * aircraft lift, sealift, handling teams, cranes, or other
 * scenario-defined transport capabilities.
 *
 * The core does not assign semantic meaning to resource_id.
 */
struct TransportExecutionResource {
    std::string resource_id;
    fixed_point_t nominal_capacity = 0;
    fixed_point_t availability_fraction = fixed_point_t::_1;
    bool enabled = true;

    [[nodiscard]] fixed_point_t calculate_effective_capacity() const;
};

struct TransportExecutionRequirement {
    std::string resource_id;
    fixed_point_t required_capacity = 0;
};

/*
 * Persistent claim on execution resources for the duration
 * of a physical movement.
 *
 * shipment_unique_id references generic logistics shipment
 * state. The shipment does not own the transport resource.
 */
struct TransportExecutionReservation {
    unique_id_t reservation_unique_id = 0;
    unique_id_t shipment_unique_id = 0;

    Date start_date {};
    Date release_date {};

    std::vector<TransportExecutionRequirement> requirements;

    bool active = true;
};

class TransportExecutionState final {
private:
    std::vector<TransportExecutionResource> resources;
    std::vector<TransportExecutionReservation> reservations;

    unique_id_t next_reservation_unique_id = 1;

public:
    [[nodiscard]]
    bool configure_resources(
        std::vector<TransportExecutionResource> new_resources
    );

    [[nodiscard]]
    std::span<TransportExecutionResource const>
    get_resources() const {
        return resources;
    }

    [[nodiscard]]
    std::span<TransportExecutionReservation const>
    get_reservations() const {
        return reservations;
    }

    [[nodiscard]]
    TransportExecutionReservation const*
    get_reservation_by_unique_id(
        unique_id_t reservation_unique_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_effective_capacity(
        std::string_view resource_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_reserved_capacity(
        std::string_view resource_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_available_capacity(
        std::string_view resource_id
    ) const;

    [[nodiscard]]
    bool set_resource_availability(
        std::string_view resource_id,
        fixed_point_t availability_fraction
    );

    [[nodiscard]]
    bool set_resource_enabled(
        std::string_view resource_id,
        bool enabled
    );

    [[nodiscard]]
    bool can_reserve(
        std::span<TransportExecutionRequirement const> requirements,
        Timespan occupation_time
    ) const;

    /*
     * Reserve execution capacity before a physical shipment exists.
     *
     * The reservation is created with shipment_unique_id == 0 and
     * must later be bound to a committed shipment or cancelled.
     */
    [[nodiscard]]
    bool reserve_planned(
        std::span<TransportExecutionRequirement const> requirements,
        Date start_date,
        Timespan occupation_time,
        unique_id_t* created_reservation_unique_id = nullptr
    );

    [[nodiscard]]
    bool bind_reservation_to_shipment(
        unique_id_t reservation_unique_id,
        unique_id_t shipment_unique_id
    );

    [[nodiscard]]
    bool cancel_reservation(
        unique_id_t reservation_unique_id
    );

    /*
     * Atomically reserve all required execution resources.
     *
     * If any resource is unknown, invalid, duplicated, or
     * insufficient, nothing is reserved.
     */
    [[nodiscard]]
    bool reserve(
        unique_id_t shipment_unique_id,
        std::span<TransportExecutionRequirement const> requirements,
        Date start_date,
        Timespan occupation_time,
        unique_id_t* created_reservation_unique_id = nullptr
    );

    /*
     * Release reservations whose occupation interval has ended.
     */
    void advance_to(Date current_date);
};

}
