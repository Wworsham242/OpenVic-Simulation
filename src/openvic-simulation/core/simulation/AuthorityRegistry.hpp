#pragma once

#include "openvic-simulation/core/simulation/OrderedCommandRuntime.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace OpenVic {

struct AuthorityGrant {
	std::string command_type;
	std::string jurisdiction_id;

	bool operator==(AuthorityGrant const&) const = default;
};

struct ActorAuthorityProfile {
	std::string actor_id;
	std::vector<AuthorityGrant> grants;

	bool operator==(ActorAuthorityProfile const&) const = default;

	[[nodiscard]] bool is_canonical() const {
		if (actor_id.empty()) {
			return false;
		}
		for (AuthorityGrant const& grant : grants) {
			if (grant.command_type.empty() || grant.jurisdiction_id.empty()) {
				return false;
			}
		}
		return std::is_sorted(
			grants.begin(), grants.end(),
			[](AuthorityGrant const& lhs, AuthorityGrant const& rhs) {
				if (lhs.command_type != rhs.command_type) {
					return lhs.command_type < rhs.command_type;
				}
				return lhs.jurisdiction_id < rhs.jurisdiction_id;
			}
		) && std::adjacent_find(
			grants.begin(), grants.end(),
			[](AuthorityGrant const& lhs, AuthorityGrant const& rhs) {
				return lhs.command_type == rhs.command_type
					&& lhs.jurisdiction_id == rhs.jurisdiction_id;
			}
		) == grants.end();
	}
};

enum class CommandAdmissionResult : uint8_t {
	accepted,
	unknown_actor,
	unauthorized,
	invalid_request
};

class AuthorityRegistry final {
private:
	std::vector<ActorAuthorityProfile> profiles;

	[[nodiscard]] ActorAuthorityProfile const* find_profile(std::string const& actor_id) const {
		auto const iterator = std::lower_bound(
			profiles.begin(), profiles.end(), actor_id,
			[](ActorAuthorityProfile const& candidate, std::string const& value) {
				return candidate.actor_id < value;
			}
		);
		if (iterator == profiles.end() || iterator->actor_id != actor_id) {
			return nullptr;
		}
		return &*iterator;
	}

public:
	[[nodiscard]] bool register_profile(ActorAuthorityProfile profile) {
		if (!profile.is_canonical()) {
			return false;
		}

		auto const iterator = std::lower_bound(
			profiles.begin(), profiles.end(), profile.actor_id,
			[](ActorAuthorityProfile const& candidate, std::string const& actor_id) {
				return candidate.actor_id < actor_id;
			}
		);
		if (iterator != profiles.end() && iterator->actor_id == profile.actor_id) {
			return false;
		}

		profiles.insert(iterator, std::move(profile));
		return true;
	}

	[[nodiscard]] bool contains(std::string const& actor_id) const {
		return find_profile(actor_id) != nullptr;
	}

	[[nodiscard]] bool authorizes(
		std::string const& actor_id,
		std::string const& command_type,
		std::string const& jurisdiction_id
	) const {
		ActorAuthorityProfile const* const profile = find_profile(actor_id);
		if (profile == nullptr) {
			return false;
		}

		AuthorityGrant const needle {
			.command_type = command_type,
			.jurisdiction_id = jurisdiction_id
		};

		return std::binary_search(
			profile->grants.begin(), profile->grants.end(), needle,
			[](AuthorityGrant const& lhs, AuthorityGrant const& rhs) {
				if (lhs.command_type != rhs.command_type) {
					return lhs.command_type < rhs.command_type;
				}
				return lhs.jurisdiction_id < rhs.jurisdiction_id;
			}
		);
	}
};

class CommandAdmissionRuntime final {
private:
	AuthorityRegistry const& authority;
	OrderedCommandRuntime& commands;

public:
	CommandAdmissionRuntime(AuthorityRegistry const& authority_registry, OrderedCommandRuntime& command_runtime)
		: authority { authority_registry }, commands { command_runtime } {}

	[[nodiscard]] CommandAdmissionResult submit(
		SimTime submitted_at,
		std::string actor_id,
		std::string command_type,
		std::string jurisdiction_id,
		std::vector<uint8_t> payload
	) {
		if (actor_id.empty() || command_type.empty() || jurisdiction_id.empty()) {
			return CommandAdmissionResult::invalid_request;
		}
		if (!authority.contains(actor_id)) {
			return CommandAdmissionResult::unknown_actor;
		}
		if (!authority.authorizes(actor_id, command_type, jurisdiction_id)) {
			return CommandAdmissionResult::unauthorized;
		}

		auto const sequence = commands.accept(
			submitted_at,
			std::move(actor_id),
			std::move(command_type),
			std::move(jurisdiction_id),
			std::move(payload)
		);
		return sequence.has_value()
			? CommandAdmissionResult::accepted
			: CommandAdmissionResult::invalid_request;
	}
};

}