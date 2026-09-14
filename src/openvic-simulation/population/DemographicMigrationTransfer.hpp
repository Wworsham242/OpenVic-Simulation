#pragma once

#include <cstdint>

#include "openvic-simulation/population/DemographicPopulationTransition.hpp"

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006B1.5
 *
 * Conserved age/sex migration transfer.
 *
 * This is a demographic transfer/accounting seam, not a destination-choice
 * model.
 *
 * One explicit migrant structure is:
 *
 *     emigration from origin
 *     ==
 *     immigration to destination
 *
 * so migration cannot create or destroy world population.
 *
 * Later causal mechanisms may decide transfers from:
 *
 * - mobility push;
 * - destination utility;
 * - employment/income;
 * - housing;
 * - safety;
 * - family/social networks;
 * - legal restrictions;
 * - transport cost/capacity;
 * - actor-perceived information.
 *
 * Those mechanisms do not belong in this transfer substrate.
 */
struct DemographicMigrationTransfer final {
PopulationAgeSexStructure migrants {};

[[nodiscard]] constexpr bool valid() const {
return migrants.has_nonnegative_counts();
}

[[nodiscard]] constexpr int64_t get_total_migrants() const {
return migrants.get_total_population();
}

bool operator==(
DemographicMigrationTransfer const&
) const = default;
};

enum class demographic_migration_transfer_status_t : uint8_t {
APPLIED,
INVALID_ORIGIN_STATE,
INVALID_DESTINATION_STATE,
INVALID_TRANSFER,
ORIGIN_TRANSFER_REJECTED,
DESTINATION_TRANSFER_REJECTED,
CONSERVATION_MISMATCH
};

struct DemographicMigrationTransferResult final {
PopulationAgeSexStructure origin_starting {};
PopulationAgeSexStructure destination_starting {};

PopulationAgeSexStructure origin_ending {};
PopulationAgeSexStructure destination_ending {};

DemographicMigrationTransfer transfer {};

int64_t migrants = 0;

int64_t combined_starting_population = 0;
int64_t combined_ending_population = 0;

demographic_migration_transfer_status_t status =
demographic_migration_transfer_status_t::
INVALID_ORIGIN_STATE;

[[nodiscard]] constexpr bool valid() const {
return
status
== demographic_migration_transfer_status_t::APPLIED;
}

bool operator==(
DemographicMigrationTransferResult const&
) const = default;
};

[[nodiscard]] constexpr DemographicMigrationTransferResult
apply_demographic_migration_transfer(
PopulationAgeSexStructure const& origin,
PopulationAgeSexStructure const& destination,
DemographicMigrationTransfer const& transfer
) {
DemographicMigrationTransferResult result {
.origin_starting = origin,
.destination_starting = destination,
.origin_ending = origin,
.destination_ending = destination,
.transfer = transfer,
.migrants = transfer.get_total_migrants(),
.combined_starting_population =
origin.get_total_population()
+ destination.get_total_population()
};

result.combined_ending_population =
result.combined_starting_population;

if (!origin.has_nonnegative_counts()) {
result.status =
demographic_migration_transfer_status_t::
INVALID_ORIGIN_STATE;
return result;
}

if (!destination.has_nonnegative_counts()) {
result.status =
demographic_migration_transfer_status_t::
INVALID_DESTINATION_STATE;
return result;
}

if (!transfer.valid()) {
result.status =
demographic_migration_transfer_status_t::
INVALID_TRANSFER;
return result;
}

/*
 * Reuse 006B1.1 as the demographic component authority.
 *
 * Origin:
 *     emigration = transfer
 *
 * Destination:
 *     immigration = same transfer
 */
DemographicPopulationComponents origin_components {};
origin_components.emigration = transfer.migrants;

auto const origin_transition =
apply_demographic_population_components(
origin,
origin_components
);

if (!origin_transition.valid()) {
result.status =
demographic_migration_transfer_status_t::
ORIGIN_TRANSFER_REJECTED;
return result;
}

DemographicPopulationComponents destination_components {};
destination_components.immigration = transfer.migrants;

auto const destination_transition =
apply_demographic_population_components(
destination,
destination_components
);

if (!destination_transition.valid()) {
/*
 * This function is pure, so the origin calculation above has not
 * mutated authority. Keep both returned endings at their starting
 * states to preserve transactional semantics.
 */
result.status =
demographic_migration_transfer_status_t::
DESTINATION_TRANSFER_REJECTED;
return result;
}

result.origin_ending =
origin_transition.ending;

result.destination_ending =
destination_transition.ending;

result.combined_ending_population =
result.origin_ending.get_total_population()
+ result.destination_ending.get_total_population();

const int64_t expected_origin =
origin.get_total_population()
- result.migrants;

const int64_t expected_destination =
destination.get_total_population()
+ result.migrants;

if (
result.origin_ending.get_total_population()
!= expected_origin
|| result.destination_ending.get_total_population()
!= expected_destination
|| result.combined_ending_population
!= result.combined_starting_population
) {
result.origin_ending = origin;
result.destination_ending = destination;

result.combined_ending_population =
result.combined_starting_population;

result.status =
demographic_migration_transfer_status_t::
CONSERVATION_MISMATCH;

return result;
}

result.status =
demographic_migration_transfer_status_t::APPLIED;

return result;
}

}