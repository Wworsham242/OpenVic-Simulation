#pragma once

#include "openvic-simulation/military/Deployment.hpp"
#include "openvic-simulation/military/LeaderTrait.hpp"
#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/military/UnitType.hpp"
#include "openvic-simulation/military/Wargoal.hpp"

namespace OpenVic {
	struct MilitaryManager {
	private:
		MilitaryDomainManager PROPERTY_REF(military_domain_manager);
		UnitTypeManager PROPERTY_REF(unit_type_manager);
		LeaderTraitManager PROPERTY_REF(leader_trait_manager);
		DeploymentManager PROPERTY_REF(deployment_manager);
		WargoalTypeManager PROPERTY_REF(wargoal_type_manager);
	};
}
