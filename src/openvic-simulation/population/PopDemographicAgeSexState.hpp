#pragma once

#include "openvic-simulation/core/memory/SmartPtr.hpp"
#include "openvic-simulation/population/DemographicAgeSexProfile.hpp"

namespace OpenVic {
struct PopDemographicAgeSexState {
private:
memory::unique_ptr<PopulationAgeSexStructure> structure {};

public:
[[nodiscard]] bool has_structure() const {
return structure != nullptr;
}

[[nodiscard]] PopulationAgeSexStructure const*
get_structure_nullable() const {
return structure.get();
}

bool initialize(
DemographicAgeSexProfile const& profile,
pop_size_t authoritative_population
) {
if (structure != nullptr) {
return false;
}

DemographicAgeSexInitializationResult const result =
materialize_demographic_age_sex_profile(
profile,
authoritative_population
);

if (!result.valid) {
return false;
}

structure =
memory::make_unique<PopulationAgeSexStructure>(
result.structure
);

return true;
}

[[nodiscard]] bool is_consistent_with_population(
pop_size_t authoritative_population
) const {
return
structure != nullptr
&& structure->is_consistent_with_population(
authoritative_population
);
}
};
}
