#pragma once

#include <algorithm>
#include <cstdint>

#include "openvic-simulation/environment/ProvinceEnvironmentalState.hpp"
#include "openvic-simulation/types/fixed_point/FixedPoint.hpp"

namespace OpenVic {

/*
 * PROJECT-CONVERGENCE-006B2.1
 *
 * Stateful aggregate soil-water balance.
 *
 * This is physical environmental state, not:
 *
 * - weather generation;
 * - irrigation allocation;
 * - reservoir operation;
 * - groundwater simulation;
 * - municipal water distribution;
 * - crop-specific agronomy.
 *
 * All water quantities use one caller-selected consistent depth/quantity unit
 * per transition. The mechanism itself does not impose a SimTime cadence.
 */
struct ProvinceSoilWaterState final {
fixed_point_t capacity = fixed_point_t::_1;
fixed_point_t storage = fixed_point_t::_1;

[[nodiscard]] bool is_valid() const {
return
capacity > fixed_point_t::_0
&& storage >= fixed_point_t::_0
&& storage <= capacity;
}

[[nodiscard]] fixed_point_t availability_fraction() const {
if (!is_valid()) {
return fixed_point_t::_0;
}

return std::clamp(
storage / capacity,
fixed_point_t::_0,
fixed_point_t::_1
);
}

[[nodiscard]] ProvinceEnvironmentalState
to_environmental_state() const {
return ProvinceEnvironmentalState {
availability_fraction()
};
}

bool operator==(ProvinceSoilWaterState const&) const = default;
};

/*
 * Exogenous physical forcing for one caller-defined interval.
 *
 * precipitation:
 *     total water arriving from precipitation
 *
 * surface_runoff:
 *     precipitation lost before entering soil storage
 *
 * evapotranspiration_demand:
 *     atmospheric/vegetation demand from the soil-water store
 *
 * Irrigation and groundwater contributions are intentionally absent.
 */
struct ProvinceSoilWaterForcing final {
fixed_point_t precipitation = fixed_point_t::_0;
fixed_point_t surface_runoff = fixed_point_t::_0;
fixed_point_t evapotranspiration_demand = fixed_point_t::_0;

[[nodiscard]] bool is_valid() const {
return
precipitation >= fixed_point_t::_0
&& surface_runoff >= fixed_point_t::_0
&& evapotranspiration_demand >= fixed_point_t::_0
&& surface_runoff <= precipitation;
}

[[nodiscard]] fixed_point_t effective_precipitation() const {
return is_valid()
? precipitation - surface_runoff
: fixed_point_t::_0;
}

bool operator==(ProvinceSoilWaterForcing const&) const = default;
};

enum class province_soil_water_balance_status_t : uint8_t {
APPLIED,
INVALID_STARTING_STATE,
INVALID_FORCING
};

struct ProvinceSoilWaterBalanceResult final {
ProvinceSoilWaterState starting {};
ProvinceSoilWaterState ending {};
ProvinceSoilWaterForcing forcing {};

fixed_point_t effective_precipitation = fixed_point_t::_0;

fixed_point_t actual_evapotranspiration = fixed_point_t::_0;
fixed_point_t evapotranspiration_deficit = fixed_point_t::_0;

fixed_point_t deep_drainage = fixed_point_t::_0;

ProvinceEnvironmentalState environmental_state {};

province_soil_water_balance_status_t status =
province_soil_water_balance_status_t::INVALID_STARTING_STATE;

[[nodiscard]] bool valid() const {
return status
== province_soil_water_balance_status_t::APPLIED;
}

bool operator==(ProvinceSoilWaterBalanceResult const&) const = default;
};

/*
 * Advance one explicit physical interval.
 *
 * Water accounting:
 *
 *     available_before_losses
 *         = prior_storage
 *         + precipitation
 *         - runoff
 *
 *     actual_evapotranspiration
 *         = min(available_before_losses, ET demand)
 *
 *     post_ET_water
 *         = available_before_losses
 *         - actual_evapotranspiration
 *
 *     deep_drainage
 *         = max(post_ET_water - capacity, 0)
 *
 *     ending_storage
 *         = min(post_ET_water, capacity)
 *
 * This ordering allows rainfall in the current interval to satisfy current
 * evapotranspiration before excess water drains below the represented store.
 */
[[nodiscard]] inline ProvinceSoilWaterBalanceResult
advance_province_soil_water_balance(
ProvinceSoilWaterState const& starting,
ProvinceSoilWaterForcing const& forcing
) {
ProvinceSoilWaterBalanceResult result {
.starting = starting,
.ending = starting,
.forcing = forcing,
.environmental_state =
starting.to_environmental_state()
};

if (!starting.is_valid()) {
result.status =
province_soil_water_balance_status_t::
INVALID_STARTING_STATE;
return result;
}

if (!forcing.is_valid()) {
result.status =
province_soil_water_balance_status_t::
INVALID_FORCING;
return result;
}

result.effective_precipitation =
forcing.effective_precipitation();

fixed_point_t const available_before_losses =
starting.storage
+ result.effective_precipitation;

result.actual_evapotranspiration =
std::min(
available_before_losses,
forcing.evapotranspiration_demand
);

result.evapotranspiration_deficit =
forcing.evapotranspiration_demand
- result.actual_evapotranspiration;

fixed_point_t const after_evapotranspiration =
available_before_losses
- result.actual_evapotranspiration;

result.deep_drainage =
std::max(
after_evapotranspiration
- starting.capacity,
fixed_point_t::_0
);

result.ending = ProvinceSoilWaterState {
.capacity = starting.capacity,
.storage = std::min(
after_evapotranspiration,
starting.capacity
)
};

result.environmental_state =
result.ending.to_environmental_state();

result.status =
province_soil_water_balance_status_t::APPLIED;

return result;
}

}