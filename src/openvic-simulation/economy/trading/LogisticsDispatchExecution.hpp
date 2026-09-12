#pragma once

#include <span>

#include "openvic-simulation/economy/trading/LogisticsShipment.hpp"
#include "openvic-simulation/economy/trading/TransportExecution.hpp"

namespace OpenVic {

struct LogisticsDispatchExecutionResult {
    unique_id_t shipment_unique_id = 0;
    unique_id_t reservation_unique_id = 0;
    fixed_point_t dispatched_quantity = fixed_point_t::_0;
};

class LogisticsDispatchExecutionCoordinator final {
public:
    [[nodiscard]]
    static bool dispatch(
        LogisticsShipmentState& shipment_state,
        TransportExecutionState& execution_state,
        LogisticsShipmentRequest const& shipment_request,
        std::span<TransportExecutionRequirement const> requirements,
        Date dispatch_date,
        Timespan occupation_time,
        LogisticsShipmentState::source_draw_provider_t const&
            source_draw_provider,
        LogisticsDispatchExecutionResult& result
    );
};

}
