#include "MilitaryEquipmentShipment.hpp"

#include <algorithm>
#include <array>

using namespace OpenVic;

MilitaryEquipmentShipmentBinding const*
MilitaryEquipmentShipmentState::
get_binding_by_shipment_unique_id(
    unique_id_t shipment_unique_id
) const {
    auto const it = std::find_if(
        bindings.begin(),
        bindings.end(),
        [shipment_unique_id](auto const& binding) {
            return
                binding.shipment_unique_id ==
                shipment_unique_id;
        }
    );

    return
        it != bindings.end()
            ? &*it
            : nullptr;
}

bool
MilitaryEquipmentShipmentState::
dispatch_replenishment(
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
    unique_id_t* created_shipment_unique_id
) {
    if (
        formation_unique_id == 0 ||
        item_id.empty() ||
        shipment_transit_time <= Timespan { 0 } ||
        route_occupation_time <= Timespan { 0 }
    ) {
        return false;
    }

    fixed_point_t const shortfall =
        assignment_state.get_outstanding_requirement(
            formation_manager,
            formation_unique_id,
            item_id
        );

    if (shortfall <= fixed_point_t::_0) {
        return false;
    }

    std::array delivery_requests {
        MilitaryEquipmentDeliveryRequest {
            .formation_unique_id = formation_unique_id,
            .item_id = std::string { item_id },
            .source_node = endpoint.source_node,
            .destination_node = endpoint.destination_node,
            .requested_quantity = shortfall
        }
    };

    MilitaryEquipmentDeliveryResult delivery_result;

    if (
        !MilitaryEquipmentDeliveryResolver::resolve(
            logistics_graph,
            delivery_requests,
            delivery_result
        )
    ) {
        return false;
    }

    auto const deliveries = delivery_result.get_deliveries();

    if (deliveries.size() != 1) {
        return false;
    }

    MilitaryEquipmentDelivery const& delivery =
        deliveries.front();

    if (
        !delivery.path.found ||
        delivery.deliverable_quantity <= fixed_point_t::_0
    ) {
        return false;
    }

    TransportDemandResult demand;

    if (
        !TransportDemandDeriver::derive(
            demand_profile,
            delivery.deliverable_quantity,
            route_occupation_time,
            demand
        )
    ) {
        return false;
    }

    LogisticsShipmentRequest shipment_request {
        .content_id = std::string { item_id },
        .source_node = endpoint.source_node,
        .destination_node = endpoint.destination_node,
        .requested_quantity = delivery.deliverable_quantity,
        .path = delivery.path,
        .required_transit_time = shipment_transit_time
    };

    LogisticsDispatchExecutionResult dispatch_result;

    if (
        !LogisticsDispatchExecutionCoordinator::dispatch(
            shipment_state,
            execution_state,
            shipment_request,
            demand.requirements,
            dispatch_date,
            demand.occupation_time,
            [&stock_draw_provider](
                std::string_view content_id,
                market_node_index_t,
                fixed_point_t requested_quantity
            ) {
                return stock_draw_provider(
                    content_id,
                    requested_quantity
                );
            },
            dispatch_result
        )
    ) {
        return false;
    }

    if (dispatch_result.shipment_unique_id == 0) {
        if (created_shipment_unique_id != nullptr) {
            *created_shipment_unique_id = 0;
        }

        return true;
    }

    bindings.push_back(
        MilitaryEquipmentShipmentBinding {
            .shipment_unique_id =
                dispatch_result.shipment_unique_id,
            .formation_unique_id = formation_unique_id,
            .item_id = std::string { item_id },
            .completed = false
        }
    );

    if (created_shipment_unique_id != nullptr) {
        *created_shipment_unique_id =
            dispatch_result.shipment_unique_id;
    }

    return true;
}

bool
MilitaryEquipmentShipmentState::
accept_arrived_replenishment(
    unique_id_t shipment_unique_id,
    LogisticsShipmentState& shipment_state,
    MilitaryEquipmentAssignmentState& assignment_state,
    MilitaryFormationInstanceManager const& formation_manager
) {
    auto const binding_it = std::find_if(
        bindings.begin(),
        bindings.end(),
        [shipment_unique_id](auto const& binding) {
            return
                binding.shipment_unique_id ==
                shipment_unique_id;
        }
    );

    if (
        binding_it == bindings.end() ||
        binding_it->completed
    ) {
        return false;
    }

    MilitaryEquipmentShipmentBinding& binding =
        *binding_it;

    bool const accepted =
        shipment_state.accept_arrival(
            shipment_unique_id,
            [
                &assignment_state,
                &formation_manager,
                &binding
            ](
                std::string_view content_id,
                market_node_index_t,
                fixed_point_t quantity
            ) {
                if (
                    content_id != binding.item_id ||
                    quantity <= fixed_point_t::_0
                ) {
                    return false;
                }

                fixed_point_t const outstanding =
                    assignment_state.get_outstanding_requirement(
                        formation_manager,
                        binding.formation_unique_id,
                        binding.item_id
                    );

                if (
                    outstanding < quantity ||
                    outstanding <= fixed_point_t::_0
                ) {
                    return false;
                }

                std::array requests {
                    MilitaryEquipmentAllocationRequest {
                        .formation_unique_id =
                            binding.formation_unique_id,
                        .priority = 0
                    }
                };

                MilitaryEquipmentAllocationResult allocation;

                bool const allocated =
                    MilitaryEquipmentAllocator::
                        allocate_with_quantity_provider(
                            formation_manager,
                            requests,
                            [
                                &binding,
                                quantity
                            ](
                                unique_id_t formation_unique_id,
                                std::string_view item_id,
                                fixed_point_t
                            ) {
                                if (
                                    formation_unique_id ==
                                        binding.formation_unique_id &&
                                    item_id == binding.item_id
                                ) {
                                    return quantity;
                                }

                                return fixed_point_t::_0;
                            },
                            [](
                                std::string_view,
                                fixed_point_t requested_quantity
                            ) {
                                /*
                                 * Shipment authority is the physical
                                 * source at this point. No depot stock
                                 * is drawn a second time.
                                 */
                                return requested_quantity;
                            },
                            allocation
                        );

                if (!allocated) {
                    return false;
                }

                return
                    assignment_state.apply_allocation_result(
                        formation_manager,
                        allocation
                    );
            }
        );

    if (!accepted) {
        return false;
    }

    binding.completed = true;
    return true;
}
