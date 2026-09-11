#pragma once

#include <string_view>

#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"

namespace OpenVic {

struct MilitarySupportTypeDefinition :
    HasIdentifier {

    explicit MilitarySupportTypeDefinition(
        std::string_view new_identifier
    );

    MilitarySupportTypeDefinition(
        MilitarySupportTypeDefinition&&
    ) = default;
};

struct MilitarySupportManager {
private:
    IdentifierRegistry<
        MilitarySupportTypeDefinition
    > IDENTIFIER_REGISTRY_CUSTOM_PLURAL(
        military_support_type,
        military_support_types
    );

public:
    bool add_military_support_type(
        std::string_view identifier
    );
};

}
