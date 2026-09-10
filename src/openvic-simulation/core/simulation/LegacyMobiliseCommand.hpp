#pragma once

#include "openvic-simulation/core/simulation/CommandTargetIdentity.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <type_safe/strong_typedef.hpp>

namespace OpenVic {

struct LegacyMobiliseCommand final {
	static constexpr char const* COMMAND_TYPE = "military.set_mobilised";
	static constexpr uint32_t PAYLOAD_SCHEMA_VERSION = 1;
	static constexpr std::size_t PAYLOAD_SIZE = 5;

	[[nodiscard]] static CommandTargetIdentity resolve_target_identity(
		std::string_view country_identifier
	) {
		if (country_identifier.empty()) {
			return {};
		}

		std::string qualified_identity { "country:" };
		qualified_identity += country_identifier;

		return CommandTargetIdentity {
			.target_id = qualified_identity,
			.jurisdiction_id = std::move(qualified_identity)
		};
	}
	[[nodiscard]] static std::vector<uint8_t> encode(
		country_index_t country_index,
		bool new_is_mobilised
	) {
		uint32_t const raw = type_safe::get(country_index);
		return {
			static_cast<uint8_t>(raw & 0xffu),
			static_cast<uint8_t>((raw >> 8) & 0xffu),
			static_cast<uint8_t>((raw >> 16) & 0xffu),
			static_cast<uint8_t>((raw >> 24) & 0xffu),
			static_cast<uint8_t>(new_is_mobilised ? 1u : 0u)
		};
	}

	[[nodiscard]] static bool decode(
		std::vector<uint8_t> const& payload,
		country_index_t& country_index,
		bool& new_is_mobilised
	) {
		if (payload.size() != PAYLOAD_SIZE || payload[4] > 1u) {
			return false;
		}

		uint32_t const raw =
			static_cast<uint32_t>(payload[0])
			| (static_cast<uint32_t>(payload[1]) << 8)
			| (static_cast<uint32_t>(payload[2]) << 16)
			| (static_cast<uint32_t>(payload[3]) << 24);

		country_index = country_index_t { raw };
		new_is_mobilised = payload[4] != 0u;
		return true;
	}
};

}