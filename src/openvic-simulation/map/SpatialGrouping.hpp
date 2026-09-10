#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OpenVic {

	struct SpatialGrouping {
		std::string grouping_id;
		std::string grouping_kind;
		std::vector<std::string> member_location_ids;

		[[nodiscard]] bool is_canonical() const {
			if (grouping_id.empty() || grouping_kind.empty()) {
				return false;
			}

			for (std::string const& member : member_location_ids) {
				if (member.empty()) {
					return false;
				}
			}

			return std::is_sorted(
				member_location_ids.begin(),
				member_location_ids.end()
			) && std::adjacent_find(
				member_location_ids.begin(),
				member_location_ids.end()
			) == member_location_ids.end();
		}

		[[nodiscard]] bool contains(
			std::string_view location_id
		) const {
			if (!is_canonical() || location_id.empty()) {
				return false;
			}

			auto const it = std::lower_bound(
				member_location_ids.begin(),
				member_location_ids.end(),
				location_id,
				[](std::string const& lhs, std::string_view rhs) {
					return lhs < rhs;
				}
			);

			return it != member_location_ids.end() && *it == location_id;
		}

		static SpatialGrouping canonicalize(
			std::string grouping_id,
			std::string grouping_kind,
			std::vector<std::string> member_location_ids
		) {
			std::sort(
				member_location_ids.begin(),
				member_location_ids.end()
			);

			member_location_ids.erase(
				std::unique(
					member_location_ids.begin(),
					member_location_ids.end()
				),
				member_location_ids.end()
			);

			return SpatialGrouping {
				.grouping_id = std::move(grouping_id),
				.grouping_kind = std::move(grouping_kind),
				.member_location_ids = std::move(member_location_ids)
			};
		}
	};

}
