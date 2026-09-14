# PROJECT-CONVERGENCE-006B2.1 — Stateful Soil-Water Balance

## Purpose

Introduce the first stateful physical environmental mechanism beneath the
existing province environmental and agricultural seams.

The increment converts explicit water forcing into persistent aggregate
soil-water state.

It does not create another resource network and does not replace the existing
agricultural production constraint.

## Repository Reconciliation

At the start of 006B2 the environmental module contains only:

- `ProvinceEnvironmentalState`;
- `ProvinceEnvironmentalObservation`.

`ProvinceEnvironmentalState` owns one physical value:

    water_availability in [0, 1]

described as water available to vegetation as a fraction of sufficient water.

The existing comment explicitly states that this value does not itself model
rainfall or irrigation.

`AgriculturalProductionConstraint` consumes that value before native farm output
enters the existing economy.

The generic resource subsystem already provides:

- physical source capability;
- availability fractions;
- multiple sources;
- logistics/access constraints;
- buffering;
- delivered quantity;
- explicit unmet demand.

Therefore B2.1 does not introduce another generic water-supply network.

`ProductiveSiteUtilityResolver` also already exposes industrial water as a
non-storable productive-site utility seam.

The missing first layer is physical environmental evolution.

## Empirical Basis

FAO root-zone soil-water methodology represents water content as a stateful
balance.

Rainfall adds water.

Surface runoff removes water before it can contribute to stored soil water.

Evapotranspiration removes water from the root zone.

Water in excess of storage capacity leaves through deep percolation.

The state persists from one interval to the next.

References:

- FAO Irrigation and Drainage Paper 56, Chapter 8:
  https://www.fao.org/4/X0490E/x0490e0e.htm

- FAO effective-rainfall guidance:
  https://www.fao.org/4/r4082e/r4082e05.htm

B2.1 uses the accounting structure of this physical method but does not claim
crop-calibrated agronomy.

## State

`ProvinceSoilWaterState` contains:

    capacity
    storage

Both use one caller-selected consistent water-depth/quantity unit.

Validity requires:

    capacity > 0
    0 <= storage <= capacity

The state is persistent across transitions.

## Forcing

`ProvinceSoilWaterForcing` contains:

    precipitation
    surface_runoff
    evapotranspiration_demand

All are explicit nonnegative physical quantities for one caller-selected
interval.

The forcing does not define its own SimTime cadence.

The caller may supply daily, weekly or another appropriately calibrated
interval.

## Transition

The mechanism computes:

    effective_precipitation =
        precipitation - surface_runoff

    available_before_losses =
        prior_storage + effective_precipitation

    actual_evapotranspiration =
        min(
            available_before_losses,
            evapotranspiration_demand
        )

    evapotranspiration_deficit =
        evapotranspiration_demand
        - actual_evapotranspiration

    post_ET_water =
        available_before_losses
        - actual_evapotranspiration

    deep_drainage =
        max(
            post_ET_water - capacity,
            0
        )

    ending_storage =
        min(
            post_ET_water,
            capacity
        )

The resulting aggregate vegetation-water availability is:

    ending_storage / capacity

and is exported through the existing:

    ProvinceEnvironmentalState

rather than introducing a second agricultural environmental representation.

## Delayed-Effect Property

Because storage persists, environmental effects have physical memory.

Repeated dry intervals progressively deplete soil water.

The return of precipitation does not automatically restore full water
availability.

This supports the project requirement that second-order effects may remain
after the headline environmental event has ended.

For example:

    precipitation shortfall
        -> declining soil-water storage
        -> lower vegetation water availability
        -> reduced agricultural output
        -> food inventory / market consequences later

The delayed consequence emerges from stored state rather than a lingering
scripted drought modifier.

## Causal Boundary

B2.1 does not implement:

- stochastic or historical weather generation;
- temperature;
- snowpack;
- irrigation;
- groundwater;
- aquifers;
- reservoirs;
- watershed routing;
- river discharge;
- municipal water;
- sanitation;
- water rights;
- infrastructure damage;
- crop-specific root depth;
- crop-specific stress thresholds;
- disease or pests;
- soil chemistry;
- erosion;
- land use.

These belong to later mechanisms when required by a concrete vertical.

## Irrigation Boundary

Irrigation is deliberately excluded even though real soil-water balance methods
include it.

Irrigation is not exogenous rainfall.

It is a human allocation of water that must eventually depend on physical water
sources, storage, infrastructure, access and policy.

Adding arbitrary irrigation directly to B2.1 would bypass those causal systems.

A later water-allocation mechanism may provide irrigation water to this
physical balance through an explicit coupling.

## Existing Agriculture Boundary

B2.1 does not alter the current aggregate agricultural response function.

For now:

    soil-water balance
        -> ProvinceEnvironmentalState
        -> existing AgriculturalProductionConstraint

The existing agricultural constraint remains intentionally coarse.

Crop-specific sensitivity can be added later without changing the physical
water-state kernel.

## Acceptance

Tests prove:

1. zero forcing preserves state;
2. precipitation recharges storage;
3. evapotranspiration depletes storage;
4. water shortage produces explicit evapotranspiration deficit;
5. excess water drains after current-interval evapotranspiration;
6. repeated dry intervals preserve drought memory;
7. rainfall recovery need not immediately restore full availability;
8. derived state feeds the existing agricultural constraint;
9. impossible runoff forcing is rejected transactionally;
10. invalid soil-water state is rejected;
11. identical inputs produce identical outputs.

## Next Boundary

After B2.1 the next environmental question is not another agriculture modifier.

The next increment should evaluate whether to add:

    weather forcing
        ->
    soil-water balance

or:

    soil-water / hydrologic state
        ->
    existing generic resource-source availability

depending on which produces the stronger first cross-domain certification
vertical without duplicating existing resource infrastructure.