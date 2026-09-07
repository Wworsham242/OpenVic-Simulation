#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "openvic-simulation/resources/ResourceSupply.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {

struct ResourceSourceState final {
	std::string source_id;
	market_node_index_t node {};
	ResourceSupplyState supply {};

	[[nodiscard]] bool is_valid() const {
		return !source_id.empty() && supply.is_valid();
	}
};

struct ResourceBufferState final {
	fixed_point_t capacity = 0;
	fixed_point_t inventory = 0;

	[[nodiscard]] bool is_valid() const {
		return capacity >= fixed_point_t::_0
			&& inventory >= fixed_point_t::_0
			&& inventory <= capacity;
	}

	[[nodiscard]] fixed_point_t store(fixed_point_t quantity) {
		if (quantity <= fixed_point_t::_0) {
			return fixed_point_t::_0;
		}
		fixed_point_t const room = capacity - inventory;
		fixed_point_t const accepted = std::min(quantity, room);
		inventory += accepted;
		return accepted;
	}

	[[nodiscard]] fixed_point_t draw(fixed_point_t quantity) {
		if (quantity <= fixed_point_t::_0) {
			return fixed_point_t::_0;
		}
		fixed_point_t const released = std::min(quantity, inventory);
		inventory -= released;
		return released;
	}
};

struct ResourceFlowResult final {
	fixed_point_t nominal_supply = 0;
	fixed_point_t accessible_supply = 0;
	fixed_point_t buffer_draw = 0;
	fixed_point_t buffer_store = 0;
	fixed_point_t delivered = 0;
	fixed_point_t unmet = 0;
};

class ResourceSupplyNetwork final {
private:
	std::vector<ResourceSourceState> sources;
	ResourceBufferState buffer {};

	[[nodiscard]] ResourceSourceState* find_source(std::string_view source_id) {
		auto const it = std::find_if(
			sources.begin(),
			sources.end(),
			[source_id](ResourceSourceState const& source) {
				return source.source_id == source_id;
			}
		);
		return it != sources.end() ? &*it : nullptr;
	}

public:
	ResourceSupplyNetwork() = default;

	ResourceSupplyNetwork(
		std::vector<ResourceSourceState> new_sources,
		ResourceBufferState new_buffer = {}
	) : sources { std::move(new_sources) }, buffer { new_buffer } {}

	[[nodiscard]] bool is_valid() const {
		if (!buffer.is_valid()) {
			return false;
		}
		for (size_t i = 0; i < sources.size(); ++i) {
			if (!sources[i].is_valid()) {
				return false;
			}
			for (size_t j = i + 1; j < sources.size(); ++j) {
				if (sources[i].source_id == sources[j].source_id) {
					return false;
				}
			}
		}
		return true;
	}

	[[nodiscard]] size_t source_count() const {
		return sources.size();
	}

	[[nodiscard]] fixed_point_t nominal_supply_per_tick() const {
		fixed_point_t total = 0;
		for (ResourceSourceState const& source : sources) {
			total += source.supply.nominal_per_tick;
		}
		return total;
	}

	[[nodiscard]] fixed_point_t accessible_supply_per_tick() const {
		fixed_point_t total = 0;
		for (ResourceSourceState const& source : sources) {
			total += source.supply.accessible_per_tick();
		}
		return total;
	}

	[[nodiscard]] fixed_point_t buffer_inventory() const {
		return buffer.inventory;
	}

	[[nodiscard]] bool set_source_availability(
		std::string_view source_id,
		fixed_point_t availability_fraction
	) {
		ResourceSourceState* const source = find_source(source_id);
		return source != nullptr
			&& source->supply.set_availability_fraction(availability_fraction);
	}

	/// Resolve one consumer demand step.
	///
	/// Accessible source flow is consumed first. Surplus replenishes the buffer.
	/// Shortfall draws the buffer. Any remainder becomes explicit unmet demand.
	[[nodiscard]] ResourceFlowResult fulfill(fixed_point_t demand) {
		ResourceFlowResult result {
			.nominal_supply = nominal_supply_per_tick(),
			.accessible_supply = accessible_supply_per_tick()
		};

		if (demand <= fixed_point_t::_0) {
			result.buffer_store = buffer.store(result.accessible_supply);
			return result;
		}

		fixed_point_t const direct = std::min(result.accessible_supply, demand);
		fixed_point_t remaining = demand - direct;

		result.buffer_draw = buffer.draw(remaining);
		remaining -= result.buffer_draw;

		fixed_point_t const surplus = result.accessible_supply - direct;
		result.buffer_store = buffer.store(surplus);

		result.delivered = direct + result.buffer_draw;
		result.unmet = remaining;
		return result;
	}
};

}