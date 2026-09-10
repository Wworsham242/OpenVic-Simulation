#pragma once

#include <string>
#include <string_view>

namespace OpenVic {
	struct CommandTargetIdentity {
		std::string target_id;
		std::string jurisdiction_id;

		[[nodiscard]] bool is_canonical() const {
			return !target_id.empty() && !jurisdiction_id.empty();
		}

		[[nodiscard]] bool matches_jurisdiction(
			std::string_view requested_jurisdiction_id
		) const {
			return is_canonical()
				&& !requested_jurisdiction_id.empty()
				&& jurisdiction_id == requested_jurisdiction_id;
		}
	};
}
