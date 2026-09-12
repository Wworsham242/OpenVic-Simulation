#include "LogisticsDispatchExecution.hpp"

using namespace OpenVic;

bool
LogisticsDispatchExecutionCoordinator::
dispatch(
    LogisticsShipmentState& shipment_state,
    TransportExecutionState& execution_state,
    LogisticsShipmentRequest const& shipment_request,
    std::span<TransportExecutionRequirement const> requirements,
    Date dispatch_date,
    Timespan occupation_time,
    LogisticsShipmentState::source_draw_provider_t const&
        source_draw_provider,
    LogisticsDispatchExecutionResult& result
) {
    result = {};

    if (!shipment_state.can_dispatch(shipment_request)) {
        return false;
    }

    unique_id_t reservation_unique_id = 0;

    if (
        !execution_state.reserve_planned(
            requirements,
            dispatch_date,
            occupation_time,
            &reservation_unique_id
        )
    ) {
        return false;
    }

    unique_id_t shipment_unique_id = 0;

    if (
        !shipment_state.dispatch(
            shipment_request,
            dispatch_date,
            source_draw_provider,
            &shipment_unique_id
        )
    ) {
        bool const cancelled =
            execution_state.cancel_reservation(
                reservation_unique_id
            );

        if (!cancelled) {
            return false;
        }
        return false;
    }

    if (shipment_unique_id == 0) {
        bool const cancelled =
            execution_state.cancel_reservation(
                reservation_unique_id
            );

        if (!cancelled) {
            return false;
        }
        return true;
    }

    if (
        !execution_state.bind_reservation_to_shipment(
            reservation_unique_id,
            shipment_unique_id
        )
    ) {
        bool const cancelled =
            execution_state.cancel_reservation(
                reservation_unique_id
            );

        if (!cancelled) {
            return false;
        }
        return false;
    }

    LogisticsShipment const* const shipment =
        shipment_state.get_shipment_by_unique_id(
            shipment_unique_id
        );

    if (shipment == nullptr) {
        bool const cancelled =
            execution_state.cancel_reservation(
                reservation_unique_id
            );

        if (!cancelled) {
            return false;
        }
        return false;
    }

    result.shipment_unique_id = shipment_unique_id;
    result.reservation_unique_id = reservation_unique_id;
    result.dispatched_quantity = shipment->quantity;

    return true;
}
