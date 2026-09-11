#pragma once

#include <functional>
#include <span>
#include <string_view>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

struct MilitaryCapabilityDefinition : HasIdentifier {
    explicit MilitaryCapabilityDefinition(
        std::string_view new_identifier
    );

    MilitaryCapabilityDefinition(
        MilitaryCapabilityDefinition&&
    ) = default;
};

struct MilitaryFormationDefinition : HasIdentifier {
private:
    MilitaryDomainDefinition const& domain;

    memory::vector<
        std::reference_wrapper<
            MilitaryCapabilityDefinition const
        >
    > capabilities;

public:
    MilitaryFormationDefinition(
        std::string_view new_identifier,
        MilitaryDomainDefinition const& new_domain,
        memory::vector<
            std::reference_wrapper<
                MilitaryCapabilityDefinition const
            >
        >&& new_capabilities
    );

    MilitaryFormationDefinition(
        MilitaryFormationDefinition&&
    ) = default;

    [[nodiscard]]
    MilitaryDomainDefinition const&
    get_domain() const {
        return domain;
    }

    [[nodiscard]]
    std::span<
        std::reference_wrapper<
            MilitaryCapabilityDefinition const
        > const
    >
    get_capabilities() const {
        return capabilities;
    }

    [[nodiscard]]
    bool has_capability(
        MilitaryCapabilityDefinition const&
            capability
    ) const;
};

struct MilitaryFormationManager {
private:
    IdentifierRegistry<
        MilitaryCapabilityDefinition
    > IDENTIFIER_REGISTRY_CUSTOM_PLURAL(
        military_capability,
        military_capabilities
    );

    IdentifierRegistry<
        MilitaryFormationDefinition
    > IDENTIFIER_REGISTRY(military_formation);

public:
    bool add_military_capability(
        std::string_view identifier
    );

    bool add_military_formation(
        std::string_view identifier,
        MilitaryDomainDefinition const& domain,
        std::span<
            MilitaryCapabilityDefinition const*
                const
        > capabilities
    );
};

}
