#pragma once

#include "openvic-simulation/core/simulation/AuthorityRegistry.hpp"
#include "openvic-simulation/player/PlayerManager.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace OpenVic {

struct PositionSessionBootstrap final {
	[[nodiscard]] static bool configure(
		PlayerManager& player_manager,
		AuthorityRegistry& authority_registry,
		PositionOccupancy occupancy,
		std::vector<AuthorityGrant> grants
	) {
		if (!occupancy.is_canonical() || player_manager.has_position_occupancy()) {
			return false;
		}

		std::sort(
			grants.begin(),
			grants.end(),
			[](AuthorityGrant const& lhs, AuthorityGrant const& rhs) {
				if (lhs.command_type != rhs.command_type) {
					return lhs.command_type < rhs.command_type;
				}
				return lhs.jurisdiction_id < rhs.jurisdiction_id;
			}
		);

		if (std::adjacent_find(
			grants.begin(),
			grants.end(),
			[](AuthorityGrant const& lhs, AuthorityGrant const& rhs) {
				return lhs.command_type == rhs.command_type
					&& lhs.jurisdiction_id == rhs.jurisdiction_id;
			}
		) != grants.end()) {
			return false;
		}

		ActorAuthorityProfile profile {
			.actor_id = std::string { occupancy.authority_actor_id() },
			.grants = std::move(grants)
		};

		if (!profile.is_canonical() || authority_registry.contains(profile.actor_id)) {
			return false;
		}

		if (!authority_registry.register_profile(std::move(profile))) {
			return false;
		}
		if (!player_manager.set_position_occupancy(std::move(occupancy))) {
			return false;
		}

		return true;
	}
};

}