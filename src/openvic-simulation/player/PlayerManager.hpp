#pragma once

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/player/PositionIdentity.hpp"
#include "openvic-simulation/utility/Getters.hpp"

#include <optional>
#include <string_view>
#include <utility>

namespace OpenVic {
	struct CountryInstance;

	/// Compatibility owner for player-facing control state.
	///
	/// Legacy OpenVic exposes a direct CountryInstance pointer. VERTICAL-001 preserves that
	/// accessor while introducing generalized position occupancy beside it.
	struct PlayerManager {
	private:
		memory::string PROPERTY(name);
		CountryInstance* PROPERTY_PTR(country, nullptr);
		std::optional<PositionOccupancy> position_occupancy;

	public:
		void set_country(CountryInstance* instance);

		[[nodiscard]] bool set_position_occupancy(PositionOccupancy occupancy) {
			if (!occupancy.is_canonical()) {
				return false;
			}
			position_occupancy = std::move(occupancy);
			return true;
		}

		void clear_position_occupancy() {
			position_occupancy.reset();
		}

		[[nodiscard]] bool has_position_occupancy() const {
			return position_occupancy.has_value();
		}

		[[nodiscard]] PositionOccupancy const* get_position_occupancy() const {
			return position_occupancy ? &*position_occupancy : nullptr;
		}

		[[nodiscard]] std::optional<std::string_view> get_authority_actor_id() const {
			if (!position_occupancy) {
				return std::nullopt;
			}
			return position_occupancy->authority_actor_id();
		}

		[[nodiscard]] std::optional<std::string_view> get_jurisdiction_id() const {
			if (!position_occupancy) {
				return std::nullopt;
			}
			return position_occupancy->jurisdiction_id();
		}
	};
}