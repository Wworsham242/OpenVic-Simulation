#include "MilitaryFormation.hpp"

#include <algorithm>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MilitaryCapabilityDefinition::
MilitaryCapabilityDefinition(
    std::string_view new_identifier
) :
    HasIdentifier { new_identifier } {}

MilitaryHostingProfileDefinition::
MilitaryHostingProfileDefinition(
    std::string_view new_identifier
) :
    HasIdentifier { new_identifier } {}

MilitaryFormationDefinition::
MilitaryFormationDefinition(
    std::string_view new_identifier,
    MilitaryDomainDefinition const& new_domain,
    memory::vector<
        std::reference_wrapper<
            MilitaryCapabilityDefinition const
        >
    >&& new_capabilities,
    memory::vector<
        MilitaryHostingProvision
    >&& new_hosting_provisions,
    memory::vector<
        MilitaryHostingRequirement
    >&& new_hosting_requirements
) :
    HasIdentifier { new_identifier },
    domain { new_domain },
    capabilities { std::move(new_capabilities) },
    hosting_provisions {
        std::move(new_hosting_provisions)
    },
    hosting_requirements {
        std::move(new_hosting_requirements)
    } {}

bool MilitaryFormationDefinition::has_capability(
    MilitaryCapabilityDefinition const&
        capability
) const {
    return std::ranges::any_of(
        capabilities,
        [&capability](
            auto const& existing
        ) {
            return &existing.get() == &capability;
        }
    );
}

MilitaryHostingProvision const*
MilitaryFormationDefinition::
get_hosting_provision(
    MilitaryHostingProfileDefinition const&
        profile
) const {
    auto const it = std::ranges::find_if(
        hosting_provisions,
        [&profile](
            MilitaryHostingProvision const&
                provision
        ) {
            return
                &provision.profile.get() ==
                &profile;
        }
    );

    return it == hosting_provisions.end()
        ? nullptr
        : &*it;
}

MilitaryHostingRequirement const*
MilitaryFormationDefinition::
get_hosting_requirement(
    MilitaryHostingProfileDefinition const&
        profile
) const {
    auto const it = std::ranges::find_if(
        hosting_requirements,
        [&profile](
            MilitaryHostingRequirement const&
                requirement
        ) {
            return
                &requirement.profile.get() ==
                &profile;
        }
    );

    return it == hosting_requirements.end()
        ? nullptr
        : &*it;
}

bool MilitaryFormationManager::
add_military_capability(
    std::string_view identifier
) {
    if (identifier.empty()) {
        spdlog::error_s(
            "Invalid military capability "
            "identifier - empty!"
        );
        return false;
    }

    return military_capabilities.emplace_item(
        identifier,
        identifier
    );
}

bool MilitaryFormationManager::
add_military_hosting_profile(
    std::string_view identifier
) {
    if (identifier.empty()) {
        spdlog::error_s(
            "Invalid military hosting profile "
            "identifier - empty!"
        );
        return false;
    }

    return
        military_hosting_profiles.emplace_item(
            identifier,
            identifier
        );
}

bool MilitaryFormationManager::
add_military_formation(
    std::string_view identifier,
    MilitaryDomainDefinition const& domain,
    std::span<
        MilitaryCapabilityDefinition const*
            const
    > capabilities
) {
    std::span<
        MilitaryHostingProvisionSpec const
    > const no_provisions {};

    std::span<
        MilitaryHostingRequirementSpec const
    > const no_requirements {};

    return add_military_formation(
        identifier,
        domain,
        capabilities,
        no_provisions,
        no_requirements
    );
}

bool MilitaryFormationManager::
add_military_formation(
    std::string_view identifier,
    MilitaryDomainDefinition const& domain,
    std::span<
        MilitaryCapabilityDefinition const*
            const
    > new_capabilities,
    std::span<
        MilitaryHostingProvisionSpec const
    > new_hosting_provisions,
    std::span<
        MilitaryHostingRequirementSpec const
    > new_hosting_requirements
) {
    if (identifier.empty()) {
        spdlog::error_s(
            "Invalid military formation "
            "identifier - empty!"
        );
        return false;
    }

    memory::vector<
        std::reference_wrapper<
            MilitaryCapabilityDefinition const
        >
    > capabilities;

    capabilities.reserve(
        new_capabilities.size()
    );

    for (
        MilitaryCapabilityDefinition const*
            capability :
        new_capabilities
    ) {
        if (capability == nullptr) {
            spdlog::error_s(
                "Military formation {} has "
                "a null capability.",
                identifier
            );
            return false;
        }

        bool const duplicate =
            std::ranges::any_of(
                capabilities,
                [capability](
                    auto const& existing
                ) {
                    return
                        &existing.get() ==
                        capability;
                }
            );

        if (duplicate) {
            spdlog::error_s(
                "Military formation {} "
                "contains duplicate capability {}.",
                identifier,
                *capability
            );
            return false;
        }

        capabilities.emplace_back(
            *capability
        );
    }

    memory::vector<
        MilitaryHostingProvision
    > hosting_provisions;

    hosting_provisions.reserve(
        new_hosting_provisions.size()
    );

    for (
        MilitaryHostingProvisionSpec const&
            provision :
        new_hosting_provisions
    ) {
        if (provision.profile == nullptr) {
            spdlog::error_s(
                "Military formation {} has "
                "a null hosting provision profile.",
                identifier
            );
            return false;
        }

        if (provision.capacity <= 0) {
            spdlog::error_s(
                "Military formation {} has "
                "non-positive hosting capacity.",
                identifier
            );
            return false;
        }

        bool const duplicate =
            std::ranges::any_of(
                hosting_provisions,
                [&provision](
                    MilitaryHostingProvision const&
                        existing
                ) {
                    return
                        &existing.profile.get() ==
                        provision.profile;
                }
            );

        if (duplicate) {
            spdlog::error_s(
                "Military formation {} contains "
                "duplicate hosting provision {}.",
                identifier,
                *provision.profile
            );
            return false;
        }

        hosting_provisions.push_back(
            MilitaryHostingProvision {
                .profile = *provision.profile,
                .capacity = provision.capacity
            }
        );
    }

    memory::vector<
        MilitaryHostingRequirement
    > hosting_requirements;

    hosting_requirements.reserve(
        new_hosting_requirements.size()
    );

    for (
        MilitaryHostingRequirementSpec const&
            requirement :
        new_hosting_requirements
    ) {
        if (requirement.profile == nullptr) {
            spdlog::error_s(
                "Military formation {} has "
                "a null hosting requirement profile.",
                identifier
            );
            return false;
        }

        if (requirement.demand <= 0) {
            spdlog::error_s(
                "Military formation {} has "
                "non-positive hosting demand.",
                identifier
            );
            return false;
        }

        bool const duplicate =
            std::ranges::any_of(
                hosting_requirements,
                [&requirement](
                    MilitaryHostingRequirement const&
                        existing
                ) {
                    return
                        &existing.profile.get() ==
                        requirement.profile;
                }
            );

        if (duplicate) {
            spdlog::error_s(
                "Military formation {} contains "
                "duplicate hosting requirement {}.",
                identifier,
                *requirement.profile
            );
            return false;
        }

        hosting_requirements.push_back(
            MilitaryHostingRequirement {
                .profile = *requirement.profile,
                .demand = requirement.demand
            }
        );
    }

    return military_formations.emplace_item(
        identifier,
        identifier,
        domain,
        std::move(capabilities),
        std::move(hosting_provisions),
        std::move(hosting_requirements)
    );
}
