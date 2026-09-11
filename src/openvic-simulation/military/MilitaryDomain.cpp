#include "MilitaryDomain.hpp"

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MilitaryDomainDefinition::MilitaryDomainDefinition(
    std::string_view new_identifier,
    bool new_legacy_land_compatible,
    bool new_legacy_naval_compatible
) :
    HasIdentifier { new_identifier },
    legacy_land_compatible { new_legacy_land_compatible },
    legacy_naval_compatible { new_legacy_naval_compatible } {}

bool MilitaryDomainManager::add_military_domain(
    std::string_view identifier,
    bool legacy_land_compatible,
    bool legacy_naval_compatible
) {
    if (identifier.empty()) {
        spdlog::error_s(
            "Invalid military domain identifier - empty!"
        );
        return false;
    }

    if (
        legacy_land_compatible &&
        legacy_naval_compatible
    ) {
        spdlog::error_s(
            "Military domain {} cannot be both legacy land "
            "and legacy naval compatible.",
            identifier
        );
        return false;
    }

    if (legacy_land_compatible) {
        for (
            MilitaryDomainDefinition const& existing :
            get_military_domains()
        ) {
            if (existing.get_legacy_land_compatible()) {
                spdlog::error_s(
                    "Military domain {} already owns legacy LAND "
                    "compatibility.",
                    existing
                );
                return false;
            }
        }
    }

    if (legacy_naval_compatible) {
        for (
            MilitaryDomainDefinition const& existing :
            get_military_domains()
        ) {
            if (existing.get_legacy_naval_compatible()) {
                spdlog::error_s(
                    "Military domain {} already owns legacy NAVAL "
                    "compatibility.",
                    existing
                );
                return false;
            }
        }
    }

    return military_domains.emplace_item(
        identifier,
        identifier,
        legacy_land_compatible,
        legacy_naval_compatible
    );
}

bool MilitaryDomainManager::setup_legacy_domains() {
    bool ret = true;

    if (
        get_military_domain_by_identifier("land") == nullptr
    ) {
        ret &= add_military_domain(
            "land",
            true,
            false
        );
    }

    if (
        get_military_domain_by_identifier("naval") == nullptr
    ) {
        ret &= add_military_domain(
            "naval",
            false,
            true
        );
    }

    return ret;
}

MilitaryDomainDefinition const*
MilitaryDomainManager::get_domain_for_legacy_branch(
    unit_branch_t branch
) const {
    using enum unit_branch_t;

    switch (branch) {
        case LAND:
            for (
                MilitaryDomainDefinition const& domain :
                get_military_domains()
            ) {
                if (
                    domain.get_legacy_land_compatible()
                ) {
                    return &domain;
                }
            }

            return nullptr;

        case NAVAL:
            for (
                MilitaryDomainDefinition const& domain :
                get_military_domains()
            ) {
                if (
                    domain.get_legacy_naval_compatible()
                ) {
                    return &domain;
                }
            }

            return nullptr;

        default:
            return nullptr;
    }
}
