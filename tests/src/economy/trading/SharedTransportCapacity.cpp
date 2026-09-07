#include "openvic-simulation/economy/trading/SharedTransportCapacity.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Shared transport segment proportionally constrains competing flows",
	"[logistics][shared-capacity]"
) {
	SharedTransportCapacity shared {
		"shared_rail_segment",
		fixed_point_t(3)
	};

	REQUIRE(shared.is_valid());

	auto allocations = shared.allocate({
		SharedTransportRequest {
			.flow_id = "mine_a",
			.requested = fixed_point_t(2)
		},
		SharedTransportRequest {
			.flow_id = "mine_b",
			.requested = fixed_point_t(2)
		}
	});

	REQUIRE(allocations.size() == 2);
	CHECK(allocations[0].allocated * fixed_point_t(2) == fixed_point_t(3));
	CHECK(allocations[1].allocated * fixed_point_t(2) == fixed_point_t(3));

	REQUIRE(shared.set_availability_fraction(fixed_point_t::_0));
	allocations = shared.allocate({
		SharedTransportRequest {
			.flow_id = "mine_a",
			.requested = fixed_point_t(2)
		}
	});

	CHECK(allocations[0].allocated == fixed_point_t::_0);
}