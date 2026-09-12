#include "snitch/snitch.hpp"

#include <array>

#include "openvic-simulation/economy/trading/LogisticsDispatchExecution.hpp"

using namespace OpenVic;

namespace {

LogisticsGraphPath path() {
    return LogisticsGraphPath {
        .found = true,
        .bottleneck_capacity = fixed_point_t { 100 },
        .edge_ids = { "route" }
    };
}

LogisticsShipmentRequest shipment_request(
    fixed_point_t quantity = fixed_point_t { 10 }
) {
    return LogisticsShipmentRequest {
        .content_id = "cargo",
        .source_node = market_node_index_t { 1 },
        .destination_node = market_node_index_t { 2 },
        .requested_quantity = quantity,
        .path = path(),
        .required_transit_time = Timespan { 4 }
    };
}

TransportExecutionState execution_state(
    fixed_point_t lift = fixed_point_t { 10 }
) {
    TransportExecutionState state;
    REQUIRE(
        state.configure_resources({
            TransportExecutionResource {
                .resource_id = "lift",
                .nominal_capacity = lift
            },
            TransportExecutionResource {
                .resource_id = "drivers",
                .nominal_capacity = fixed_point_t { 5 }
            }
        })
    );
    return state;
}

}

TEST_CASE(
    "005A25 insufficient transport prevents physical source draw",
    "[convergence][005a25][logistics][dispatch][preflight]"
) {
    LogisticsShipmentState shipments;
    auto execution = execution_state(fixed_point_t { 4 });

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 5 }
        }
    };

    bool draw_called = false;
    LogisticsDispatchExecutionResult result;

    CHECK_FALSE(
        LogisticsDispatchExecutionCoordinator::dispatch(
            shipments,
            execution,
            shipment_request(),
            requirements,
            Date { 2026, 1, 1 },
            Timespan { 4 },
            [&draw_called](
                std::string_view,
                market_node_index_t,
                fixed_point_t requested
            ) {
                draw_called = true;
                return requested;
            },
            result
        )
    );

    CHECK_FALSE(draw_called);
    CHECK(shipments.get_shipments().empty());
    CHECK(execution.get_reservations().empty());
}

TEST_CASE(
    "005A25 successful dispatch binds transport reservation to shipment",
    "[convergence][005a25][logistics][dispatch][bind]"
) {
    LogisticsShipmentState shipments;
    auto execution = execution_state();

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 6 }
        },
        TransportExecutionRequirement {
            .resource_id = "drivers",
            .required_capacity = fixed_point_t { 2 }
        }
    };

    fixed_point_t source_stock = fixed_point_t { 20 };
    LogisticsDispatchExecutionResult result;

    REQUIRE(
        LogisticsDispatchExecutionCoordinator::dispatch(
            shipments,
            execution,
            shipment_request(),
            requirements,
            Date { 2026, 2, 1 },
            Timespan { 4 },
            [&source_stock](
                std::string_view,
                market_node_index_t,
                fixed_point_t requested
            ) {
                source_stock -= requested;
                return requested;
            },
            result
        )
    );

    CHECK(result.shipment_unique_id == 1);
    CHECK(result.reservation_unique_id == 1);
    CHECK(result.dispatched_quantity == fixed_point_t { 10 });
    CHECK(source_stock == fixed_point_t { 10 });

    auto const* reservation =
        execution.get_reservation_by_unique_id(
            result.reservation_unique_id
        );

    REQUIRE(reservation != nullptr);
    CHECK(reservation->active);
    CHECK(
        reservation->shipment_unique_id ==
        result.shipment_unique_id
    );
    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t { 4 }
    );
}

TEST_CASE(
    "005A25 zero source stock cancels planned transport reservation",
    "[convergence][005a25][logistics][dispatch][zero-source]"
) {
    LogisticsShipmentState shipments;
    auto execution = execution_state();

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 5 }
        }
    };

    LogisticsDispatchExecutionResult result;

    REQUIRE(
        LogisticsDispatchExecutionCoordinator::dispatch(
            shipments,
            execution,
            shipment_request(),
            requirements,
            Date { 2026, 3, 1 },
            Timespan { 4 },
            [](
                std::string_view,
                market_node_index_t,
                fixed_point_t
            ) {
                return fixed_point_t::_0;
            },
            result
        )
    );

    CHECK(result.shipment_unique_id == 0);
    CHECK(result.reservation_unique_id == 0);
    CHECK(shipments.get_shipments().empty());
    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t { 10 }
    );
    REQUIRE(execution.get_reservations().size() == 1);
    CHECK_FALSE(execution.get_reservations()[0].active);
}

TEST_CASE(
    "005A25 invalid shipment request does not reserve transport",
    "[convergence][005a25][logistics][dispatch][validation]"
) {
    LogisticsShipmentState shipments;
    auto execution = execution_state();

    auto request = shipment_request();
    request.required_transit_time = Timespan { 0 };

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 5 }
        }
    };

    bool draw_called = false;
    LogisticsDispatchExecutionResult result;

    CHECK_FALSE(
        LogisticsDispatchExecutionCoordinator::dispatch(
            shipments,
            execution,
            request,
            requirements,
            Date { 2026, 4, 1 },
            Timespan { 4 },
            [&draw_called](
                std::string_view,
                market_node_index_t,
                fixed_point_t requested
            ) {
                draw_called = true;
                return requested;
            },
            result
        )
    );

    CHECK_FALSE(draw_called);
    CHECK(execution.get_reservations().empty());
}

TEST_CASE(
    "005A25 transport remains occupied independently of shipment arrival",
    "[convergence][005a25][logistics][dispatch][occupation]"
) {
    LogisticsShipmentState shipments;
    auto execution = execution_state();

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 10 }
        }
    };

    LogisticsDispatchExecutionResult result;

    REQUIRE(
        LogisticsDispatchExecutionCoordinator::dispatch(
            shipments,
            execution,
            shipment_request(),
            requirements,
            Date { 2026, 5, 1 },
            Timespan { 8 },
            [](
                std::string_view,
                market_node_index_t,
                fixed_point_t requested
            ) {
                return requested;
            },
            result
        )
    );

    shipments.advance_to(Date { 2026, 5, 5 });

    REQUIRE(
        shipments.get_shipment_by_unique_id(
            result.shipment_unique_id
        ) != nullptr
    );

    CHECK(
        shipments.get_shipment_by_unique_id(
            result.shipment_unique_id
        )->has_arrived()
    );

    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t::_0
    );

    execution.advance_to(Date { 2026, 5, 9 });

    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A25 partial physical draw creates matching partial shipment without duplicate stock",
    "[convergence][005a25][logistics][dispatch][partial]"
) {
    LogisticsShipmentState shipments;
    auto execution = execution_state();

    std::array requirements {
        TransportExecutionRequirement {
            .resource_id = "lift",
            .required_capacity = fixed_point_t { 5 }
        }
    };

    fixed_point_t source_stock = fixed_point_t { 4 };
    LogisticsDispatchExecutionResult result;

    REQUIRE(
        LogisticsDispatchExecutionCoordinator::dispatch(
            shipments,
            execution,
            shipment_request(fixed_point_t { 10 }),
            requirements,
            Date { 2026, 6, 1 },
            Timespan { 4 },
            [&source_stock](
                std::string_view,
                market_node_index_t,
                fixed_point_t requested
            ) {
                fixed_point_t const drawn =
                    std::min(source_stock, requested);
                source_stock -= drawn;
                return drawn;
            },
            result
        )
    );

    CHECK(source_stock == fixed_point_t::_0);
    CHECK(result.dispatched_quantity == fixed_point_t { 4 });

    REQUIRE(
        shipments.get_shipment_by_unique_id(
            result.shipment_unique_id
        ) != nullptr
    );

    CHECK(
        shipments.get_shipment_by_unique_id(
            result.shipment_unique_id
        )->quantity == fixed_point_t { 4 }
    );

    auto const* reservation =
        execution.get_reservation_by_unique_id(
            result.reservation_unique_id
        );

    REQUIRE(reservation != nullptr);
    CHECK(reservation->active);
    CHECK(
        reservation->shipment_unique_id ==
        result.shipment_unique_id
    );

    CHECK(
        execution.get_reserved_capacity("lift") ==
        fixed_point_t { 5 }
    );
}
