#include "openvic-simulation/map/LegacyRegionGroupingAdapter.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"Legacy OpenVic region maps into generic spatial grouping",
	"[convergence][geography][grouping][legacy][region]"
) {
	ProvinceDefinition province_b {
		"B",
		colour_t { 0x11, 0x22, 0x33 },
		province_index_t { 1u }
	};

	ProvinceDefinition province_a {
		"A",
		colour_t { 0x44, 0x55, 0x66 },
		province_index_t { 0u }
	};

	Region region {
		"test_region",
		colour_t { 0x77, 0x88, 0x99 },
		false
	};

	REQUIRE(region.add_province(province_b));
	REQUIRE(region.add_province(province_a));

	SpatialGrouping const grouping =
		LegacyRegionGroupingAdapter::resolve(region);

	REQUIRE(grouping.is_canonical());

	CHECK(grouping.grouping_id == "legacy.region:test_region");
	CHECK(grouping.grouping_kind == "legacy.region");

	REQUIRE(grouping.member_location_ids.size() == 2);
	CHECK(grouping.member_location_ids[0] == "province:A");
	CHECK(grouping.member_location_ids[1] == "province:B");

	CHECK(grouping.contains("province:A"));
	CHECK(grouping.contains("province:B"));
	CHECK_FALSE(grouping.contains("province:C"));
}
