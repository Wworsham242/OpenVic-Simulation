#include "MilitaryEquipmentAssignment.hpp"

#include <algorithm>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MilitaryEquipmentAssignment*
MilitaryEquipmentAssignmentState::
find_assignment(
    unique_id_t formation_unique_id,
    std::string_view item_id
) {
    auto const it =
        std::ranges::find_if(
            assignments,
            [
                formation_unique_id,
                item_id
            ](
                MilitaryEquipmentAssignment const&
                    assignment
            ) {
                return
                    assignment.formation_unique_id ==
                        formation_unique_id &&
                    assignment.item_id ==
                        item_id;
            }
        );

    return
        it == assignments.end()
            ? nullptr
            : &*it;
}

MilitaryEquipmentAssignment const*
MilitaryEquipmentAssignmentState::
find_assignment(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    auto const it =
        std::ranges::find_if(
            assignments,
            [
                formation_unique_id,
                item_id
            ](
                MilitaryEquipmentAssignment const&
                    assignment
            ) {
                return
                    assignment.formation_unique_id ==
                        formation_unique_id &&
                    assignment.item_id ==
                        item_id;
            }
        );

    return
        it == assignments.end()
            ? nullptr
            : &*it;
}

fixed_point_t
MilitaryEquipmentAssignmentState::
get_assigned_quantity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    MilitaryEquipmentAssignment const* const
        assignment =
            find_assignment(
                formation_unique_id,
                item_id
            );

    return
        assignment == nullptr
            ? fixed_point_t::_0
            : assignment->
                assigned_quantity;
}

fixed_point_t
MilitaryEquipmentAssignmentState::
get_total_assigned(
    std::string_view item_id
) const {
    fixed_point_t total =
        fixed_point_t::_0;

    for (
        MilitaryEquipmentAssignment const&
            assignment :
        assignments
    ) {
        if (assignment.item_id == item_id) {
            total +=
                assignment.assigned_quantity;
        }
    }

    return total;
}

fixed_point_t
MilitaryEquipmentAssignmentState::
get_outstanding_requirement(
    MilitaryFormationInstanceManager const&
        formation_manager,
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    MilitaryFormationInstance const* const
        formation =
            formation_manager.
                get_military_formation_instance_by_unique_id(
                    formation_unique_id
                );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot inspect equipment requirement for "
            "unknown military formation {}.",
            formation_unique_id
        );

        return fixed_point_t { -1 };
    }

    auto const requirements =
        formation->
            get_formation_definition().
            get_equipment_requirements();

    auto const it =
        std::ranges::find_if(
            requirements,
            [item_id](
                MilitaryEquipmentRequirement const&
                    requirement
            ) {
                return
                    requirement.item_id ==
                    item_id;
            }
        );

    if (it == requirements.end()) {
        spdlog::error_s(
            "Military formation {} has no declared "
            "equipment requirement for {}.",
            formation_unique_id,
            item_id
        );

        return fixed_point_t { -1 };
    }

    fixed_point_t const assigned =
        get_assigned_quantity(
            formation_unique_id,
            item_id
        );

    if (assigned >= it->required_quantity) {
        return fixed_point_t::_0;
    }

    return
        it->required_quantity -
        assigned;
}

bool
MilitaryEquipmentAssignmentState::
apply_allocation_result(
    MilitaryFormationInstanceManager const&
        formation_manager,
    MilitaryEquipmentAllocationResult const&
        allocation_result
) {
    /*
     * Validate complete result before mutating persistent state.
     */
    for (
        MilitaryEquipmentAllocation const&
            allocation :
        allocation_result.get_allocations()
    ) {
        if (
            allocation.assigned_quantity <
                fixed_point_t::_0
        ) {
            spdlog::error_s(
                "Cannot commit negative equipment "
                "assignment quantity."
            );

            return false;
        }

        fixed_point_t const outstanding =
            get_outstanding_requirement(
                formation_manager,
                allocation.formation_unique_id,
                allocation.item_id
            );

        if (outstanding < fixed_point_t::_0) {
            return false;
        }

        if (
            allocation.assigned_quantity >
            outstanding
        ) {
            spdlog::error_s(
                "Equipment assignment commit would "
                "exceed declared requirement for "
                "formation {} item {}.",
                allocation.formation_unique_id,
                allocation.item_id
            );

            return false;
        }
    }

    for (
        MilitaryEquipmentAllocation const&
            allocation :
        allocation_result.get_allocations()
    ) {
        if (
            allocation.assigned_quantity ==
                fixed_point_t::_0
        ) {
            continue;
        }

        MilitaryEquipmentAssignment* const
            existing =
                find_assignment(
                    allocation.
                        formation_unique_id,
                    allocation.item_id
                );

        if (existing != nullptr) {
            existing->assigned_quantity +=
                allocation.assigned_quantity;

            continue;
        }

        assignments.push_back(
            MilitaryEquipmentAssignment {
                .formation_unique_id =
                    allocation.
                        formation_unique_id,
                .item_id =
                    memory::string {
                        allocation.item_id
                    },
                .assigned_quantity =
                    allocation.
                        assigned_quantity
            }
        );
    }

    return true;
}

bool
MilitaryEquipmentAssignmentState::
allocate_replenishment(
    MilitaryFormationInstanceManager const&
        formation_manager,
    std::span<
        MilitaryEquipmentAllocationRequest const
    > requests,
    MilitaryEquipmentAllocator::draw_provider_t const&
        draw_provider,
    MilitaryEquipmentAllocationResult& result
) const {
    return
        MilitaryEquipmentAllocator::
            allocate_with_quantity_provider(
                formation_manager,
                requests,
                [
                    this,
                    &formation_manager
                ](
                    unique_id_t formation_unique_id,
                    std::string_view item_id,
                    fixed_point_t
                ) {
                    return
                        get_outstanding_requirement(
                            formation_manager,
                            formation_unique_id,
                            item_id
                        );
                },
                draw_provider,
                result
            );
}

bool
MilitaryEquipmentAssignmentState::
allocate_network_replenishment(
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
) const {
    std::vector<
        MilitaryEquipmentDeliveryRequest
    > delivery_requests;

    /*
     * Build physical network demands from real persistent shortfall.
     *
     * This pass performs no stock mutation.
     */
    for (
        MilitaryEquipmentAllocationRequest const&
            allocation_request :
        requests
    ) {
        MilitaryFormationInstance const* const
            formation =
                formation_manager.
                    get_military_formation_instance_by_unique_id(
                        allocation_request.
                            formation_unique_id
                    );

        if (formation == nullptr) {
            spdlog::error_s(
                "Cannot route replenishment for unknown "
                "military formation {}.",
                allocation_request.
                    formation_unique_id
            );

            return false;
        }

        auto const equipment_requirements =
            formation->
                get_formation_definition().
                get_equipment_requirements();

        for (
            MilitaryEquipmentRequirement const&
                requirement :
            equipment_requirements
        ) {
            fixed_point_t const shortfall =
                get_outstanding_requirement(
                    formation_manager,
                    allocation_request.
                        formation_unique_id,
                    requirement.item_id
                );

            if (
                shortfall <
                fixed_point_t::_0
            ) {
                return false;
            }

            if (
                shortfall ==
                fixed_point_t::_0
            ) {
                continue;
            }

            auto const endpoint =
                endpoint_provider(
                    allocation_request.
                        formation_unique_id,
                    requirement.item_id
                );

            if (!endpoint.has_value()) {
                spdlog::error_s(
                    "No logistics endpoints available "
                    "for formation {} item {}.",
                    allocation_request.
                        formation_unique_id,
                    requirement.item_id
                );

                return false;
            }

            delivery_requests.push_back(
                MilitaryEquipmentDeliveryRequest {
                    .formation_unique_id =
                        allocation_request.
                            formation_unique_id,
                    .item_id =
                        std::string {
                            requirement.item_id
                        },
                    .source_node =
                        endpoint->source_node,
                    .destination_node =
                        endpoint->
                            destination_node,
                    .requested_quantity =
                        shortfall
                }
            );
        }
    }

    MilitaryEquipmentDeliveryResult
        candidate_delivery;

    if (
        !MilitaryEquipmentDeliveryResolver::
            resolve(
                logistics_graph,
                delivery_requests,
                candidate_delivery
            )
    ) {
        return false;
    }

    MilitaryEquipmentAllocationResult
        candidate_allocation;

    /*
     * The generic allocator retains stock authority and priority.
     *
     * Network resolution simply caps how much demand is physically
     * capable of reaching the destination during this pass.
     */
    bool const allocated =
        MilitaryEquipmentAllocator::
            allocate_with_quantity_provider(
                formation_manager,
                requests,
                [
                    &candidate_delivery
                ](
                    unique_id_t formation_unique_id,
                    std::string_view item_id,
                    fixed_point_t
                ) {
                    return
                        candidate_delivery.
                            get_deliverable_quantity(
                                formation_unique_id,
                                item_id
                            );
                },
                draw_provider,
                candidate_allocation
            );

    if (!allocated) {
        return false;
    }

    delivery_result =
        std::move(candidate_delivery);

    allocation_result =
        std::move(candidate_allocation);

    return true;
}

bool
MilitaryEquipmentAssignmentState::
release(
    unique_id_t formation_unique_id,
    std::string_view item_id,
    fixed_point_t quantity,
    return_provider_t const& return_provider
) {
    if (quantity <= fixed_point_t::_0) {
        spdlog::error_s(
            "Equipment release quantity must be positive."
        );

        return false;
    }

    MilitaryEquipmentAssignment* const
        assignment =
            find_assignment(
                formation_unique_id,
                item_id
            );

    if (
        assignment == nullptr ||
        quantity >
            assignment->assigned_quantity
    ) {
        spdlog::error_s(
            "Military formation {} cannot release {} "
            "of item {} because insufficient quantity "
            "is assigned.",
            formation_unique_id,
            quantity,
            item_id
        );

        return false;
    }

    /*
     * External stock authority accepts the return first.
     * Only then does persistent assignment change.
     */
    if (
        !return_provider(
            item_id,
            quantity
        )
    ) {
        return false;
    }

    assignment->assigned_quantity -=
        quantity;

    if (
        assignment->assigned_quantity ==
        fixed_point_t::_0
    ) {
        std::erase_if(
            assignments,
            [
                formation_unique_id,
                item_id
            ](
                MilitaryEquipmentAssignment const&
                    candidate
            ) {
                return
                    candidate.
                        formation_unique_id ==
                        formation_unique_id &&
                    candidate.item_id ==
                        item_id;
            }
        );
    }

    return true;
}

bool
MilitaryEquipmentAssignmentState::
transfer(
    MilitaryFormationInstanceManager const&
        formation_manager,
    unique_id_t source_formation_unique_id,
    unique_id_t target_formation_unique_id,
    std::string_view item_id,
    fixed_point_t quantity
) {
    if (
        source_formation_unique_id ==
        target_formation_unique_id
    ) {
        spdlog::error_s(
            "Equipment transfer source and target "
            "formation cannot be identical."
        );

        return false;
    }

    if (quantity <= fixed_point_t::_0) {
        spdlog::error_s(
            "Equipment transfer quantity must be positive."
        );

        return false;
    }

    MilitaryEquipmentAssignment* const
        source =
            find_assignment(
                source_formation_unique_id,
                item_id
            );

    if (
        source == nullptr ||
        source->assigned_quantity <
            quantity
    ) {
        spdlog::error_s(
            "Military formation {} has insufficient "
            "assigned {} for transfer.",
            source_formation_unique_id,
            item_id
        );

        return false;
    }

    fixed_point_t const target_outstanding =
        get_outstanding_requirement(
            formation_manager,
            target_formation_unique_id,
            item_id
        );

    if (target_outstanding < fixed_point_t::_0) {
        return false;
    }

    if (quantity > target_outstanding) {
        spdlog::error_s(
            "Equipment transfer would exceed target "
            "formation {} requirement for {}.",
            target_formation_unique_id,
            item_id
        );

        return false;
    }

    MilitaryEquipmentAssignment* target =
        find_assignment(
            target_formation_unique_id,
            item_id
        );

    /*
     * Mutate only after all validation succeeds.
     */
    source->assigned_quantity -=
        quantity;

    if (target != nullptr) {
        target->assigned_quantity +=
            quantity;
    } else {
        assignments.push_back(
            MilitaryEquipmentAssignment {
                .formation_unique_id =
                    target_formation_unique_id,
                .item_id =
                    memory::string {
                        item_id
                    },
                .assigned_quantity =
                    quantity
            }
        );
    }

    if (
        source->assigned_quantity ==
        fixed_point_t::_0
    ) {
        std::erase_if(
            assignments,
            [
                source_formation_unique_id,
                item_id
            ](
                MilitaryEquipmentAssignment const&
                    candidate
            ) {
                return
                    candidate.
                        formation_unique_id ==
                        source_formation_unique_id &&
                    candidate.item_id ==
                        item_id;
            }
        );
    }

    return true;
}
