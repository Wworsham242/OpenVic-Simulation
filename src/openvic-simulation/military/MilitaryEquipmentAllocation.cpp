#include "MilitaryEquipmentAllocation.hpp"

#include <algorithm>
#include <vector>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

fixed_point_t
MilitaryEquipmentAllocationResult::
get_assigned_quantity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    fixed_point_t total =
        fixed_point_t::_0;

    for (
        MilitaryEquipmentAllocation const&
            allocation :
        allocations
    ) {
        if (
            allocation.formation_unique_id ==
                formation_unique_id &&
            allocation.item_id == item_id
        ) {
            total +=
                allocation.assigned_quantity;
        }
    }

    return total;
}

fixed_point_t
MilitaryEquipmentAllocationResult::
get_unmet_quantity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    fixed_point_t total =
        fixed_point_t::_0;

    for (
        MilitaryEquipmentAllocation const&
            allocation :
        allocations
    ) {
        if (
            allocation.formation_unique_id ==
                formation_unique_id &&
            allocation.item_id == item_id
        ) {
            total +=
                allocation.unmet_quantity;
        }
    }

    return total;
}

fixed_point_t
MilitaryEquipmentAllocationResult::
get_total_assigned(
    std::string_view item_id
) const {
    fixed_point_t total =
        fixed_point_t::_0;

    for (
        MilitaryEquipmentAllocation const&
            allocation :
        allocations
    ) {
        if (allocation.item_id == item_id) {
            total +=
                allocation.assigned_quantity;
        }
    }

    return total;
}

void
MilitaryEquipmentAllocationResult::
add_allocation(
    unique_id_t formation_unique_id,
    std::string_view item_id,
    fixed_point_t requested_quantity,
    fixed_point_t assigned_quantity
) {
    allocations.push_back(
        MilitaryEquipmentAllocation {
            .formation_unique_id =
                formation_unique_id,
            .item_id =
                memory::string {
                    item_id
                },
            .requested_quantity =
                requested_quantity,
            .assigned_quantity =
                assigned_quantity,
            .unmet_quantity =
                requested_quantity -
                assigned_quantity
        }
    );
}

bool MilitaryEquipmentAllocator::allocate(
    MilitaryFormationInstanceManager const&
        formation_manager,
    std::span<
        unique_id_t const
    > formation_unique_ids,
    draw_provider_t const& draw_provider,
    MilitaryEquipmentAllocationResult& result
) {
    std::vector<
        MilitaryEquipmentAllocationRequest
    > requests;

    requests.reserve(
        formation_unique_ids.size()
    );

    for (
        unique_id_t const formation_unique_id :
        formation_unique_ids
    ) {
        requests.push_back(
            MilitaryEquipmentAllocationRequest {
                .formation_unique_id =
                    formation_unique_id,
                .priority = 0
            }
        );
    }

    return allocate(
        formation_manager,
        requests,
        draw_provider,
        result
    );
}

bool MilitaryEquipmentAllocator::allocate(
    MilitaryFormationInstanceManager const&
        formation_manager,
    std::span<
        MilitaryEquipmentAllocationRequest const
    > requests,
    draw_provider_t const& draw_provider,
    MilitaryEquipmentAllocationResult& result
) {
    return allocate_with_quantity_provider(
        formation_manager,
        requests,
        [](
            unique_id_t,
            std::string_view,
            fixed_point_t declared_requirement
        ) {
            return declared_requirement;
        },
        draw_provider,
        result
    );
}

bool
MilitaryEquipmentAllocator::
allocate_with_quantity_provider(
    MilitaryFormationInstanceManager const&
        formation_manager,
    std::span<
        MilitaryEquipmentAllocationRequest const
    > requests,
    request_quantity_provider_t const&
        request_quantity_provider,
    draw_provider_t const& draw_provider,
    MilitaryEquipmentAllocationResult& result
) {
    /*
     * Validate every formation before any external draw.
     */
    std::vector<unique_id_t>
        validation_ids;

    validation_ids.reserve(
        requests.size()
    );

    for (
        MilitaryEquipmentAllocationRequest const&
            request :
        requests
    ) {
        validation_ids.push_back(
            request.formation_unique_id
        );
    }

    std::ranges::sort(
        validation_ids
    );

    if (
        std::adjacent_find(
            validation_ids.begin(),
            validation_ids.end()
        ) != validation_ids.end()
    ) {
        spdlog::error_s(
            "Military equipment allocation request "
            "contains duplicate formation identities."
        );

        return false;
    }

    for (
        unique_id_t const formation_unique_id :
        validation_ids
    ) {
        if (
            formation_manager.
                get_military_formation_instance_by_unique_id(
                    formation_unique_id
                ) == nullptr
        ) {
            spdlog::error_s(
                "Cannot allocate equipment to unknown "
                "military formation {}.",
                formation_unique_id
            );

            return false;
        }
    }

    std::vector<
        MilitaryEquipmentAllocationRequest
    > ordered_requests {
        requests.begin(),
        requests.end()
    };

    std::ranges::sort(
        ordered_requests,
        [](
            MilitaryEquipmentAllocationRequest const&
                lhs,
            MilitaryEquipmentAllocationRequest const&
                rhs
        ) {
            if (lhs.priority != rhs.priority) {
                return
                    lhs.priority >
                    rhs.priority;
            }

            return
                lhs.formation_unique_id <
                rhs.formation_unique_id;
        }
    );

    MilitaryEquipmentAllocationResult candidate;

    for (
        MilitaryEquipmentAllocationRequest const&
            request :
        ordered_requests
    ) {
        unique_id_t const formation_unique_id =
            request.formation_unique_id;

        MilitaryFormationInstance const* const
            formation =
                formation_manager.
                    get_military_formation_instance_by_unique_id(
                        formation_unique_id
                    );

        auto const requirements =
            formation->
                get_formation_definition().
                get_equipment_requirements();

        for (
            MilitaryEquipmentRequirement const&
                requirement :
            requirements
        ) {
            fixed_point_t const
                requested_quantity =
                    request_quantity_provider(
                        formation_unique_id,
                        requirement.item_id,
                        requirement.required_quantity
                    );

            if (
                requested_quantity <
                    fixed_point_t::_0 ||
                requested_quantity >
                    requirement.required_quantity
            ) {
                spdlog::error_s(
                    "Equipment request quantity provider "
                    "returned invalid quantity {} for "
                    "formation {} item {} with declared "
                    "requirement {}.",
                    requested_quantity,
                    formation_unique_id,
                    requirement.item_id,
                    requirement.required_quantity
                );

                return false;
            }

            /*
             * A fully satisfied requirement performs no physical
             * draw. We still record the zero request so the result
             * remains inspectable.
             */
            if (
                requested_quantity ==
                fixed_point_t::_0
            ) {
                candidate.add_allocation(
                    formation_unique_id,
                    requirement.item_id,
                    fixed_point_t::_0,
                    fixed_point_t::_0
                );

                continue;
            }

            fixed_point_t const assigned =
                draw_provider(
                    requirement.item_id,
                    requested_quantity
                );

            if (
                assigned < fixed_point_t::_0 ||
                assigned >
                    requested_quantity
            ) {
                spdlog::error_s(
                    "Equipment draw provider returned "
                    "invalid quantity {} for item {} "
                    "with request {}.",
                    assigned,
                    requirement.item_id,
                    requested_quantity
                );

                return false;
            }

            candidate.add_allocation(
                formation_unique_id,
                requirement.item_id,
                requested_quantity,
                assigned
            );
        }
    }

    result =
        std::move(candidate);

    return true;
}
