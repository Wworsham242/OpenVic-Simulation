#include "snitch/snitch.hpp"

#include <string_view>

#include "openvic-simulation/economy/trading/LogisticsShipment.hpp"

using namespace OpenVic;

namespace {

LogisticsGraphPath make_path(
std::string edge_id
) {
return LogisticsGraphPath {
.found = true,
.bottleneck_capacity =
fixed_point_t { 100 },
.edge_ids = {
std::move(edge_id)
}
};
}

}

TEST_CASE(
"005A23 dispatch removes source quantity into persistent transit state",
"[convergence][005a23][logistics][shipment][dispatch]"
) {
LogisticsShipmentState state;

fixed_point_t source_stock =
fixed_point_t { 20 };

unique_id_t shipment_id = 0;

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "equipment",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 2 },
.requested_quantity =
fixed_point_t { 8 },
.path =
make_path("sea_route"),
.required_transit_time =
Timespan { 3 }
},
Date { 2026, 1, 1 },
[
&source_stock
](
std::string_view,
market_node_index_t,
fixed_point_t requested
) {
fixed_point_t const drawn =
std::min(
source_stock,
requested
);

source_stock -= drawn;

return drawn;
},
&shipment_id
)
);

CHECK(shipment_id == 1);

CHECK(
source_stock ==
fixed_point_t { 12 }
);

CHECK(
state.get_in_transit_quantity(
"equipment"
) ==
fixed_point_t { 8 }
);

REQUIRE(
state.get_shipments().size() ==
1
);

auto const& shipment =
state.get_shipments()[0];

CHECK(
shipment.status ==
LogisticsShipmentStatus::IN_TRANSIT
);

CHECK(
shipment.dispatch_date ==
Date { 2026, 1, 1 }
);

CHECK(
shipment.expected_arrival_date ==
Date { 2026, 1, 4 }
);

REQUIRE(
shipment.path.edge_ids.size() ==
1
);

CHECK(
shipment.path.edge_ids[0] ==
"sea_route"
);
}

TEST_CASE(
"005A23 shipment arrives only after required transit time",
"[convergence][005a23][logistics][shipment][time]"
) {
LogisticsShipmentState state;

unique_id_t shipment_id = 0;

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "fuel",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 2 },
.requested_quantity =
fixed_point_t { 10 },
.path =
make_path("route"),
.required_transit_time =
Timespan { 5 }
},
Date { 2026, 2, 1 },
[](
std::string_view,
market_node_index_t,
fixed_point_t requested
) {
return requested;
},
&shipment_id
)
);

state.advance_to(
Date { 2026, 2, 5 }
);

REQUIRE(
state.get_shipment_by_unique_id(
shipment_id
) != nullptr
);

CHECK(
state.get_shipment_by_unique_id(
shipment_id
)->is_in_transit()
);

state.advance_to(
Date { 2026, 2, 6 }
);

CHECK(
state.get_shipment_by_unique_id(
shipment_id
)->has_arrived()
);

CHECK(
state.get_in_transit_quantity(
"fuel"
) ==
fixed_point_t::_0
);

CHECK(
state.get_arrived_quantity(
"fuel"
) ==
fixed_point_t { 10 }
);
}

TEST_CASE(
"005A23 near shipment arrives before distant shipment",
"[convergence][005a23][logistics][shipment][distance-proof]"
) {
LogisticsShipmentState state;

unique_id_t near_id = 0;
unique_id_t far_id = 0;

auto const draw =
[](
std::string_view,
market_node_index_t,
fixed_point_t requested
) {
return requested;
};

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "food",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 3 },
.requested_quantity =
fixed_point_t { 5 },
.path =
make_path("near_route"),
.required_transit_time =
Timespan { 2 }
},
Date { 2026, 3, 1 },
draw,
&near_id
)
);

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "food",
.source_node =
market_node_index_t { 2 },
.destination_node =
market_node_index_t { 3 },
.requested_quantity =
fixed_point_t { 5 },
.path =
make_path("distant_route"),
.required_transit_time =
Timespan { 8 }
},
Date { 2026, 3, 1 },
draw,
&far_id
)
);

state.advance_to(
Date { 2026, 3, 4 }
);

REQUIRE(
state.get_shipment_by_unique_id(
near_id
) != nullptr
);

REQUIRE(
state.get_shipment_by_unique_id(
far_id
) != nullptr
);

CHECK(
state.get_shipment_by_unique_id(
near_id
)->has_arrived()
);

CHECK(
state.get_shipment_by_unique_id(
far_id
)->is_in_transit()
);

CHECK(
state.get_arrived_quantity(
"food"
) ==
fixed_point_t { 5 }
);

CHECK(
state.get_in_transit_quantity(
"food"
) ==
fixed_point_t { 5 }
);
}

TEST_CASE(
"005A23 arrived shipment remains authoritative until destination accepts it",
"[convergence][005a23][logistics][shipment][arrival]"
) {
LogisticsShipmentState state;

unique_id_t shipment_id = 0;

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "spares",
.source_node =
market_node_index_t { 4 },
.destination_node =
market_node_index_t { 5 },
.requested_quantity =
fixed_point_t { 6 },
.path =
make_path("route"),
.required_transit_time =
Timespan { 1 }
},
Date { 2026, 4, 1 },
[](
std::string_view,
market_node_index_t,
fixed_point_t requested
) {
return requested;
},
&shipment_id
)
);

state.advance_to(
Date { 2026, 4, 2 }
);

bool reject_called = false;

CHECK_FALSE(
state.accept_arrival(
shipment_id,
[
&reject_called
](
std::string_view,
market_node_index_t,
fixed_point_t
) {
reject_called = true;
return false;
}
)
);

CHECK(reject_called);

REQUIRE(
state.get_shipment_by_unique_id(
shipment_id
) != nullptr
);

CHECK(
state.get_shipment_by_unique_id(
shipment_id
)->has_arrived()
);

fixed_point_t destination_stock =
fixed_point_t::_0;

REQUIRE(
state.accept_arrival(
shipment_id,
[
&destination_stock
](
std::string_view,
market_node_index_t,
fixed_point_t quantity
) {
destination_stock += quantity;
return true;
}
)
);

CHECK(
destination_stock ==
fixed_point_t { 6 }
);

CHECK(
state.get_shipment_by_unique_id(
shipment_id
)->is_delivered()
);

CHECK(
state.get_arrived_quantity(
"spares"
) ==
fixed_point_t::_0
);

CHECK_FALSE(
state.accept_arrival(
shipment_id,
[](
std::string_view,
market_node_index_t,
fixed_point_t
) {
return true;
}
)
);
}

TEST_CASE(
"005A23 invalid shipment cannot create transit state",
"[convergence][005a23][logistics][shipment][validation]"
) {
LogisticsShipmentState state;

bool draw_called = false;

CHECK_FALSE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "equipment",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 2 },
.requested_quantity =
fixed_point_t { 5 },
.path =
make_path("route"),
.required_transit_time =
Timespan { 0 }
},
Date { 2026, 5, 1 },
[
&draw_called
](
std::string_view,
market_node_index_t,
fixed_point_t requested
) {
draw_called = true;
return requested;
}
)
);

CHECK_FALSE(draw_called);

CHECK(
state.get_shipments().empty()
);

CHECK_FALSE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 2 },
.requested_quantity =
fixed_point_t { 5 },
.path =
make_path("route"),
.required_transit_time =
Timespan { 2 }
},
Date { 2026, 5, 1 },
[](
std::string_view,
market_node_index_t,
fixed_point_t requested
) {
return requested;
}
)
);

CHECK(
state.get_shipments().empty()
);
}

TEST_CASE(
"005A23 partial source availability creates only physical partial shipment",
"[convergence][005a23][logistics][shipment][partial]"
) {
LogisticsShipmentState state;

unique_id_t shipment_id = 0;

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "equipment",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 2 },
.requested_quantity =
fixed_point_t { 10 },
.path =
make_path("route"),
.required_transit_time =
Timespan { 3 }
},
Date { 2026, 6, 1 },
[](
std::string_view,
market_node_index_t,
fixed_point_t
) {
return fixed_point_t { 4 };
},
&shipment_id
)
);

REQUIRE(
state.get_shipment_by_unique_id(
shipment_id
) != nullptr
);

CHECK(
state.get_shipment_by_unique_id(
shipment_id
)->quantity ==
fixed_point_t { 4 }
);

CHECK(
state.get_in_transit_quantity(
"equipment"
) ==
fixed_point_t { 4 }
);
}

TEST_CASE(
"005A23 zero source draw creates no shipment",
"[convergence][005a23][logistics][shipment][zero-draw]"
) {
LogisticsShipmentState state;

unique_id_t shipment_id = 99;

REQUIRE(
state.dispatch(
LogisticsShipmentRequest {
.content_id = "equipment",
.source_node =
market_node_index_t { 1 },
.destination_node =
market_node_index_t { 2 },
.requested_quantity =
fixed_point_t { 10 },
.path =
make_path("route"),
.required_transit_time =
Timespan { 3 }
},
Date { 2026, 7, 1 },
[](
std::string_view,
market_node_index_t,
fixed_point_t
) {
return fixed_point_t::_0;
},
&shipment_id
)
);

CHECK(shipment_id == 0);

CHECK(
state.get_shipments().empty()
);
}
