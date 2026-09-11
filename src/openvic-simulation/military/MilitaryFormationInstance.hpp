#pragma once

#include <string_view>

#include "openvic-simulation/core/memory/String.hpp"
#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/military/MilitarySupport.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

struct MilitarySupportRelationship {
    std::reference_wrapper<
        MilitarySupportTypeDefinition const
    > support_type;

    memory::string target_id;
};

struct MilitaryFormationInstance {
    friend struct MilitaryFormationInstanceManager;

private:
    memory::string PROPERTY(name);

    MilitaryFormationDefinition const&
        formation_definition;

    fixed_point_t PROPERTY(readiness);

    /*
     * Current operational placement.
     *
     * direct_position_id references an existing canonical
     * location identity. It does not duplicate spatial state.
     *
     * hosted_by_unique_id identifies another runtime military
     * formation instance whose effective position is inherited.
     *
     * Zero means no host.
     */
    memory::string direct_position_id;
    unique_id_t hosted_by_unique_id = 0;

    /*
     * Persistent support relationships are independent from
     * operational placement and hosting.
     *
     * A formation may have zero, one, or many concurrent support
     * relationships.
     */
    memory::vector<
        MilitarySupportRelationship
    > support_relationships;

public:
    const unique_id_t unique_id;

    MilitaryFormationInstance(
        unique_id_t new_unique_id,
        std::string_view new_name,
        MilitaryFormationDefinition const&
            new_formation_definition,
        fixed_point_t new_readiness
    );

    MilitaryFormationInstance(
        MilitaryFormationInstance&&
    ) = default;

    [[nodiscard]]
    MilitaryFormationDefinition const&
    get_formation_definition() const {
        return formation_definition;
    }

    [[nodiscard]]
    MilitaryDomainDefinition const&
    get_domain() const {
        return formation_definition.get_domain();
    }

    [[nodiscard]]
    bool has_capability(
        MilitaryCapabilityDefinition const&
            capability
    ) const {
        return formation_definition.
            has_capability(capability);
    }

    [[nodiscard]]
    bool has_direct_position() const {
        return !direct_position_id.empty();
    }

    [[nodiscard]]
    std::string_view
    get_direct_position_id() const {
        return direct_position_id;
    }

    [[nodiscard]]
    bool is_hosted() const {
        return hosted_by_unique_id != 0;
    }

    [[nodiscard]]
    unique_id_t
    get_host_unique_id() const {
        return hosted_by_unique_id;
    }

    [[nodiscard]]
    std::span<
        MilitarySupportRelationship const
    >
    get_support_relationships() const {
        return support_relationships;
    }

    [[nodiscard]]
    bool has_support_relationship(
        MilitarySupportTypeDefinition const&
            support_type,
        std::string_view target_id
    ) const;

    bool set_readiness(
        fixed_point_t new_readiness
    );

    void set_name(
        std::string_view new_name
    );

    constexpr bool operator==(
        MilitaryFormationInstance const& rhs
    ) const {
        return unique_id == rhs.unique_id;
    }
};

struct MilitaryFormationInstanceManager {
private:
    memory::vector<
        MilitaryFormationInstance
    > PROPERTY_REF(military_formation_instances);

    unique_id_t next_unique_id = 1;

    [[nodiscard]]
    bool would_create_host_cycle(
        unique_id_t guest_unique_id,
        unique_id_t proposed_host_unique_id
    ) const;

public:
    bool create_military_formation_instance(
        std::string_view name,
        MilitaryFormationDefinition const&
            formation_definition,
        fixed_point_t readiness = 1
    );

    [[nodiscard]]
    MilitaryFormationInstance const*
    get_military_formation_instance_by_unique_id(
        unique_id_t unique_id
    ) const;

    [[nodiscard]]
    MilitaryFormationInstance*
    get_military_formation_instance_by_unique_id(
        unique_id_t unique_id
    );

    [[nodiscard]]
    size_t
    get_military_formation_instance_count() const {
        return military_formation_instances.size();
    }

    /*
     * Direct placement and hosting are alternative representations
     * of current operational position.
     *
     * Setting a direct position detaches the instance from any host.
     * Hosting an instance clears its direct position.
     */
    bool set_direct_operational_position(
        unique_id_t formation_unique_id,
        std::string_view position_id
    );

    bool host_formation(
        unique_id_t guest_unique_id,
        unique_id_t host_unique_id
    );

    bool detach_formation(
        unique_id_t guest_unique_id
    );

    bool add_support_relationship(
        unique_id_t formation_unique_id,
        MilitarySupportTypeDefinition const&
            support_type,
        std::string_view target_id
    );

    bool remove_support_relationship(
        unique_id_t formation_unique_id,
        MilitarySupportTypeDefinition const&
            support_type,
        std::string_view target_id
    );

    [[nodiscard]]
    std::string_view
    get_effective_operational_position_id(
        unique_id_t formation_unique_id
    ) const;
};

}
