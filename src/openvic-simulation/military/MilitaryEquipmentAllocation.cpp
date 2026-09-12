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
    /*
     * Validate the complete request set before drawing anything.
     * A malformed formation list therefore cannot partially consume
     * external stock.
     */
    std::vector<unique_id_t> ordered_ids {
        formation_unique_ids.begin(),
        formation_unique_ids.end()
    };

    std::ranges::sort(ordered_ids);

    if (
        std::adjacent_find(
            ordered_ids.begin(),
            ordered_ids.end()
        ) != ordered_ids.end()
    ) {
        spdlog::error_s(
            "Military equipment allocation request "
            "contains duplicate formation identities."
        );

        return false;
    }

    for (
        unique_id_t const formation_unique_id :
        ordered_ids
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

    MilitaryEquipmentAllocationResult candidate;

    for (
        unique_id_t const formation_unique_id :
        ordered_ids
    ) {
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
            fixed_point_t const assigned =
                draw_provider(
                    requirement.item_id,
                    requirement.required_quantity
                );

            if (
                assigned < fixed_point_t::_0 ||
                assigned >
                    requirement.required_quantity
            ) {
                spdlog::error_s(
                    "Equipment draw provider returned "
                    "invalid quantity {} for item {} "
                    "with request {}.",
                    assigned,
                    requirement.item_id,
                    requirement.required_quantity
                );

                /*
                 * We cannot roll back a provider that already mutated
                 * external state. The provider contract therefore
                 * requires validation at its own authority boundary.
                 *
                 * We do, however, refuse to publish a partial result.
                 */
                return false;
            }

            candidate.add_allocation(
                formation_unique_id,
                requirement.item_id,
                requirement.required_quantity,
                assigned
            );
        }
    }

    result =
        std::move(candidate);

    return true;
}
