#pragma once

#include <map>
#include <optional>
#include <tuple>

#include "openvic-simulation/economy/trading/DeliverableSupply.hpp"
#include "openvic-simulation/types/TypedIndices.hpp"

namespace OpenVic {

/// Stable source/destination identity for a market-access relationship.
///
/// Nodes identify economically meaningful origins/destinations at the lowest
/// sufficient resolution. A node may eventually represent a national market,
/// region, port complex, pipeline zone, strategic facility, or other
/// ruleset-defined trading locus.
struct MarketAccessKey final {
	market_node_index_t source_node {};
	market_node_index_t destination_node {};

	[[nodiscard]] constexpr bool operator==(MarketAccessKey const&) const = default;

	[[nodiscard]] constexpr auto operator<=>(MarketAccessKey const& other) const {
		return std::tie(source_node, destination_node)
			<=> std::tie(other.source_node, other.destination_node);
	}
};

/// Deterministic lookup table for source-to-destination deliverable supply.
///
/// This is not a route solver. It stores the already-derived access envelope
/// for a concrete source/destination relationship. Logistics, sanctions,
/// tariffs, border rules, and routing calculations remain outside this class.
class MarketNodeAccessTable final {
private:
	std::map<MarketAccessKey, DeliverableSupply> access_by_pair;

public:
	void set_access(
		market_node_index_t source_node,
		market_node_index_t destination_node,
		DeliverableSupply access
	) {
		access_by_pair[MarketAccessKey { source_node, destination_node }] = access;
	}

	[[nodiscard]] std::optional<DeliverableSupply> get_access(
		market_node_index_t source_node,
		market_node_index_t destination_node
	) const {
		auto const it = access_by_pair.find(
			MarketAccessKey { source_node, destination_node }
		);

		if (it == access_by_pair.end()) {
			return std::nullopt;
		}
		return it->second;
	}

	[[nodiscard]] fixed_point_t calculate_deliverable_quantity(
		market_node_index_t source_node,
		market_node_index_t destination_node
	) const {
		auto const access = get_access(source_node, destination_node);
		return access.has_value()
			? access->calculate_deliverable_quantity()
			: fixed_point_t::_0;
	}

	[[nodiscard]] bool has_access(
		market_node_index_t source_node,
		market_node_index_t destination_node
	) const {
		return access_by_pair.contains(
			MarketAccessKey { source_node, destination_node }
		);
	}

	void clear_access(
		market_node_index_t source_node,
		market_node_index_t destination_node
	) {
		access_by_pair.erase(
			MarketAccessKey { source_node, destination_node }
		);
	}

	[[nodiscard]] size_t size() const {
		return access_by_pair.size();
	}
};

}