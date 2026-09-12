#include "LogisticsShipment.hpp"

#include <algorithm>

using namespace OpenVic;

LogisticsShipment const*
LogisticsShipmentState::
get_shipment_by_unique_id(
unique_id_t shipment_unique_id
) const {
auto const it =
std::find_if(
shipments.begin(),
shipments.end(),
[
shipment_unique_id
](LogisticsShipment const& shipment) {
return
shipment.shipment_unique_id ==
shipment_unique_id;
}
);

return
it != shipments.end()
? &*it
: nullptr;
}

fixed_point_t
LogisticsShipmentState::
get_in_transit_quantity(
std::string_view content_id
) const {
fixed_point_t total =
fixed_point_t::_0;

for (
LogisticsShipment const& shipment :
shipments
) {
if (
shipment.content_id == content_id &&
shipment.status ==
LogisticsShipmentStatus::IN_TRANSIT
) {
total += shipment.quantity;
}
}

return total;
}

fixed_point_t
LogisticsShipmentState::
get_arrived_quantity(
std::string_view content_id
) const {
fixed_point_t total =
fixed_point_t::_0;

for (
LogisticsShipment const& shipment :
shipments
) {
if (
shipment.content_id == content_id &&
shipment.status ==
LogisticsShipmentStatus::ARRIVED
) {
total += shipment.quantity;
}
}

return total;
}

bool
LogisticsShipmentState::
can_dispatch(
LogisticsShipmentRequest const& request
) const {
return
!request.content_id.empty() &&
request.requested_quantity > fixed_point_t::_0 &&
request.path.found &&
request.required_transit_time > Timespan { 0 };
}

bool
LogisticsShipmentState::
dispatch(
LogisticsShipmentRequest const& request,
Date dispatch_date,
source_draw_provider_t const&
source_draw_provider,
unique_id_t* created_shipment_unique_id
) {
if (!can_dispatch(request)) {
return false;
}

fixed_point_t const drawn_quantity =
source_draw_provider(
request.content_id,
request.source_node,
request.requested_quantity
);

if (
drawn_quantity <
fixed_point_t::_0 ||
drawn_quantity >
request.requested_quantity
) {
return false;
}

if (
drawn_quantity ==
fixed_point_t::_0
) {
if (
created_shipment_unique_id !=
nullptr
) {
*created_shipment_unique_id = 0;
}

return true;
}

unique_id_t const shipment_unique_id =
next_shipment_unique_id++;

shipments.push_back(
LogisticsShipment {
.shipment_unique_id =
shipment_unique_id,
.content_id =
request.content_id,
.source_node =
request.source_node,
.destination_node =
request.destination_node,
.quantity =
drawn_quantity,
.path =
request.path,
.dispatch_date =
dispatch_date,
.expected_arrival_date =
dispatch_date +
request.required_transit_time,
.status =
LogisticsShipmentStatus::
IN_TRANSIT
}
);

if (
created_shipment_unique_id !=
nullptr
) {
*created_shipment_unique_id =
shipment_unique_id;
}

return true;
}

void
LogisticsShipmentState::
advance_to(
Date current_date
) {
for (
LogisticsShipment& shipment :
shipments
) {
if (
shipment.status ==
LogisticsShipmentStatus::
IN_TRANSIT &&
current_date >=
shipment.expected_arrival_date
) {
shipment.status =
LogisticsShipmentStatus::
ARRIVED;
}
}
}

bool
LogisticsShipmentState::
accept_arrival(
unique_id_t shipment_unique_id,
destination_accept_provider_t const&
destination_accept_provider
) {
auto const it =
std::find_if(
shipments.begin(),
shipments.end(),
[
shipment_unique_id
](LogisticsShipment const& shipment) {
return
shipment.shipment_unique_id ==
shipment_unique_id;
}
);

if (
it == shipments.end() ||
it->status !=
LogisticsShipmentStatus::ARRIVED
) {
return false;
}

if (
!destination_accept_provider(
it->content_id,
it->destination_node,
it->quantity
)
) {
return false;
}

it->status =
LogisticsShipmentStatus::DELIVERED;

return true;
}
