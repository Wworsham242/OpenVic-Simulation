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
