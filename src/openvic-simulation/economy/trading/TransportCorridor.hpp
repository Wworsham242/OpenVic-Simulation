#pragma once

#include <algorithm>
#include <vector>

#include "openvic-simulation/economy/trading/DeliverableSupply.hpp"
#include "openvic-simulation/economy/trading/MarketNodeAccess.hpp"

namespace OpenVic {

/// One coarse transport/access leg in a source-to-destination corridor.
///
/// A leg may represent a rail corridor, port throughput, pipeline segment,
/// shipping chokepoint, border crossing, or other aggregate transport seam.
/// It is intentionally not a vehicle-level object.
struct TransportLeg final {
	fixed_point_t nominal_capacity = 0;
	fixed_point_t availability_fraction = fixed_point_t::_1;
	bool open = true;

	[[nodiscard]] fixed_point_t calculate_effective_capacity() const {
		if (!open || nominal_capacity <= 0 || availability_fraction <= 0) {
			return 0;
		}

		const fixed_point_t bounded_availability = std::clamp(
			availability_fraction,
			fixed_point_t::_0,
			fixed_point_t::_1
		);

		return std::max(
			nominal_capacity * bounded_availability,
			fixed_point_t::_0
		);
	}
};

/// Ordered coarse corridor from one market node to another.
///
/// Capacity is constrained by the tightest leg. This is a deliberately simple
/// first logistics mechanism suitable for rail corridors, ports, pipelines,
/// chokepoints, and similar strategic bottlenecks.
class TransportCorridor final {
private:
	market_node_index_t source_node {};
	market_node_index_t destination_node {};
	std::vector<TransportLeg> legs;

public:
	TransportCorridor(
		market_node_index_t new_source_node,
		market_node_index_t new_destination_node
	) : source_node { new_source_node },
		destination_node { new_destination_node } {}

	void add_leg(TransportLeg leg) {
		legs.push_back(leg);
	}

	[[nodiscard]] market_node_index_t get_source_node() const {
		return source_node;
	}

	[[nodiscard]] market_node_index_t get_destination_node() const {
		return destination_node;
	}

	[[nodiscard]] size_t get_leg_count() const {
		return legs.size();
	}

	[[nodiscard]] fixed_point_t calculate_bottleneck_capacity() const {
		if (legs.empty()) {
			return 0;
		}

		fixed_point_t capacity = fixed_point_t::usable_max;

		for (TransportLeg const& leg : legs) {
			capacity = std::min(
				capacity,
				leg.calculate_effective_capacity()
			);

			if (capacity <= 0) {
				return 0;
			}
		}

		return capacity;
	}

	[[nodiscard]] DeliverableSupply make_deliverable_supply(
		fixed_point_t physical_supply,
		fixed_point_t accessible_fraction = fixed_point_t::_1,
		bool access_allowed = true
	) const {
		return DeliverableSupply {
			.physical_supply = physical_supply,
			.accessible_fraction = accessible_fraction,
			.delivery_capacity = calculate_bottleneck_capacity(),
			.access_allowed = access_allowed
		};
	}

	void publish_access(
		MarketNodeAccessTable& access_table,
		fixed_point_t physical_supply,
		fixed_point_t accessible_fraction = fixed_point_t::_1,
		bool access_allowed = true
	) const {
		access_table.set_access(
			source_node,
			destination_node,
			make_deliverable_supply(
				physical_supply,
				accessible_fraction,
				access_allowed
			)
		);
	}
};

}