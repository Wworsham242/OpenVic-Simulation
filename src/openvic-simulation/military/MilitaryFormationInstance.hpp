#pragma once

#include <string_view>

#include "openvic-simulation/core/memory/Vector.hpp"
#include "openvic-simulation/military/MilitaryFormation.hpp"
#include "openvic-simulation/types/UniqueId.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {

struct MilitaryFormationInstance {
private:
    memory::string PROPERTY(name);

    MilitaryFormationDefinition const&
        formation_definition;

    fixed_point_t PROPERTY(readiness);

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
};

}
