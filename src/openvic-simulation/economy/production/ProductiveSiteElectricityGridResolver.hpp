#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/production/ProductiveSiteUtilityResolver.hpp"
#include "openvic-simulation/economy/trading/LogisticsGraph.hpp"

namespace OpenVic {

struct ProductiveSiteElectricityConnection final {
	std::string employer_id;
	market_node_index_t destination_node {};
};

struct ProductiveSiteElectricityGridState final {
	market_node_index_t source_node {};
	fixed_point_t available_generation_per_tick = fixed_point_t::_0;
};

struct ProductiveSiteElectricityAllocation final {
	std::string employer_id;
	fixed_point_t requested = fixed_point_t::_0;
	fixed_point_t transmission_allocated = fixed_point_t::_0;
	fixed_point_t delivered = fixed_point_t::_0;
};

/// Coarse deterministic electricity allocation for productive sites.
///
/// Generation is one finite shared pool. Transmission uses LogisticsGraph so
/// all productive-site loads contend for shared line capacity in one batched
/// solve. If available generation is tighter than transmission, every reached
/// load receives the same proportional generation factor.
///
/// This intentionally models delivered electrical service, not an inventory.
/// It is not yet unit commitment, economic dispatch, AC power flow, frequency,
/// storage, or a multi-generator market.
class ProductiveSiteElectricityGridResolver final {
private:
	[[nodiscard]] static ProductiveSiteUtilityTarget const* find_target(
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::string_view employer_id
	) {
		auto const it = std::find_if(
			targets.begin(),
			targets.end(),
			[employer_id](ProductiveSiteUtilityTarget const& target) {
				return target.employer_id == employer_id;
			}
		);
		return it != targets.end() ? &*it : nullptr;
	}

	[[nodiscard]] static ProductiveSiteElectricityConnection const*
	find_connection(
		std::vector<ProductiveSiteElectricityConnection> const& connections,
		std::string_view employer_id
	) {
		auto const it = std::find_if(
			connections.begin(),
			connections.end(),
			[employer_id](ProductiveSiteElectricityConnection const& connection) {
				return connection.employer_id == employer_id;
			}
		);
		return it != connections.end() ? &*it : nullptr;
	}

public:
	[[nodiscard]] static std::vector<ProductiveSiteElectricityAllocation> resolve(
		ProductiveSiteElectricityGridState const& grid,
		LogisticsGraph const& transmission_graph,
		std::vector<ProductiveSiteElectricityConnection> const& connections,
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::vector<ProductiveSiteUtilityRequirement>& requirements
	) {
		std::vector<LogisticsGraphFlowRequest> requests;
		std::vector<ProductiveSiteElectricityAllocation> results;

		for (ProductiveSiteUtilityRequirement& requirement : requirements) {
			if (
				requirement.kind != ProductiveSiteUtilityKind::Electricity ||
				requirement.required_per_output <= fixed_point_t::_0
			) {
				continue;
			}

			ProductiveSiteUtilityTarget const* const target =
				find_target(targets, requirement.employer_id);
			ProductiveSiteElectricityConnection const* const connection =
				find_connection(connections, requirement.employer_id);

			if (
				target == nullptr ||
				target->producer == nullptr ||
				connection == nullptr
			) {
				// Once a grid is configured, an electricity load without a
				// grid connection receives no electrical service.
				requirement.available_per_tick = fixed_point_t::_0;
				continue;
			}

			fixed_point_t const requested =
				target->producer->calculate_pre_external_desired_output() *
				requirement.required_per_output;

			std::string flow_id = "electricity|";
			flow_id += requirement.employer_id;

			requests.push_back(LogisticsGraphFlowRequest {
				.flow_id = flow_id,
				.source = grid.source_node,
				.destination = connection->destination_node,
				.requested = requested
			});

			results.push_back(ProductiveSiteElectricityAllocation {
				.employer_id = requirement.employer_id,
				.requested = requested
			});
		}

		auto const transmission_allocations =
			transmission_graph.allocate_flows(requests);

		fixed_point_t total_transmission_allocated = fixed_point_t::_0;

		for (LogisticsGraphFlowAllocation const& allocation :
				transmission_allocations) {
			std::string_view const prefix = "electricity|";
			if (!std::string_view { allocation.flow_id }.starts_with(prefix)) {
				continue;
			}

			std::string_view const employer_id =
				std::string_view { allocation.flow_id }.substr(prefix.size());

			auto const result_it = std::find_if(
				results.begin(),
				results.end(),
				[employer_id](ProductiveSiteElectricityAllocation const& result) {
					return result.employer_id == employer_id;
				}
			);

			if (result_it == results.end()) {
				continue;
			}

			result_it->transmission_allocated = allocation.allocated;
			total_transmission_allocated += allocation.allocated;
		}

		// Generation is a second finite shared pool after transmission.
		// Allocate proportionally, then assign fixed-point residue in stable
		// employer-id order so generation is neither created nor lost merely
		// because the proportional fractions are not exactly representable.
		std::sort(
			results.begin(),
			results.end(),
			[](ProductiveSiteElectricityAllocation const& lhs,
				ProductiveSiteElectricityAllocation const& rhs) {
				return lhs.employer_id < rhs.employer_id;
			}
		);

		fixed_point_t const generation_available = std::min(
			std::max(
				grid.available_generation_per_tick,
				fixed_point_t::_0
			),
			total_transmission_allocated
		);

		fixed_point_t remaining_generation = generation_available;

		if (total_transmission_allocated > fixed_point_t::_0) {
			for (ProductiveSiteElectricityAllocation& result : results) {
				result.delivered = std::min(
					result.transmission_allocated,
					generation_available *
						result.transmission_allocated /
						total_transmission_allocated
				);
				remaining_generation -= result.delivered;
			}

			for (ProductiveSiteElectricityAllocation& result : results) {
				if (remaining_generation <= fixed_point_t::_0) {
					break;
				}

				fixed_point_t const unmet = std::max(
					result.transmission_allocated - result.delivered,
					fixed_point_t::_0
				);

				fixed_point_t const extra = std::min(
					unmet,
					remaining_generation
				);

				result.delivered += extra;
				remaining_generation -= extra;
			}
		}

		for (ProductiveSiteElectricityAllocation const& result : results) {
			for (ProductiveSiteUtilityRequirement& requirement :
					requirements) {
				if (
					requirement.kind ==
						ProductiveSiteUtilityKind::Electricity &&
					requirement.employer_id == result.employer_id
				) {
					requirement.available_per_tick = result.delivered;
					break;
				}
			}
		}

		return results;
	}
};

}