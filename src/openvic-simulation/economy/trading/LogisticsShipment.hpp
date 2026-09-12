#pragma once

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"
#include "openvic-simulation/types/Date.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

enum class LogisticsShipmentStatus {
IN_TRANSIT,
ARRIVED,
DELIVERED
};

/*
 * Generic request to move physical material through the logistics system.
 *
 * content_id is deliberately opaque. It may identify equipment, fuel,
 * ammunition, food, industrial material, relief supplies, or any other
 * physical content defined by higher-level systems.
 *
 * required_transit_time is supplied by an external route/mode/travel-time
 * mechanism. This state object does not invent transport speed.
 */
struct LogisticsShipmentRequest {
std::string content_id;

market_node_index_t source_node {};
market_node_index_t destination_node {};

fixed_point_t requested_quantity = 0;

LogisticsGraphPath path {};

Timespan required_transit_time {};
};

/*
 * History-bearing physical material between authoritative inventories.
 *
 * Once dispatched, quantity is no longer part of source inventory.
 * Until accepted at destination, it is represented here.
 */
struct LogisticsShipment {
unique_id_t shipment_unique_id = 0;

std::string content_id;

market_node_index_t source_node {};
market_node_index_t destination_node {};

fixed_point_t quantity = 0;

LogisticsGraphPath path {};

Date dispatch_date {};
Date expected_arrival_date {};

LogisticsShipmentStatus status =
LogisticsShipmentStatus::IN_TRANSIT;

[[nodiscard]] bool is_in_transit() const {
return status == LogisticsShipmentStatus::IN_TRANSIT;
}

[[nodiscard]] bool has_arrived() const {
return status == LogisticsShipmentStatus::ARRIVED;
}

[[nodiscard]] bool is_delivered() const {
return status == LogisticsShipmentStatus::DELIVERED;
}
};

class LogisticsShipmentState final {
public:
using source_draw_provider_t =
std::function<
fixed_point_t(
std::string_view content_id,
market_node_index_t source_node,
fixed_point_t requested_quantity
)
>;

using destination_accept_provider_t =
std::function<
bool(
std::string_view content_id,
market_node_index_t destination_node,
fixed_point_t quantity
)
>;

private:
std::vector<LogisticsShipment> shipments;

unique_id_t next_shipment_unique_id = 1;

public:
[[nodiscard]]
std::span<LogisticsShipment const>
get_shipments() const {
return shipments;
}

[[nodiscard]]
LogisticsShipment const*
get_shipment_by_unique_id(
unique_id_t shipment_unique_id
) const;

[[nodiscard]]
fixed_point_t get_in_transit_quantity(
std::string_view content_id
) const;

[[nodiscard]]
fixed_point_t get_arrived_quantity(
std::string_view content_id
) const;

[[nodiscard]]
bool can_dispatch(
LogisticsShipmentRequest const& request
) const;

/*
 * Removes material from authoritative source stock and creates
 * persistent in-transit state.
 *
 * Returns false on validation/provider failure.
 *
 * A partial source draw creates a partial shipment.
 * A zero source draw is valid but creates no shipment.
 */
bool dispatch(
LogisticsShipmentRequest const& request,
Date dispatch_date,
source_draw_provider_t const& source_draw_provider,
unique_id_t* created_shipment_unique_id = nullptr
);

/*
 * Advance shipment state to an authoritative simulation date.
 *
 * Arrival is monotonic. Advancing to an earlier date cannot make an
 * already-arrived shipment become in-transit again.
 */
void advance_to(Date current_date);

/*
 * Transfer an arrived shipment into external destination authority.
 *
 * The shipment is marked DELIVERED only after the destination provider
 * accepts it. Rejection leaves the material in ARRIVED state.
 */
bool accept_arrival(
unique_id_t shipment_unique_id,
destination_accept_provider_t const&
destination_accept_provider
);
};

}
