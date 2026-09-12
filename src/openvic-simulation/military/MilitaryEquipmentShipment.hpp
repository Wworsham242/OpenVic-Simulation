#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/trading/LogisticsDispatchExecution.hpp"
#include "openvic-simulation/economy/trading/TransportDemand.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAssignment.hpp"
#include "openvic-simulation/military/MilitaryEquipmentDelivery.hpp"

namespace OpenVic {

/*
 * Narrow persistent binding between one generic shipment batch
 * and the military formation/item that requested it.
 *
 * This does not own equipment stock or transport capacity.
 */
struct MilitaryEquipmentShipmentBinding {
    unique_id_t shipment_unique_id = 0;
    unique_id_t formation_unique_id = 0;
    std::string item_id;
    bool completed = false;
};

class MilitaryEquipmentShipmentState final {
private:
    std::vector<MilitaryEquipmentShipmentBinding> bindings;

public:
    [[nodiscard]]
    std::span<MilitaryEquipmentShipmentBinding const>
    get_bindings() const {
        return bindings;
    }

    [[nodiscard]]
    MilitaryEquipmentShipmentBinding const*
    get_binding_by_shipment_unique_id(
        unique_id_t shipment_unique_id
    ) const;

    /*
     * Dispatch one strategically meaningful replenishment batch.
     *
     * Sequence:
     * shortfall -> graph capacity -> aggregate transport demand
     * -> execution reservation -> stock draw -> shipment.
     */
    bool dispatch_replenishment(
        MilitaryEquipmentAssignmentState const& assignment_state,
        MilitaryFormationInstanceManager const& formation_manager,
        unique_id_t formation_unique_id,
        std::string_view item_id,
        LogisticsGraph const& logistics_graph,
        MilitaryEquipmentDeliveryEndpoint const& endpoint,
        TransportDemandProfile const& demand_profile,
        LogisticsShipmentState& shipment_state,
        TransportExecutionState& execution_state,
        Date dispatch_date,
        Timespan shipment_transit_time,
        Timespan route_occupation_time,
        MilitaryEquipmentAllocator::draw_provider_t const&
            stock_draw_provider,
        unique_id_t* created_shipment_unique_id = nullptr
    );

    /*
     * Transfer an arrived shipment from shipment authority into
     * persistent military equipment assignment.
     *
     * Assignment does not change before physical arrival.
     */
    bool accept_arrived_replenishment(
        unique_id_t shipment_unique_id,
        LogisticsShipmentState& shipment_state,
        MilitaryEquipmentAssignmentState& assignment_state,
        MilitaryFormationInstanceManager const& formation_manager
    );
};

}
