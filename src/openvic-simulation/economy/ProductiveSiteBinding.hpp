#pragma once

#include <optional>
#include <string>

#include "openvic-simulation/economy/production/WorkforceAllocation.hpp"

namespace OpenVic {

struct MapInstance;
struct ProductionType;

// Dispatch belongs to world binding, not to buildings or producers. Future
// scopes extend this resolver and continue allocating against the same POPs.
enum class LaborPoolScope { ProvinceLocal };

struct ResolvedProductiveSite final {
	fixed_point_t installed_capacity;
	WorkforcePool workforce;
};

struct ProductiveSiteBinding final {
	// Province + building identifier is the existing world's facility identity.
	std::string province_id;
	std::string building_id;
	std::string production_type_id;
	LaborPoolScope labor_scope = LaborPoolScope::ProvinceLocal;

	bool operator==(ProductiveSiteBinding const&) const = default;

	// Returned views are for immediate use only; no world pointers are retained.
	[[nodiscard]] std::optional<ResolvedProductiveSite> resolve(
		MapInstance& map, ProductionType const& expected_process
	) const;
};

}
