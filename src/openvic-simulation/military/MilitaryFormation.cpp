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
    >&& new_hosting_requirements,
    memory::vector<
        std::reference_wrapper<
            MilitarySupportTypeDefinition const
        >
    >&& new_required_support_types,
    memory::vector<
        MilitaryEquipmentRequirement
    >&& new_equipment_requirements
) :
    HasIdentifier { new_identifier },
    domain { new_domain },
    capabilities { std::move(new_capabilities) },
    hosting_provisions {
        std::move(new_hosting_provisions)
    },
    hosting_requirements {
        std::move(new_hosting_requirements)
    },
    required_support_types {
        std::move(new_required_support_types)
    },
    equipment_requirements {
        std::move(new_equipment_requirements)
    } {}

bool MilitaryFormationDefinition::
requires_support_type(
    MilitarySupportTypeDefinition const&
        support_type
) const {
    return std::ranges::any_of(
        required_support_types,
        [&support_type](
            auto const& existing
        ) {
            return
                &existing.get() ==
                &support_type;
        }
    );
}

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
    std::span<
        MilitarySupportTypeDefinition const*
            const
    > const no_required_support {};

    return add_military_formation(
        identifier,
        domain,
        new_capabilities,
        new_hosting_provisions,
        new_hosting_requirements,
        no_required_support
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
    > new_hosting_requirements,
    std::span<
        MilitarySupportTypeDefinition const*
            const
    > new_required_support_types
) {
    std::span<
        MilitaryEquipmentRequirementSpec const
    > const no_equipment_requirements {};

    return add_military_formation(
        identifier,
        domain,
        new_capabilities,
        new_hosting_provisions,
        new_hosting_requirements,
        new_required_support_types,
        no_equipment_requirements
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
    > new_hosting_requirements,
    std::span<
        MilitarySupportTypeDefinition const*
            const
    > new_required_support_types,
    std::span<
        MilitaryEquipmentRequirementSpec const
    > new_equipment_requirements
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

    memory::vector<
        std::reference_wrapper<
            MilitarySupportTypeDefinition const
        >
    > required_support_types;

    required_support_types.reserve(
        new_required_support_types.size()
    );

    for (
        MilitarySupportTypeDefinition const*
            support_type :
        new_required_support_types
    ) {
        if (support_type == nullptr) {
            spdlog::error_s(
                "Military formation {} has "
                "a null required support type.",
                identifier
            );

            return false;
        }

        bool const duplicate =
            std::ranges::any_of(
                required_support_types,
                [support_type](
                    auto const& existing
                ) {
                    return
                        &existing.get() ==
                        support_type;
                }
            );

        if (duplicate) {
            spdlog::error_s(
                "Military formation {} contains "
                "duplicate required support type {}.",
                identifier,
                *support_type
            );

            return false;
        }

        required_support_types.emplace_back(
            *support_type
        );
    }

    memory::vector<
        MilitaryEquipmentRequirement
    > equipment_requirements;

    equipment_requirements.reserve(
        new_equipment_requirements.size()
    );

    for (
        MilitaryEquipmentRequirementSpec const&
            requirement :
        new_equipment_requirements
    ) {
        if (requirement.item_id.empty()) {
            spdlog::error_s(
                "Military formation {} has an empty "
                "equipment requirement identifier.",
                identifier
            );

            return false;
        }

        if (requirement.required_quantity <= 0) {
            spdlog::error_s(
                "Military formation {} has non-positive "
                "equipment requirement {}.",
                identifier,
                requirement.item_id
            );

            return false;
        }

        bool const duplicate =
            std::ranges::any_of(
                equipment_requirements,
                [&requirement](
                    MilitaryEquipmentRequirement const&
                        existing
                ) {
                    return
                        existing.item_id ==
                        requirement.item_id;
                }
            );

        if (duplicate) {
            spdlog::error_s(
                "Military formation {} contains duplicate "
                "equipment requirement {}.",
                identifier,
                requirement.item_id
            );

            return false;
        }

        equipment_requirements.push_back(
            MilitaryEquipmentRequirement {
                .item_id =
                    memory::string {
                        requirement.item_id
                    },
                .required_quantity =
                    requirement.required_quantity
            }
        );
    }

    return military_formations.emplace_item(
        identifier,
        identifier,
        domain,
        std::move(capabilities),
        std::move(hosting_provisions),
        std::move(hosting_requirements),
        std::move(required_support_types),
        std::move(equipment_requirements)
    );
}
