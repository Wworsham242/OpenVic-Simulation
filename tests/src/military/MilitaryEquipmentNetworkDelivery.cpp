#include "snitch/snitch.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"
#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAllocation.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAssignment.hpp"
#include "openvic-simulation/military/MilitaryEquipmentDelivery.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitaryFormationInstance.hpp"

using namespace OpenVic;

namespace {

struct NetworkDeliveryFixture {
    MilitaryDomainManager domains;
    MilitaryFormationManager definitions;
    MilitaryFormationInstanceManager runtime;

    MilitaryDomainDefinition const* domain =
        nullptr;

    NetworkDeliveryFixture() {
        REQUIRE(
            domains.add_military_domain(
                "generic"
            )
        );

        domain =
            domains.
                get_military_domain_by_identifier(
                    "generic"
                );

        REQUIRE(domain != nullptr);
    }

    MilitaryFormationDefinition const*
    add_definition(
        std::string_view identifier
    ) {
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
                .required_quantity =
                    fixed_point_t { 10 }
            }
        };

        REQUIRE(
            definitions.add_military_formation(
                identifier,
                *domain,
                capabilities,
                provisions,
                hosting,
                support,
                equipment
            )
        );

        return
            definitions.
                get_military_formation_by_identifier(
                    identifier
                );
    }

    void create(
        std::string_view name,
        MilitaryFormationDefinition const&
            definition
    ) {
        REQUIRE(
            runtime.
                create_military_formation_instance(
                    name,
                    definition
                )
        );
    }
};

MilitaryEquipmentAssignmentState::
    delivery_endpoint_provider_t
endpoint_provider(
    market_node_index_t source,
    market_node_index_t destination
) {
    return
        [
            source,
            destination
        ](
            unique_id_t,
            std::string_view
        )
            -> std::optional<
                MilitaryEquipmentDeliveryEndpoint
            >
        {
            return
                MilitaryEquipmentDeliveryEndpoint {
                    .source_node = source,
                    .destination_node =
                        destination
                };
        };
}

}

TEST_CASE(
    "005A22 network bottleneck caps replenishment before stock draw",
    "[convergence][005a22][military][logistics][bottleneck]"
) {
    NetworkDeliveryFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    LogisticsGraph graph;

    REQUIRE(
        graph.configure({
            LogisticsGraphEdge {
                .edge_id = "port",
                .source =
                    market_node_index_t { 1 },
                .destination =
                    market_node_index_t { 2 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 4 }
                }
            }
        })
    );

    MilitaryEquipmentAssignmentState state;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    fixed_point_t stock =
        fixed_point_t { 100 };

    fixed_point_t observed_draw =
        fixed_point_t::_0;

    MilitaryEquipmentAllocationResult
        allocation;

    MilitaryEquipmentDeliveryResult
        delivery;

    REQUIRE(
        state.allocate_network_replenishment(
            fixture.runtime,
            requests,
            graph,
            endpoint_provider(
                market_node_index_t { 1 },
                market_node_index_t { 2 }
            ),
            [
                &stock,
                &observed_draw
            ](
                std::string_view,
                fixed_point_t requested
            ) {
                observed_draw = requested;

                fixed_point_t const assigned =
                    std::min(
                        stock,
                        requested
                    );

                stock -= assigned;
                return assigned;
            },
            allocation,
            delivery
        )
    );

    CHECK(
        delivery.get_deliverable_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 4 }
    );

    CHECK(
        observed_draw ==
        fixed_point_t { 4 }
    );

    CHECK(
        allocation.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 4 }
    );
}

TEST_CASE(
    "005A22 closed logistics edge blocks physical replenishment",
    "[convergence][005a22][military][logistics][closure]"
) {
    NetworkDeliveryFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    LogisticsGraph graph;

    REQUIRE(
        graph.configure({
            LogisticsGraphEdge {
                .edge_id = "main_port",
                .source =
                    market_node_index_t { 1 },
                .destination =
                    market_node_index_t { 2 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 10 }
                }
            }
        })
    );

    REQUIRE(
        graph.set_edge_open(
            "main_port",
            false
        )
    );

    MilitaryEquipmentAssignmentState state;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    bool draw_called = false;

    MilitaryEquipmentAllocationResult
        allocation;

    MilitaryEquipmentDeliveryResult
        delivery;

    REQUIRE(
        state.allocate_network_replenishment(
            fixture.runtime,
            requests,
            graph,
            endpoint_provider(
                market_node_index_t { 1 },
                market_node_index_t { 2 }
            ),
            [
                &draw_called
            ](
                std::string_view,
                fixed_point_t requested
            ) {
                draw_called = true;
                return requested;
            },
            allocation,
            delivery
        )
    );

    CHECK_FALSE(draw_called);

    CHECK(
        delivery.get_deliverable_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t::_0
    );

    CHECK(
        delivery.get_unmet_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A22 alternate route restores constrained delivery after primary closure",
    "[convergence][005a22][military][logistics][reroute]"
) {
    NetworkDeliveryFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    LogisticsGraph graph;

    REQUIRE(
        graph.configure({
            LogisticsGraphEdge {
                .edge_id = "a_primary",
                .source =
                    market_node_index_t { 1 },
                .destination =
                    market_node_index_t { 4 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 10 }
                }
            },
            LogisticsGraphEdge {
                .edge_id = "b_remote_1",
                .source =
                    market_node_index_t { 1 },
                .destination =
                    market_node_index_t { 3 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 3 }
                }
            },
            LogisticsGraphEdge {
                .edge_id = "b_remote_2",
                .source =
                    market_node_index_t { 3 },
                .destination =
                    market_node_index_t { 4 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 3 }
                }
            }
        })
    );

    REQUIRE(
        graph.set_edge_open(
            "a_primary",
            false
        )
    );

    MilitaryEquipmentAssignmentState state;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    fixed_point_t observed_draw =
        fixed_point_t::_0;

    MilitaryEquipmentAllocationResult
        allocation;

    MilitaryEquipmentDeliveryResult
        delivery;

    REQUIRE(
        state.allocate_network_replenishment(
            fixture.runtime,
            requests,
            graph,
            endpoint_provider(
                market_node_index_t { 1 },
                market_node_index_t { 4 }
            ),
            [
                &observed_draw
            ](
                std::string_view,
                fixed_point_t requested
            ) {
                observed_draw = requested;
                return requested;
            },
            allocation,
            delivery
        )
    );

    CHECK(
        observed_draw ==
        fixed_point_t { 3 }
    );

    REQUIRE(
        delivery.get_deliveries().size() ==
        1
    );

    auto const& fact =
        delivery.get_deliveries()[0];

    REQUIRE(fact.path.found);
    REQUIRE(fact.path.edge_ids.size() == 2);

    CHECK(
        fact.path.edge_ids[0] ==
        "b_remote_1"
    );
}

TEST_CASE(
    "005A22 shared corridor capacity constrains competing formations",
    "[convergence][005a22][military][logistics][shared-capacity]"
) {
    NetworkDeliveryFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation A",
        *definition
    );

    fixture.create(
        "Formation B",
        *definition
    );

    LogisticsGraph graph;

    REQUIRE(
        graph.configure({
            LogisticsGraphEdge {
                .edge_id = "a_feeder",
                .source =
                    market_node_index_t { 1 },
                .destination =
                    market_node_index_t { 3 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 10 }
                }
            },
            LogisticsGraphEdge {
                .edge_id = "b_feeder",
                .source =
                    market_node_index_t { 2 },
                .destination =
                    market_node_index_t { 3 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 10 }
                }
            },
            LogisticsGraphEdge {
                .edge_id = "shared_trunk",
                .source =
                    market_node_index_t { 3 },
                .destination =
                    market_node_index_t { 4 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 10 }
                }
            }
        })
    );

    MilitaryEquipmentAssignmentState state;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        },
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 2,
            .priority = 0
        }
    };

    auto endpoints =
        [](
            unique_id_t formation_unique_id,
            std::string_view
        )
            -> std::optional<
                MilitaryEquipmentDeliveryEndpoint
            >
        {
            market_node_index_t const source_node =
                formation_unique_id == 1
                    ? market_node_index_t { 1 }
                    : market_node_index_t { 2 };

            return
                MilitaryEquipmentDeliveryEndpoint {
                    .source_node =
                        source_node,
                    .destination_node =
                        market_node_index_t { 4 }
                };
        };

    fixed_point_t stock =
        fixed_point_t { 100 };

    MilitaryEquipmentAllocationResult
        allocation;

    MilitaryEquipmentDeliveryResult
        delivery;

    REQUIRE(
        state.allocate_network_replenishment(
            fixture.runtime,
            requests,
            graph,
            endpoints,
            [
                &stock
            ](
                std::string_view,
                fixed_point_t requested
            ) {
                fixed_point_t const assigned =
                    std::min(
                        stock,
                        requested
                    );

                stock -= assigned;
                return assigned;
            },
            allocation,
            delivery
        )
    );

    CHECK(
        delivery.get_deliverable_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );

    CHECK(
        delivery.get_deliverable_quantity(
            2,
            "equipment"
        ) ==
        fixed_point_t { 5 }
    );

    CHECK(
        allocation.get_total_assigned(
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );
}

TEST_CASE(
    "005A22 unconstrained network permits full shortfall delivery",
    "[convergence][005a22][military][logistics][full-delivery]"
) {
    NetworkDeliveryFixture fixture;

    auto const* definition =
        fixture.add_definition(
            "formation"
        );

    REQUIRE(definition != nullptr);

    fixture.create(
        "Formation",
        *definition
    );

    LogisticsGraph graph;

    REQUIRE(
        graph.configure({
            LogisticsGraphEdge {
                .edge_id = "high_capacity",
                .source =
                    market_node_index_t { 1 },
                .destination =
                    market_node_index_t { 2 },
                .leg = TransportLeg {
                    .nominal_capacity =
                        fixed_point_t { 100 }
                }
            }
        })
    );

    MilitaryEquipmentAssignmentState state;

    std::array requests {
        MilitaryEquipmentAllocationRequest {
            .formation_unique_id = 1,
            .priority = 0
        }
    };

    fixed_point_t stock =
        fixed_point_t { 100 };

    MilitaryEquipmentAllocationResult
        allocation;

    MilitaryEquipmentDeliveryResult
        delivery;

    REQUIRE(
        state.allocate_network_replenishment(
            fixture.runtime,
            requests,
            graph,
            endpoint_provider(
                market_node_index_t { 1 },
                market_node_index_t { 2 }
            ),
            [
                &stock
            ](
                std::string_view,
                fixed_point_t requested
            ) {
                fixed_point_t const assigned =
                    std::min(
                        stock,
                        requested
                    );

                stock -= assigned;
                return assigned;
            },
            allocation,
            delivery
        )
    );

    CHECK(
        delivery.get_deliverable_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );

    CHECK(
        allocation.get_assigned_quantity(
            1,
            "equipment"
        ) ==
        fixed_point_t { 10 }
    );
}
