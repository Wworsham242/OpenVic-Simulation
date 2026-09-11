#include "MilitaryFormation.hpp"

#include <algorithm>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MilitaryCapabilityDefinition::
MilitaryCapabilityDefinition(
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
    >&& new_capabilities
) :
    HasIdentifier { new_identifier },
    domain { new_domain },
    capabilities { std::move(new_capabilities) } {}

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
add_military_formation(
    std::string_view identifier,
    MilitaryDomainDefinition const& domain,
    std::span<
        MilitaryCapabilityDefinition const*
            const
    > new_capabilities
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

    return military_formations.emplace_item(
        identifier,
        identifier,
        domain,
        std::move(capabilities)
    );
}
