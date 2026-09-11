#include "MilitaryFormationInstance.hpp"

#include <algorithm>

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MilitaryFormationInstance::
MilitaryFormationInstance(
    unique_id_t new_unique_id,
    std::string_view new_name,
    MilitaryFormationDefinition const&
        new_formation_definition,
    fixed_point_t new_readiness
) :
    name { new_name },
    formation_definition {
        new_formation_definition
    },
    readiness { new_readiness },
    unique_id { new_unique_id } {}

bool MilitaryFormationInstance::set_readiness(
    fixed_point_t new_readiness
) {
    if (
        new_readiness < 0 ||
        new_readiness > 1
    ) {
        spdlog::error_s(
            "Military formation instance {} "
            "readiness must be between 0 and 1.",
            unique_id
        );

        return false;
    }

    readiness = new_readiness;
    return true;
}

void MilitaryFormationInstance::set_name(
    std::string_view new_name
) {
    name = new_name;
}

bool MilitaryFormationInstanceManager::
create_military_formation_instance(
    std::string_view name,
    MilitaryFormationDefinition const&
        formation_definition,
    fixed_point_t readiness
) {
    if (name.empty()) {
        spdlog::error_s(
            "Cannot create military formation "
            "instance with empty name."
        );

        return false;
    }

    if (
        readiness < 0 ||
        readiness > 1
    ) {
        spdlog::error_s(
            "Cannot create military formation "
            "instance {} with readiness outside "
            "[0,1].",
            name
        );

        return false;
    }

    military_formation_instances.emplace_back(
        next_unique_id,
        name,
        formation_definition,
        readiness
    );

    ++next_unique_id;

    return true;
}

MilitaryFormationInstance const*
MilitaryFormationInstanceManager::
get_military_formation_instance_by_unique_id(
    unique_id_t unique_id
) const {
    auto const it = std::ranges::find_if(
        military_formation_instances,
        [unique_id](
            MilitaryFormationInstance const&
                instance
        ) {
            return
                instance.unique_id ==
                unique_id;
        }
    );

    if (
        it ==
        military_formation_instances.end()
    ) {
        return nullptr;
    }

    return &*it;
}

MilitaryFormationInstance*
MilitaryFormationInstanceManager::
get_military_formation_instance_by_unique_id(
    unique_id_t unique_id
) {
    auto const it = std::ranges::find_if(
        military_formation_instances,
        [unique_id](
            MilitaryFormationInstance const&
                instance
        ) {
            return
                instance.unique_id ==
                unique_id;
        }
    );

    if (
        it ==
        military_formation_instances.end()
    ) {
        return nullptr;
    }

    return &*it;
}

bool MilitaryFormationInstanceManager::
set_direct_operational_position(
    unique_id_t formation_unique_id,
    std::string_view position_id
) {
    if (position_id.empty()) {
        spdlog::error_s(
            "Cannot directly place military "
            "formation instance {} at an empty "
            "position identifier.",
            formation_unique_id
        );

        return false;
    }

    MilitaryFormationInstance* const formation =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot place unknown military "
            "formation instance {}.",
            formation_unique_id
        );

        return false;
    }

    formation->hosted_by_unique_id = 0;
    formation->direct_position_id = position_id;

    return true;
}

bool MilitaryFormationInstanceManager::
would_create_host_cycle(
    unique_id_t guest_unique_id,
    unique_id_t proposed_host_unique_id
) const {
    unique_id_t current =
        proposed_host_unique_id;

    size_t remaining =
        military_formation_instances.size();

    while (
        current != 0 &&
        remaining > 0
    ) {
        if (current == guest_unique_id) {
            return true;
        }

        MilitaryFormationInstance const*
            const instance =
                get_military_formation_instance_by_unique_id(
                    current
                );

        if (instance == nullptr) {
            return false;
        }

        current =
            instance->hosted_by_unique_id;

        --remaining;
    }

    /*
     * Exhausting the maximum possible chain length while still
     * having a host means pre-existing corrupted cyclic state.
     * Reject the mutation rather than extending it.
     */
    return current != 0;
}

bool MilitaryFormationInstanceManager::
host_formation(
    unique_id_t guest_unique_id,
    unique_id_t host_unique_id
) {
    if (guest_unique_id == host_unique_id) {
        spdlog::error_s(
            "Military formation instance {} "
            "cannot host itself.",
            guest_unique_id
        );

        return false;
    }

    MilitaryFormationInstance* const guest =
        get_military_formation_instance_by_unique_id(
            guest_unique_id
        );

    MilitaryFormationInstance const* const host =
        get_military_formation_instance_by_unique_id(
            host_unique_id
        );

    if (
        guest == nullptr ||
        host == nullptr
    ) {
        spdlog::error_s(
            "Cannot create military hosting "
            "relationship guest={} host={} because "
            "one or both instances do not exist.",
            guest_unique_id,
            host_unique_id
        );

        return false;
    }

    if (
        would_create_host_cycle(
            guest_unique_id,
            host_unique_id
        )
    ) {
        spdlog::error_s(
            "Military hosting relationship "
            "guest={} host={} would create a cycle.",
            guest_unique_id,
            host_unique_id
        );

        return false;
    }

    /*
     * Hosting contracts are optional.
     *
     * If the guest declares no requirements, the generic 005A11
     * hosting behavior remains available.
     *
     * Once a guest declares requirements, every requirement must
     * be supplied by the proposed host with sufficient remaining
     * capacity.
     */
    auto const requirements =
        guest->
            get_formation_definition().
            get_hosting_requirements();

    for (
        MilitaryHostingRequirement const&
            requirement :
        requirements
    ) {
        MilitaryHostingProvision const*
            const provision =
                host->
                    get_formation_definition().
                    get_hosting_provision(
                        requirement.profile.get()
                    );

        if (provision == nullptr) {
            spdlog::error_s(
                "Military formation instance {} "
                "cannot host {} because required "
                "hosting profile {} is unavailable.",
                host_unique_id,
                guest_unique_id,
                requirement.profile.get()
            );

            return false;
        }

        fixed_point_t used_capacity = 0;

        for (
            MilitaryFormationInstance const&
                existing_guest :
            military_formation_instances
        ) {
            if (
                existing_guest.unique_id ==
                    guest_unique_id ||
                existing_guest.
                    hosted_by_unique_id !=
                    host_unique_id
            ) {
                continue;
            }

            MilitaryHostingRequirement const*
                const existing_requirement =
                    existing_guest.
                        get_formation_definition().
                        get_hosting_requirement(
                            requirement.profile.get()
                        );

            if (
                existing_requirement != nullptr
            ) {
                used_capacity +=
                    existing_requirement->demand;
            }
        }

        if (
            used_capacity +
                requirement.demand >
            provision->capacity
        ) {
            spdlog::error_s(
                "Military formation instance {} "
                "has insufficient remaining "
                "capacity for hosting profile {} "
                "when attempting to host {}.",
                host_unique_id,
                requirement.profile.get(),
                guest_unique_id
            );

            return false;
        }
    }

    guest->direct_position_id.clear();
    guest->hosted_by_unique_id =
        host->unique_id;

    return true;
}

bool MilitaryFormationInstanceManager::
detach_formation(
    unique_id_t guest_unique_id
) {
    MilitaryFormationInstance* const guest =
        get_military_formation_instance_by_unique_id(
            guest_unique_id
        );

    if (guest == nullptr) {
        spdlog::error_s(
            "Cannot detach unknown military "
            "formation instance {}.",
            guest_unique_id
        );

        return false;
    }

    guest->hosted_by_unique_id = 0;

    return true;
}

std::string_view
MilitaryFormationInstanceManager::
get_effective_operational_position_id(
    unique_id_t formation_unique_id
) const {
    MilitaryFormationInstance const* current =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    size_t remaining =
        military_formation_instances.size();

    while (
        current != nullptr &&
        remaining > 0
    ) {
        if (current->has_direct_position()) {
            return
                current->get_direct_position_id();
        }

        if (!current->is_hosted()) {
            return {};
        }

        current =
            get_military_formation_instance_by_unique_id(
                current->get_host_unique_id()
            );

        --remaining;
    }

    return {};
}
