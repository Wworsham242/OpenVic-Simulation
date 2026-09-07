#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/types/IndexedFlatMap.hpp"

#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace OpenVic;

TEST_CASE(
	"IndexedFlatMap safely accepts an empty indexed definition domain",
	"[types][indexed-flat-map][empty-domain]"
) {
	GoodDefinitionManager definitions;

	IndexedFlatMap<GoodDefinition, int> map {
		definitions.get_good_definitions(),
		[](GoodDefinition const&) -> int {
			return 0;
		}
	};

	CHECK(map.get_keys().empty());
	CHECK(map.get_min_index() == 0);
	CHECK(map.get_max_index() == 0);
}