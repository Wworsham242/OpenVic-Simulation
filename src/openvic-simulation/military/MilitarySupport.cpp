#include "MilitarySupport.hpp"

#include "openvic-simulation/utility/Logger.hpp"

using namespace OpenVic;

MilitarySupportTypeDefinition::
MilitarySupportTypeDefinition(
    std::string_view new_identifier
) :
    HasIdentifier { new_identifier } {}

bool MilitarySupportManager::
add_military_support_type(
    std::string_view identifier
) {
    if (identifier.empty()) {
        spdlog::error_s(
            "Invalid military support type "
            "identifier - empty!"
        );

        return false;
    }

    return military_support_types.emplace_item(
        identifier,
        identifier
    );
}
