#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace OpenVic {

/// Stable ruleset-facing identity for a position that can exercise authority.
///
/// The position, not its current human/AI controller, is the authority-bearing actor.
/// Examples belong in ruleset data rather than engine enums.
struct PositionIdentity {
	std::string position_id;
	std::string jurisdiction_id;

	bool operator==(PositionIdentity const&) const = default;

	[[nodiscard]] bool is_canonical() const {
		return !position_id.empty() && !jurisdiction_id.empty();
	}

	[[nodiscard]] std::string_view authority_actor_id() const {
		return position_id;
	}
};

enum class PositionControllerKind : uint8_t {
	human,
	ai,
	scripted,
	external
};

/// Current controller occupying a stable authority-bearing position.
///
/// controller_id identifies the input/decision source. It is deliberately not used as the
/// authority actor id, so human and AI occupants of the same position obey identical authority.
struct PositionOccupancy {
	PositionIdentity position;
	std::string controller_id;
	PositionControllerKind controller_kind = PositionControllerKind::human;

	bool operator==(PositionOccupancy const&) const = default;

	[[nodiscard]] bool is_canonical() const {
		if (!position.is_canonical() || controller_id.empty()) {
			return false;
		}

		switch (controller_kind) {
			case PositionControllerKind::human:
			case PositionControllerKind::ai:
			case PositionControllerKind::scripted:
			case PositionControllerKind::external:
				return true;
		}
		return false;
	}

	[[nodiscard]] std::string_view authority_actor_id() const {
		return position.authority_actor_id();
	}

	[[nodiscard]] std::string_view jurisdiction_id() const {
		return position.jurisdiction_id;
	}
};

}