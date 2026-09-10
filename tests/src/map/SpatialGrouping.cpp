#include "openvic-simulation/map/SpatialGrouping.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Spatial grouping requires generic canonical identity",
	"[convergence][geography][grouping][validation]"
) {
	CHECK_FALSE(SpatialGrouping {}.is_canonical());

	CHECK_FALSE(SpatialGrouping {
		.grouping_id = "group:A",
		.grouping_kind = "",
		.member_location_ids = {}
	}.is_canonical());

	CHECK_FALSE(SpatialGrouping {
		.grouping_id = "group:A",
		.grouping_kind = "kind:A",
		.member_location_ids = { "location:B", "location:A" }
	}.is_canonical());

	CHECK(SpatialGrouping {
		.grouping_id = "group:A",
		.grouping_kind = "kind:A",
		.member_location_ids = { "location:A", "location:B" }
	}.is_canonical());
}

TEST_CASE(
	"Spatial group canonicalization is deterministic",
	"[convergence][geography][grouping][determinism]"
) {
	SpatialGrouping const grouping = SpatialGrouping::canonicalize(
		"group:A",
		"kind:A",
		{
			"location:C",
			"location:A",
			"location:B",
			"location:A"
		}
	);

	REQUIRE(grouping.is_canonical());
	REQUIRE(grouping.member_location_ids.size() == 3);

	CHECK(grouping.member_location_ids[0] == "location:A");
	CHECK(grouping.member_location_ids[1] == "location:B");
	CHECK(grouping.member_location_ids[2] == "location:C");

	CHECK(grouping.contains("location:A"));
	CHECK(grouping.contains("location:B"));
	CHECK(grouping.contains("location:C"));
	CHECK_FALSE(grouping.contains("location:D"));
}

TEST_CASE(
	"Independent spatial groupings permit overlapping membership",
	"[convergence][geography][grouping][overlap]"
) {
	SpatialGrouping const grouping_a = SpatialGrouping::canonicalize(
		"group:A",
		"kind:A",
		{ "location:shared", "location:only-A" }
	);

	SpatialGrouping const grouping_b = SpatialGrouping::canonicalize(
		"group:B",
		"kind:B",
		{ "location:shared", "location:only-B" }
	);

	REQUIRE(grouping_a.is_canonical());
	REQUIRE(grouping_b.is_canonical());

	CHECK(grouping_a.contains("location:shared"));
	CHECK(grouping_b.contains("location:shared"));

	CHECK(grouping_a.contains("location:only-A"));
	CHECK_FALSE(grouping_b.contains("location:only-A"));

	CHECK(grouping_b.contains("location:only-B"));
	CHECK_FALSE(grouping_a.contains("location:only-B"));
}
