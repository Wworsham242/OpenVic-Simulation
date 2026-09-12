#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/*
 * One allocation fact.
 *
 * This is NOT authoritative stock ownership.
 *
 * The external draw provider owns/reserves/removes the physical
 * quantity. This structure records only what the allocation pass
 * assigned to a formation.
 */
struct MilitaryEquipmentAllocation {
    unique_id_t formation_unique_id = 0;
    memory::string item_id;

    fixed_point_t requested_quantity = 0;
    fixed_point_t assigned_quantity = 0;
    fixed_point_t unmet_quantity = 0;
};

struct MilitaryEquipmentAllocationResult {
private:
    memory::vector<
        MilitaryEquipmentAllocation
    > allocations;

public:
    [[nodiscard]]
    std::span<
        MilitaryEquipmentAllocation const
    >
    get_allocations() const {
        return allocations;
    }

    [[nodiscard]]
    fixed_point_t get_assigned_quantity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_unmet_quantity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_total_assigned(
        std::string_view item_id
    ) const;

    void clear() {
        allocations.clear();
    }

private:
    friend struct MilitaryEquipmentAllocator;

    void add_allocation(
        unique_id_t formation_unique_id,
        std::string_view item_id,
        fixed_point_t requested_quantity,
        fixed_point_t assigned_quantity
    );
};

/*
 * Narrow allocation proof.
 *
 * Formations are processed in ascending runtime unique-id order.
 * This provides deterministic neutral ordering for the proof.
 *
 * Final allocation priority belongs to policy/command/procurement
 * mechanics and is intentionally deferred.
 *
 * The draw provider is authoritative for stock mutation:
 *
 *     draw(item_id, requested) -> quantity actually committed
 *
 * It must return a quantity in [0, requested].
 */
/*
 * Allocation priority is intentionally semantic-free.
 *
 * Higher values allocate first.
 * Equal values are resolved by stable formation unique ID.
 *
 * The source of priority belongs to command, policy, AI,
 * mobilization, scenario data, or another external authority.
 */
struct MilitaryEquipmentAllocationRequest {
    unique_id_t formation_unique_id = 0;
    int64_t priority = 0;
};

struct MilitaryEquipmentAllocator final {
    using draw_provider_t =
        std::function<
            fixed_point_t(
                std::string_view,
                fixed_point_t
            )
        >;

    /*
     * Compatibility overload.
     *
     * All formations receive priority zero, preserving the 005A18
     * deterministic formation-ID ordering.
     */
    static bool allocate(
        MilitaryFormationInstanceManager const&
            formation_manager,
        std::span<
            unique_id_t const
        > formation_unique_ids,
        draw_provider_t const& draw_provider,
        MilitaryEquipmentAllocationResult& result
    );

    /*
     * Priority-aware allocation.
     *
     * Higher priority allocates first.
     * Stable formation ID breaks ties.
     */
    static bool allocate(
        MilitaryFormationInstanceManager const&
            formation_manager,
        std::span<
            MilitaryEquipmentAllocationRequest const
        > requests,
        draw_provider_t const& draw_provider,
        MilitaryEquipmentAllocationResult& result
    );
};

}
