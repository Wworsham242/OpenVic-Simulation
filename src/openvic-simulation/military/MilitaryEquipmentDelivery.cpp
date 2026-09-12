#include "MilitaryEquipmentDelivery.hpp"

#include <algorithm>
#include <utility>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

fixed_point_t
MilitaryEquipmentDeliveryResult::
get_deliverable_quantity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    for (
        MilitaryEquipmentDelivery const&
            delivery :
        deliveries
    ) {
        if (
            delivery.formation_unique_id ==
                formation_unique_id &&
            delivery.item_id ==
                item_id
        ) {
            return
                delivery.deliverable_quantity;
        }
    }

    return fixed_point_t::_0;
}

fixed_point_t
MilitaryEquipmentDeliveryResult::
get_unmet_quantity(
    unique_id_t formation_unique_id,
    std::string_view item_id
) const {
    for (
        MilitaryEquipmentDelivery const&
            delivery :
        deliveries
    ) {
        if (
            delivery.formation_unique_id ==
                formation_unique_id &&
            delivery.item_id ==
                item_id
        ) {
            return
                delivery.unmet_quantity;
        }
    }

    return fixed_point_t::_0;
}

void
MilitaryEquipmentDeliveryResult::
add_delivery(
    unique_id_t formation_unique_id,
    std::string_view item_id,
    fixed_point_t requested_quantity,
    LogisticsGraphFlowAllocation const&
        allocation
) {
    deliveries.push_back(
        MilitaryEquipmentDelivery {
            .formation_unique_id =
                formation_unique_id,
            .item_id =
                std::string { item_id },
            .requested_quantity =
                requested_quantity,
            .deliverable_quantity =
                allocation.allocated,
            .unmet_quantity =
                requested_quantity -
                allocation.allocated,
            .path =
                allocation.path,
            .alternate_path =
                allocation.alternate_path
        }
    );
}

bool
MilitaryEquipmentDeliveryResolver::
resolve(
    LogisticsGraph const& logistics_graph,
    std::span<
        MilitaryEquipmentDeliveryRequest const
    > requests,
    MilitaryEquipmentDeliveryResult& result
) {
    /*
     * Canonicalize before routing so caller iteration order does not
     * change deterministic graph contention or rerouting order.
     */
    std::vector<
        MilitaryEquipmentDeliveryRequest
    > ordered {
        requests.begin(),
        requests.end()
    };

    std::ranges::sort(
        ordered,
        [](
            MilitaryEquipmentDeliveryRequest const&
                lhs,
            MilitaryEquipmentDeliveryRequest const&
                rhs
        ) {
            if (
                lhs.formation_unique_id !=
                rhs.formation_unique_id
            ) {
                return
                    lhs.formation_unique_id <
                    rhs.formation_unique_id;
            }

            return
                lhs.item_id <
                rhs.item_id;
        }
    );

    for (
        size_t i = 0;
        i < ordered.size();
        ++i
    ) {
        MilitaryEquipmentDeliveryRequest const&
            request =
                ordered[i];

        if (
            request.item_id.empty() ||
            request.requested_quantity <
                fixed_point_t::_0
        ) {
            spdlog::error_s(
                "Invalid military equipment delivery "
                "request for formation {}.",
                request.formation_unique_id
            );

            return false;
        }

        if (
            i > 0 &&
            ordered[i - 1].
                formation_unique_id ==
                request.formation_unique_id &&
            ordered[i - 1].item_id ==
                request.item_id
        ) {
            spdlog::error_s(
                "Military equipment delivery request "
                "contains duplicate formation/item pair."
            );

            return false;
        }
    }

    std::vector<
        LogisticsGraphFlowRequest
    > graph_requests;

    graph_requests.reserve(
        ordered.size()
    );

    for (
        MilitaryEquipmentDeliveryRequest const&
            request :
        ordered
    ) {
        graph_requests.push_back(
            LogisticsGraphFlowRequest {
                .flow_id =
                    std::to_string(
                        request.
                            formation_unique_id
                    ) +
                    ":" +
                    request.item_id,
                .source =
                    request.source_node,
                .destination =
                    request.destination_node,
                .requested =
                    request.requested_quantity
            }
        );
    }

    auto const allocations =
        logistics_graph.allocate_flows(
            graph_requests
        );

    if (
        allocations.size() !=
        ordered.size()
    ) {
        spdlog::error_s(
            "Logistics graph returned unexpected "
            "military delivery allocation count."
        );

        return false;
    }

    MilitaryEquipmentDeliveryResult candidate;

    for (
        size_t i = 0;
        i < ordered.size();
        ++i
    ) {
        candidate.add_delivery(
            ordered[i].
                formation_unique_id,
            ordered[i].
                item_id,
            ordered[i].
                requested_quantity,
            allocations[i]
        );
    }

    result =
        std::move(candidate);

    return true;
}
