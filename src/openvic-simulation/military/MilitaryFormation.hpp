#pragma once

#include <functional>
#include <span>
#include <string_view>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/MilitaryDomain.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

struct MilitaryCapabilityDefinition : HasIdentifier {
    explicit MilitaryCapabilityDefinition(
        std::string_view new_identifier
    );

    MilitaryCapabilityDefinition(
        MilitaryCapabilityDefinition&&
    ) = default;
};

struct MilitaryHostingProfileDefinition :
    HasIdentifier {

    explicit MilitaryHostingProfileDefinition(
        std::string_view new_identifier
    );

    MilitaryHostingProfileDefinition(
        MilitaryHostingProfileDefinition&&
    ) = default;
};

/*
 * Input specifications used while constructing a formation
 * definition.
 *
 * The profile identifier carries the semantic meaning.
 * Capacity/demand are generic profile-local units.
 */
struct MilitaryHostingProvisionSpec {
    MilitaryHostingProfileDefinition const*
        profile = nullptr;

    fixed_point_t capacity = 0;
};

struct MilitaryHostingRequirementSpec {
    MilitaryHostingProfileDefinition const*
        profile = nullptr;

    fixed_point_t demand = 0;
};

struct MilitaryHostingProvision {
    std::reference_wrapper<
        MilitaryHostingProfileDefinition const
    > profile;

    fixed_point_t capacity = 0;
};

struct MilitaryHostingRequirement {
    std::reference_wrapper<
        MilitaryHostingProfileDefinition const
    > profile;

    fixed_point_t demand = 0;
};

struct MilitaryFormationDefinition : HasIdentifier {
private:
    MilitaryDomainDefinition const& domain;

    memory::vector<
        std::reference_wrapper<
            MilitaryCapabilityDefinition const
        >
    > capabilities;

    memory::vector<
        MilitaryHostingProvision
    > hosting_provisions;

    memory::vector<
        MilitaryHostingRequirement
    > hosting_requirements;

public:
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
    std::span<
        MilitaryHostingProvision const
    >
    get_hosting_provisions() const {
        return hosting_provisions;
    }

    [[nodiscard]]
    std::span<
        MilitaryHostingRequirement const
    >
    get_hosting_requirements() const {
        return hosting_requirements;
    }

    [[nodiscard]]
    bool has_capability(
        MilitaryCapabilityDefinition const&
            capability
    ) const;

    [[nodiscard]]
    MilitaryHostingProvision const*
    get_hosting_provision(
        MilitaryHostingProfileDefinition const&
            profile
    ) const;

    [[nodiscard]]
    MilitaryHostingRequirement const*
    get_hosting_requirement(
        MilitaryHostingProfileDefinition const&
            profile
    ) const;

    [[nodiscard]]
    bool has_hosting_contract() const {
        return !hosting_provisions.empty() ||
            !hosting_requirements.empty();
    }
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
        MilitaryHostingProfileDefinition
    > IDENTIFIER_REGISTRY_CUSTOM_PLURAL(
        military_hosting_profile,
        military_hosting_profiles
    );

    IdentifierRegistry<
        MilitaryFormationDefinition
    > IDENTIFIER_REGISTRY(military_formation);

public:
    bool add_military_capability(
        std::string_view identifier
    );

    bool add_military_hosting_profile(
        std::string_view identifier
    );

    /*
     * Compatibility overload retained for formations that do not
     * require typed hosting semantics.
     */
    bool add_military_formation(
        std::string_view identifier,
        MilitaryDomainDefinition const& domain,
        std::span<
            MilitaryCapabilityDefinition const*
                const
        > capabilities
    );

    bool add_military_formation(
        std::string_view identifier,
        MilitaryDomainDefinition const& domain,
        std::span<
            MilitaryCapabilityDefinition const*
                const
        > capabilities,
        std::span<
            MilitaryHostingProvisionSpec const
        > hosting_provisions,
        std::span<
            MilitaryHostingRequirementSpec const
        > hosting_requirements
    );
};

}
