#pragma once

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"
#include "openvic-simulation/military/MilitaryEquipmentAllocation.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/*
 * Source/destination binding supplied by an external geography,
 * logistics, basing, depot, or scenario authority.
 *
 * The delivery resolver does not decide what a node means.
 */
struct MilitaryEquipmentDeliveryEndpoint {
    market_node_index_t source_node {};
    market_node_index_t destination_node {};
};

/*
 * One physical network-delivery demand.
 *
 * requested_quantity is already the executable equipment demand
 * produced upstream (for example by persistent shortfall and later
 * command/logistics planning mechanics).
 */
struct MilitaryEquipmentDeliveryRequest {
    unique_id_t formation_unique_id = 0;
    std::string item_id;

    market_node_index_t source_node {};
    market_node_index_t destination_node {};

    fixed_point_t requested_quantity = 0;
};

/*
 * Derived physical-delivery fact.
 *
 * This is not persistent inventory and not a second transport ledger.
 * The authoritative transport network remains LogisticsGraph.
 */
struct MilitaryEquipmentDelivery {
    unique_id_t formation_unique_id = 0;
    std::string item_id;

    fixed_point_t requested_quantity = 0;
    fixed_point_t deliverable_quantity = 0;
    fixed_point_t unmet_quantity = 0;

    LogisticsGraphPath path {};
    LogisticsGraphPath alternate_path {};
};

struct MilitaryEquipmentDeliveryResult {
private:
    std::vector<
        MilitaryEquipmentDelivery
    > deliveries;

public:
    [[nodiscard]]
    std::span<
        MilitaryEquipmentDelivery const
    >
    get_deliveries() const {
        return deliveries;
    }

    [[nodiscard]]
    fixed_point_t get_deliverable_quantity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    [[nodiscard]]
    fixed_point_t get_unmet_quantity(
        unique_id_t formation_unique_id,
        std::string_view item_id
    ) const;

    void clear() {
        deliveries.clear();
    }

private:
    friend struct MilitaryEquipmentDeliveryResolver;

    void add_delivery(
        unique_id_t formation_unique_id,
        std::string_view item_id,
        fixed_point_t requested_quantity,
        LogisticsGraphFlowAllocation const&
            allocation
    );
};

/*
 * Narrow adapter between military equipment demand and the generic
 * strategic logistics graph.
 *
 * It does not own:
 *
 * - stock;
 * - transport infrastructure;
 * - formation placement;
 * - depot choice;
 * - territorial access policy;
 * - route semantics.
 *
 * It asks the existing LogisticsGraph what quantity can physically
 * traverse the configured network.
 */
struct MilitaryEquipmentDeliveryResolver final {
    static bool resolve(
        LogisticsGraph const& logistics_graph,
        std::span<
            MilitaryEquipmentDeliveryRequest const
        > requests,
        MilitaryEquipmentDeliveryResult& result
    );
};

}
