#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "openvic-simulation/economy/production/AggregateProducer.hpp"

namespace OpenVic {

enum class ProductiveSiteUtilityKind {
	Electricity,
	IndustrialWater
};

struct ProductiveSiteUtilityRequirement final {
	std::string employer_id;
	ProductiveSiteUtilityKind kind = ProductiveSiteUtilityKind::Electricity;
	fixed_point_t required_per_output = fixed_point_t::_0;
	fixed_point_t available_per_tick = fixed_point_t::_0;
};

struct ProductiveSiteUtilityTarget final {
	std::string employer_id;
	AggregateProducer* producer = nullptr;
};

/// Resolves non-storable utility service constraints before material demand.
///
/// Electricity and industrial water are deliberately not placed into commodity
/// inventories. Each requirement produces an output ceiling:
///     available utility / utility required per output
/// and the tightest utility ceiling is applied to the producer.
///
/// This is a subsystem seam, not yet a power-grid or water-network solver.
/// Future network solvers can populate available_per_tick without changing
/// AggregateProducer or material-flow logic.
class ProductiveSiteUtilityResolver final {
public:
	static void apply(
		std::vector<ProductiveSiteUtilityTarget> const& targets,
		std::vector<ProductiveSiteUtilityRequirement> const& requirements
	) {
		for (ProductiveSiteUtilityTarget const& target : targets) {
			if (target.producer == nullptr) {
				continue;
			}

			target.producer->clear_external_output_ceiling();

			std::optional<fixed_point_t> supported_output;

			for (ProductiveSiteUtilityRequirement const& requirement :
					requirements) {
				if (
					requirement.employer_id != target.employer_id ||
					requirement.required_per_output <= fixed_point_t::_0
				) {
					continue;
				}

				fixed_point_t const feasible =
					std::max(
						requirement.available_per_tick,
						fixed_point_t::_0
					) /
					requirement.required_per_output;

				if (
					!supported_output.has_value() ||
					feasible < *supported_output
				) {
					supported_output = feasible;
				}
			}

			if (supported_output.has_value()) {
				target.producer->set_external_output_ceiling(
					*supported_output
				);
			}
		}
	}
};

}