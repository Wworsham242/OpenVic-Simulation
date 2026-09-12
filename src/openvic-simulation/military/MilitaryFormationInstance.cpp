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
    sustainment { fixed_point_t::_1 },
    equipment_condition { fixed_point_t::_1 },
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

bool MilitaryFormationInstance::
set_equipment_condition(
    fixed_point_t new_equipment_condition
) {
    if (
        new_equipment_condition < 0 ||
        new_equipment_condition > 1
    ) {
        spdlog::error_s(
            "Military formation instance {} "
            "equipment condition must be between "
            "0 and 1.",
            unique_id
        );

        return false;
    }

    equipment_condition =
        new_equipment_condition;

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

bool MilitaryFormationInstance::
has_support_relationship(
    MilitarySupportTypeDefinition const&
        support_type,
    std::string_view target_id
) const {
    return std::ranges::any_of(
        support_relationships,
        [&support_type, target_id](
            MilitarySupportRelationship const&
                relationship
        ) {
            return
                &relationship.support_type.get() ==
                    &support_type &&
                relationship.target_id ==
                    target_id;
        }
    );
}

bool MilitaryFormationInstanceManager::
add_support_relationship(
    unique_id_t formation_unique_id,
    MilitarySupportTypeDefinition const&
        support_type,
    std::string_view target_id
) {
    if (target_id.empty()) {
        spdlog::error_s(
            "Cannot add military support "
            "relationship with empty target ID."
        );

        return false;
    }

    MilitaryFormationInstance* const formation =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot add support relationship "
            "to unknown military formation {}.",
            formation_unique_id
        );

        return false;
    }

    if (
        formation->has_support_relationship(
            support_type,
            target_id
        )
    ) {
        spdlog::error_s(
            "Military formation {} already has "
            "support relationship {} -> {}.",
            formation_unique_id,
            support_type,
            target_id
        );

        return false;
    }

    formation->support_relationships.push_back(
        MilitarySupportRelationship {
            .support_type = support_type,
            .target_id = memory::string {
                target_id
            }
        }
    );

    return true;
}

bool MilitaryFormationInstanceManager::
remove_support_relationship(
    unique_id_t formation_unique_id,
    MilitarySupportTypeDefinition const&
        support_type,
    std::string_view target_id
) {
    MilitaryFormationInstance* const formation =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot remove support relationship "
            "from unknown military formation {}.",
            formation_unique_id
        );

        return false;
    }

    auto const it = std::ranges::find_if(
        formation->support_relationships,
        [&support_type, target_id](
            MilitarySupportRelationship const&
                relationship
        ) {
            return
                &relationship.support_type.get() ==
                    &support_type &&
                relationship.target_id ==
                    target_id;
        }
    );

    if (
        it ==
        formation->support_relationships.end()
    ) {
        spdlog::error_s(
            "Military formation {} does not have "
            "support relationship {} -> {}.",
            formation_unique_id,
            support_type,
            target_id
        );

        return false;
    }

    formation->support_relationships.erase(it);

    return true;
}
bool MilitaryFormationInstanceManager::
evaluate_support_sustainment(
    unique_id_t formation_unique_id,
    std::function<
        fixed_point_t(std::string_view)
    > const& support_availability_provider
) {
    MilitaryFormationInstance* const formation =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot evaluate support sustainment "
            "for unknown military formation {}.",
            formation_unique_id
        );

        return false;
    }

    auto const required_support_types =
        formation->
            get_formation_definition().
            get_required_support_types();

    /*
     * No declared support dependency means no mandatory support
     * penalty. This is what permits an austere force to exist
     * without pretending it owns a sophisticated base network.
     */
    if (required_support_types.empty()) {
        formation->sustainment =
            fixed_point_t::_1;

        return true;
    }

    fixed_point_t overall =
        fixed_point_t::_1;

    for (
        auto const& required_reference :
        required_support_types
    ) {
        MilitarySupportTypeDefinition const&
            required_type =
                required_reference.get();

        fixed_point_t best_available = 0;

        for (
            MilitarySupportRelationship const&
                relationship :
            formation->support_relationships
        ) {
            if (
                &relationship.support_type.get() !=
                &required_type
            ) {
                continue;
            }

            fixed_point_t const availability =
                support_availability_provider(
                    relationship.target_id
                );

            if (
                availability < 0 ||
                availability > 1
            ) {
                spdlog::error_s(
                    "Support availability provider "
                    "returned {} outside [0,1] "
                    "for target {}.",
                    availability,
                    relationship.target_id
                );

                return false;
            }

            if (
                availability >
                best_available
            ) {
                best_available =
                    availability;
            }
        }

        /*
         * Redundant nodes of the same required type substitute for
         * one another, so use the best available linked node.
         *
         * Different required support types are complementary
         * bottlenecks, so the weakest required type constrains
         * overall sustainment.
         */
        if (
            best_available <
            overall
        ) {
            overall =
                best_available;
        }
    }

    formation->sustainment = overall;

    return true;
}
bool MilitaryFormationInstanceManager::
adjust_readiness_toward(
    unique_id_t formation_unique_id,
    fixed_point_t desired_readiness,
    fixed_point_t max_adjustment
) {
    MilitaryFormationInstance* const formation =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot adjust readiness for unknown "
            "military formation {}.",
            formation_unique_id
        );

        return false;
    }

    if (
        desired_readiness < 0 ||
        desired_readiness > 1
    ) {
        spdlog::error_s(
            "Military formation {} desired readiness "
            "{} is outside [0,1].",
            formation_unique_id,
            desired_readiness
        );

        return false;
    }

    if (
        max_adjustment <= 0 ||
        max_adjustment > 1
    ) {
        spdlog::error_s(
            "Military formation {} readiness "
            "adjustment {} must be within (0,1].",
            formation_unique_id,
            max_adjustment
        );

        return false;
    }

    /*
     * Sustainment is a ceiling, not the readiness value itself.
     *
     * Other mechanisms may request a lower readiness target. That
     * lower target remains authoritative for this transition.
     */
    fixed_point_t effective_target =
        desired_readiness <
            formation->sustainment
        ? desired_readiness
        : formation->sustainment;

    if (
        formation->equipment_condition <
        effective_target
    ) {
        effective_target =
            formation->equipment_condition;
    }

    if (
        formation->readiness ==
        effective_target
    ) {
        return true;
    }

    if (
        formation->readiness <
        effective_target
    ) {
        fixed_point_t const gap =
            effective_target -
            formation->readiness;

        fixed_point_t const adjustment =
            gap < max_adjustment
            ? gap
            : max_adjustment;

        formation->readiness +=
            adjustment;

        return true;
    }

    fixed_point_t const gap =
        formation->readiness -
        effective_target;

    fixed_point_t const adjustment =
        gap < max_adjustment
        ? gap
        : max_adjustment;

    formation->readiness -=
        adjustment;

    return true;
}
bool MilitaryFormationInstanceManager::
evaluate_equipment_condition(
    unique_id_t formation_unique_id,
    std::function<
        fixed_point_t(std::string_view)
    > const& equipment_quantity_provider
) {
    MilitaryFormationInstance* const formation =
        get_military_formation_instance_by_unique_id(
            formation_unique_id
        );

    if (formation == nullptr) {
        spdlog::error_s(
            "Cannot evaluate equipment condition for "
            "unknown military formation {}.",
            formation_unique_id
        );

        return false;
    }

    auto const requirements =
        formation->
            formation_definition.
            get_equipment_requirements();

    /*
     * Equipment mechanics remain optional.
     *
     * A formation with no declared equipment requirements is not
     * forced into an equipment model.
     */
    if (requirements.empty()) {
        formation->equipment_condition =
            fixed_point_t::_1;

        return true;
    }

    fixed_point_t candidate =
        fixed_point_t::_1;

    for (
        MilitaryEquipmentRequirement const&
            requirement :
        requirements
    ) {
        fixed_point_t const available =
            equipment_quantity_provider(
                requirement.item_id
            );

        if (available < 0) {
            spdlog::error_s(
                "Equipment quantity provider returned "
                "negative quantity {} for item {}.",
                available,
                requirement.item_id
            );

            return false;
        }

        fixed_point_t availability_fraction =
            available /
            requirement.required_quantity;

        if (
            availability_fraction >
            fixed_point_t::_1
        ) {
            availability_fraction =
                fixed_point_t::_1;
        }

        if (
            availability_fraction <
            candidate
        ) {
            candidate =
                availability_fraction;
        }
    }

    formation->equipment_condition =
        candidate;

    return true;
}