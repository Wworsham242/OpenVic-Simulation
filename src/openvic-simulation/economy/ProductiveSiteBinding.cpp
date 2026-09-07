#include "ProductiveSiteBinding.hpp"

#include "openvic-simulation/economy/BuildingInstance.hpp"
#include "openvic-simulation/economy/BuildingType.hpp"
#include "openvic-simulation/economy/production/ProductionType.hpp"
#include "openvic-simulation/map/MapInstance.hpp"

using namespace OpenVic;

std::optional<ResolvedProductiveSite> ProductiveSiteBinding::resolve(
	MapInstance& map, ProductionType const& expected_process
) const {
	if (province_id.empty() || building_id.empty() || production_type_id != expected_process.get_identifier()) {
		return std::nullopt;
	}
	ProvinceInstance* province = map.get_province_instance_by_identifier(province_id);
	if (province == nullptr) { return std::nullopt; }
	for (BuildingInstance const& building : province->get_buildings()) {
		if (building.get_identifier() != building_id) { continue; }
		BuildingType const& type = building.building_type;
		if (!type.is_setting_general_capacity_asset() || type.production_type != &expected_process
			|| building.get_level() < building_level_t { 0 } || building.get_level() > type.max_level) {
			return std::nullopt;
		}
		switch (labor_scope) {
		case LaborPoolScope::ProvinceLocal:
			return ResolvedProductiveSite {
				type.calculate_installed_capacity(building.get_level()),
				WorkforceColonyView { province->get_mutable_pops() }
			};
		}
		return std::nullopt;
	}
	return std::nullopt;
}
