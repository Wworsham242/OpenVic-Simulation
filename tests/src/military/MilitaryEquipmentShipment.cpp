#include "snitch/snitch.hpp"

#include <algorithm>
#include <array>

#include "openvic-simulation/military/MilitaryEquipmentShipment.hpp"
#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"

using namespace OpenVic;

namespace {

struct ShipmentFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;
    MilitaryEquipmentAssignmentState assignments;

    ShipmentFixture() {
        REQUIRE(
            domains.add_military_domain("generic")
        );

        auto const* domain =
            domains.get_military_domain_by_identifier(
                "generic"
            );

        REQUIRE(domain != nullptr);

        std::array<
            MilitaryCapabilityDefinition const*,
            0
        > capabilities {};

        std::array<
            MilitaryHostingProvisionSpec,
            0
        > provisions {};

        std::array<
            MilitaryHostingRequirementSpec,
            0
        > hosting {};

        std::array<
            MilitarySupportTypeDefinition const*,
            0
        > support {};

        std::array equipment {
            MilitaryEquipmentRequirementSpec {
                .item_id = "equipment",
                .required_quantity = fixed_point_t { 10 }
            }
        };

        REQUIRE(
            definitions.add_military_formation(
                "formation",
                *domain,
                capabilities,
                provisions,
                hosting,
                support,
                equipment
            )
        );

        auto const* definition =
            definitions.get_military_formation_by_identifier(
                "formation"
            );

        REQUIRE(definition != nullptr);

        REQUIRE(
            runtime.create_military_formation_instance(
                "Formation",
                *definition
            )
        );
    }

    LogisticsGraph graph(
        fixed_point_t capacity = fixed_point_t { 6 }
    ) {
        LogisticsGraph result;

        REQUIRE(
            result.configure({
                LogisticsGraphEdge {
                    .edge_id = "route",
                    .source = market_node_index_t { 1 },
                    .destination = market_node_index_t { 2 },
                    .leg = TransportLeg {
                        .nominal_capacity = capacity
                    }
                }
            })
        );

        return result;
    }

    TransportDemandProfile profile() {
        return TransportDemandProfile {
            .profile_id = "aggregate_lift",
            .factors = {
                TransportDemandFactor {
                    .resource_id = "lift",
                    .capacity_per_quantity =
                        fixed_point_t { 1 } / 2
                }
            },
            .minimum_occupation_time = Timespan { 1 }
        };
    }

    TransportExecutionState execution(
        fixed_point_t capacity = fixed_point_t { 3 }
    ) {
        TransportExecutionState result;

        REQUIRE(
            result.configure_resources({
                TransportExecutionResource {
                    .resource_id = "lift",
                    .nominal_capacity = capacity
                }
            })
        );

        return result;
    }
};

}

TEST_CASE(
    "005A27 military replenishment remains in transit before assignment",
    "[convergence][005a27][military][logistics][shipment]"
) {
    ShipmentFixture fixture;
    auto graph = fixture.graph();
    auto execution = fixture.execution();
    auto profile = fixture.profile();

    LogisticsShipmentState shipments;
    MilitaryEquipmentShipmentState military_shipments;

    fixed_point_t stock = fixed_point_t { 10 };
    unique_id_t shipment_id = 0;

    REQUIRE(
        military_shipments.dispatch_replenishment(
            fixture.assignments,
            fixture.runtime,
            1,
            "equipment",
            graph,
            MilitaryEquipmentDeliveryEndpoint {
                .source_node = market_node_index_t { 1 },
                .destination_node = market_node_index_t { 2 }
            },
            profile,
            shipments,
            execution,
            Date { 2026, 1, 1 },
            Timespan { 2 },
            Timespan { 4 },
            [&stock](
                std::string_view,
                fixed_point_t requested
            ) {
                fixed_point_t const drawn =
                    std::min(stock, requested);

                stock -= drawn;
                return drawn;
            },
            &shipment_id
        )
    );

    CHECK(shipment_id == 1);
    CHECK(stock == fixed_point_t { 4 });

    CHECK(
        fixture.assignments.get_assigned_quantity(
            1,
            "equipment"
        ) == fixed_point_t::_0
    );

    auto const* shipment =
        shipments.get_shipment_by_unique_id(shipment_id);

    REQUIRE(shipment != nullptr);
    CHECK(shipment->quantity == fixed_point_t { 6 });
    CHECK(shipment->is_in_transit());

    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t::_0
    );
}

TEST_CASE(
    "005A27 arrived military shipment becomes assignment without second stock draw",
    "[convergence][005a27][military][logistics][arrival]"
) {
    ShipmentFixture fixture;
    auto graph = fixture.graph();
    auto execution = fixture.execution();
    auto profile = fixture.profile();

    LogisticsShipmentState shipments;
    MilitaryEquipmentShipmentState military_shipments;

    fixed_point_t stock = fixed_point_t { 10 };
    int stock_draw_calls = 0;
    unique_id_t shipment_id = 0;

    REQUIRE(
        military_shipments.dispatch_replenishment(
            fixture.assignments,
            fixture.runtime,
            1,
            "equipment",
            graph,
            MilitaryEquipmentDeliveryEndpoint {
                .source_node = market_node_index_t { 1 },
                .destination_node = market_node_index_t { 2 }
            },
            profile,
            shipments,
            execution,
            Date { 2026, 2, 1 },
            Timespan { 2 },
            Timespan { 4 },
            [&stock, &stock_draw_calls](
                std::string_view,
                fixed_point_t requested
            ) {
                ++stock_draw_calls;

                fixed_point_t const drawn =
                    std::min(stock, requested);

                stock -= drawn;
                return drawn;
            },
            &shipment_id
        )
    );

    REQUIRE(shipment_id == 1);
    CHECK(stock_draw_calls == 1);
    CHECK(stock == fixed_point_t { 4 });

    shipments.advance_to(Date { 2026, 2, 3 });

    REQUIRE(
        military_shipments.accept_arrived_replenishment(
            shipment_id,
            shipments,
            fixture.assignments,
            fixture.runtime
        )
    );

    CHECK(stock_draw_calls == 1);
    CHECK(stock == fixed_point_t { 4 });

    CHECK(
        fixture.assignments.get_assigned_quantity(
            1,
            "equipment"
        ) == fixed_point_t { 6 }
    );

    auto const* shipment =
        shipments.get_shipment_by_unique_id(shipment_id);

    REQUIRE(shipment != nullptr);
    CHECK(shipment->is_delivered());

    auto const* binding =
        military_shipments.
            get_binding_by_shipment_unique_id(shipment_id);

    REQUIRE(binding != nullptr);
    CHECK(binding->completed);

    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t::_0
    );

    execution.advance_to(Date { 2026, 2, 5 });

    CHECK(
        execution.get_available_capacity("lift") ==
        fixed_point_t { 3 }
    );
}
