#pragma once

#include "openvic-simulation/map/ProvinceDefinition.hpp"
#include "openvic-simulation/map/Region.hpp"
#include "openvic-simulation/map/SpatialGrouping.hpp"

#include <string>
#include <utility>
#include <vector>

namespace OpenVic {

	struct LegacyRegionGroupingAdapter final {
		[[nodiscard]] static SpatialGrouping resolve(
			Region const& region
		) {
			std::vector<std::string> members;
			members.reserve(region.get_provinces().size());

			for (ProvinceDefinition const& province : region.get_provinces()) {
				std::string location_id { "province:" };
				location_id += province.get_identifier();
				members.emplace_back(std::move(location_id));
			}

			std::string grouping_id { "legacy.region:" };
			grouping_id += region.get_identifier();

			return SpatialGrouping::canonicalize(
				std::move(grouping_id),
				"legacy.region",
				std::move(members)
			);
		}
	};

}
