# PROJECT-CONVERGENCE-006B2.2 — Stateful Environmental Food-Scarcity Certification

## Purpose

Certify that the stateful physical soil-water mechanism introduced by 006B2.1
composes with the existing native agriculture, market and population-needs
systems.

This increment introduces no second agriculture model, no second market, and no
scripted environmental outcome.

## Existing Downstream Vertical

The existing `EnvironmentalFoodNeeds` certification already demonstrates:

    ProvinceEnvironmentalState
        ->
    native RGO production
        ->
    market supply and trading
        ->
    POP food needs fulfillment

The native province environmental state is installed through the existing
`ProvinceInstance::set_environmental_state(...)` seam.

006B2.1 supplies the missing physical upstream mechanism.

## Certified Chain

006B2.2 joins the mechanisms as:

    explicit precipitation / evapotranspiration forcing
        ->
    persistent ProvinceSoilWaterState
        ->
    ProvinceEnvironmentalState
        ->
    native RGO agricultural output
        ->
    native market supply
        ->
    native POP food-needs fulfillment

No downstream quantity is directly assigned by the soil-water mechanism.

## Persistent Environmental Memory

Repeated dry intervals reduce stored soil water.

Because the storage state persists, a later production cycle receives lower
physical vegetation-water availability even though the agriculture system itself
contains no drought script.

This produces:

    environmental history
        ->
    physical state
        ->
    production effect

rather than:

    event flag
        ->
    arbitrary production penalty

## Delayed Recovery

The certification also proves that renewed precipitation need not immediately
restore normal agricultural output.

A previously depleted soil-water store can remain below capacity after rainfall
returns.

Therefore agricultural consequences may persist after the forcing event changes.

This establishes a basic physical mechanism for delayed second-order effects.

## Determinism

Identical initial soil-water state and identical environmental forcing produce:

- identical soil-water transition results;
- identical derived environmental state;
- identical native agriculture/market/population outcomes.

This preserves deterministic replay across the new cross-domain chain.

## Scope Boundary

006B2.2 does not add:

- weather generation;
- climate baselines;
- temperature;
- snow;
- irrigation;
- rivers;
- reservoirs;
- groundwater;
- municipal water;
- crop-specific agronomy;
- migration;
- political response.

Those remain separate mechanisms.

## Certification Progress

This advances the permanent Bronze Age environmental-stress scenario by
demonstrating an ordinary engine path through:

    environmental forcing
        ->
    hydrologic state
        ->
    agricultural production
        ->
    food availability
        ->
    population welfare

without requiring a scenario-specific scripted result.

It also provides the first concrete portion of the climate/environment-driven
migration certification chain.

## Acceptance

The increment must prove:

1. repeated dry intervals produce persistent soil-water depletion;
2. later depletion reduces real native RGO food output;
3. reduced RGO output propagates through the real market;
4. reduced market availability lowers real POP life-needs fulfillment;
5. rainfall recovery can leave production below the undisturbed baseline;
6. identical physical trajectories reproduce identical downstream results;
7. all existing tests continue to pass.

## Next Boundary

After this cross-domain certification, 006B2 may proceed upstream toward a
weather/climate forcing producer or laterally toward broader hydrologic resource
availability.

The next increment must not replace the now-certified physical state and native
food-market chain with direct modifiers.