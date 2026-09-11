#pragma once

#include <string_view>

#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/UnitBranchType.hpp"

namespace OpenVic {

struct MilitaryDomainDefinition : HasIdentifier {
private:
    bool PROPERTY(legacy_land_compatible, false);
    bool PROPERTY(legacy_naval_compatible, false);

public:
    MilitaryDomainDefinition(
        std::string_view new_identifier,
        bool new_legacy_land_compatible = false,
        bool new_legacy_naval_compatible = false
    );

    MilitaryDomainDefinition(MilitaryDomainDefinition&&) = default;
};

struct MilitaryDomainManager {
private:
    IdentifierRegistry<MilitaryDomainDefinition>
        IDENTIFIER_REGISTRY(military_domain);

public:
    bool add_military_domain(
        std::string_view identifier,
        bool legacy_land_compatible = false,
        bool legacy_naval_compatible = false
    );

    bool setup_legacy_domains();

    [[nodiscard]]
    MilitaryDomainDefinition const*
    get_domain_for_legacy_branch(
        unit_branch_t branch
    ) const;
};

}
