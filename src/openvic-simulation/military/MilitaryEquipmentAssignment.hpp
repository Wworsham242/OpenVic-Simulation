#pragma once

#include <functional>
#include <span>
#include <string_view>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAllocation.hpp"
#include "openvic-simulation/military/MilitaryEquipmentDelivery.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/*
 * History-bearing equipment committed to one formation.
 *
 * This is authoritative assignment state, not a duplicate copy of
 * unassigned national/depot/economy stock.
 *
 * Allocation moves quantity out of an external stock authority and
 * into assignment state.
 */
struct MilitaryEquipmentAssignment {
    unique_id_t formation_unique_id = 0;
    memory::string item_id;
    fixed_point_t assigned_quantity = 0;
};

struct MilitaryEquipmentAssignmentState {
private:
    memory::vector<
        MilitaryEquipmentAssignment
    > assignments;

    MilitaryEquipmentAssignment*
    find_assignment(
        unique_id_t formation_unique_id,
        std::string_view item_id
    );

    MilitaryEquipmentAssignment const*
    find_assignment(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

public:
    using return_provider_t =
        std::function<
            bool(
                std::string_view,
                fixed_point_t
            )
        >;

    [[nodiscard]]
    std::span<
        MilitaryEquipmentAssignment const
    >
    get_assignments() const {
        return assignments;
    }

    [[nodiscard]]
    fixed_point_t get_assigned_quantity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_total_assigned(
        std::string_view item_id
    ) const;

    /*
     * Remaining declared requirement after persistent assignment.
     *
     * Returns -1 for an invalid formation/item pair.
     */
    [[nodiscard]]
    fixed_point_t get_outstanding_requirement(
        MilitaryFormationInstanceManager const&
            formation_manager,
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    /*
     * Commit a successful allocation result into persistent state.
     *
     * Existing assignments are retained.
     * New allocation adds only to them.
     *
     * A commit that would exceed the formation's declared
     * requirement is rejected before mutation.
     */
    bool apply_allocation_result(
        MilitaryFormationInstanceManager const&
            formation_manager,
        MilitaryEquipmentAllocationResult const&
            allocation_result
    );

    /*
     * Allocate only the outstanding requirement after persistent
     * assignment is considered.
     *
     * Existing assignments remain in place and therefore are not
     * redrawn from external stock.
     */
    bool allocate_replenishment(
        MilitaryFormationInstanceManager const&
            formation_manager,
        std::span<
            MilitaryEquipmentAllocationRequest const
        > requests,
        MilitaryEquipmentAllocator::draw_provider_t const&
            draw_provider,
        MilitaryEquipmentAllocationResult& result
    ) const;

    using delivery_endpoint_provider_t =
        std::function<
            std::optional<
                MilitaryEquipmentDeliveryEndpoint
            >(
                unique_id_t,
                std::string_view
            )
        >;

    /*
     * Replenishment resolved through the existing generic strategic
     * logistics network.
     *
     * Persistent assignment determines real shortfall.
     * External geography/logistics state supplies endpoints.
     * LogisticsGraph determines physically deliverable quantity.
     * Only that quantity reaches the external stock draw.
     */
    bool allocate_network_replenishment(
        MilitaryFormationInstanceManager const&
            formation_manager,
        std::span<
            MilitaryEquipmentAllocationRequest const
        > requests,
        LogisticsGraph const& logistics_graph,
        delivery_endpoint_provider_t const&
            endpoint_provider,
        MilitaryEquipmentAllocator::draw_provider_t const&
            draw_provider,
        MilitaryEquipmentAllocationResult&
            allocation_result,
        MilitaryEquipmentDeliveryResult&
            delivery_result
    ) const;

    /*
     * Return committed equipment to an external stock authority.
     *
     * Persistent assignment is reduced only after the external
     * authority accepts the return.
     */
    bool release(
        unique_id_t formation_unique_id,
        std::string_view item_id,
        fixed_point_t quantity,
        return_provider_t const& return_provider
    );

    /*
     * Move already-assigned equipment between formations.
     *
     * This conserves total assigned quantity and therefore requires
     * no draw/return to external unassigned stock.
     */
    bool transfer(
        MilitaryFormationInstanceManager const&
            formation_manager,
        unique_id_t source_formation_unique_id,
        unique_id_t target_formation_unique_id,
        std::string_view item_id,
        fixed_point_t quantity
    );
};

}
