# PROJECT-CONVERGENCE-004A3 — Food Scarcity to Lagged Survival-Needs Stress

## Purpose

004A3 extends the verified causal strand:

    environmental water stress
        -> farm physical output
        -> market food supply
        -> POP actual life-needs acquisition
        -> life-needs fulfillment
        -> lagged survival-needs stress

The new state represents accumulated exposure to failure to acquire
survival needs.

It is deliberately not direct mortality, famine classification,
migration, fertility change, disease, or political unrest.

## Real-world causal basis

Real food crises are not well represented by:

    one bad harvest -> immediate population deaths

The observable sequence is more commonly:

    food availability / affordability shock
        -> household food-consumption deficit
        -> coping behaviour and asset depletion
        -> persistent nutritional stress
        -> acute malnutrition and disease interaction
        -> fertility, morbidity, mortality, or migration effects

The IPC acute-food-insecurity framework distinguishes evidence concerning
food consumption and livelihood change from later nutritional and mortality
outcomes. It explicitly recognises temporal lag between deterioration in
food access and downstream demographic consequences.

WHO likewise describes wasting and other undernutrition as consequences of
inadequate nutrient intake and/or disease, with elevated mortality risk
arising as nutritional condition deteriorates.

Accordingly, 004A3 does not map today's food-market shortfall directly to
a death count.

References:

- IPC Acute Food Insecurity analytical framework:
  https://www.ipcinfo.org/ipc-manual-interactive/ipc-acute-food-insecurity-protocols/function-2-classify-severity-and-identify-key-drivers/protocol-21-converge-evidence-using-the-ipc-analytical-framework/en/

- IPC Technical Manual:
  https://www.ipcinfo.org/IPC/technical/manual/version3

- WHO — Malnutrition:
  https://www.who.int/news-room/fact-sheets/detail/malnutrition

## Native input

OpenVic already owns authoritative POP life-needs fulfillment:

    fulfillment =
        acquired life-needs quantity
        / desired life-needs quantity

004A2 proved that physical food scarcity lowers this value through the
native market and consumption systems.

004A3 reuses that state. It does not introduce a second food ledger.

## Daily deficit

The completed native fulfillment result is converted to:

    deficit_t =
        1 - clamp(fulfillment_t, 0, 1)

Therefore:

    fulfillment = 1.00 -> deficit = 0.00
    fulfillment = 0.75 -> deficit = 0.25
    fulfillment = 0.50 -> deficit = 0.50
    fulfillment = 0.00 -> deficit = 1.00

## Distributed-lag exposure

Population harm depends strongly on persistence.

Rather than storing an arbitrary daily history, 004A3 uses a compact
first-order distributed lag:

    exposure_t =
        exposure_(t-1)
        + (deficit_t - exposure_(t-1)) / 30

Exposure is clamped to [0, 1].

This is equivalent to an exponentially weighted accumulated exposure.

It has useful causal behaviour:

- one bad day creates very little accumulated exposure;
- persistent deprivation progressively raises exposure;
- greater deficits produce greater stress;
- restored life-needs fulfillment causes gradual recovery;
- no abrupt threshold creates instant famine or death.

The 30-day parameter is an initial model calibration timescale for acute
food-security persistence. It is not claimed to be a universal biological
constant and should ultimately be empirically calibrated.

## Timing

The POP submits its purchase order during its tick.

GoodMarket later clears the market and invokes the POP purchase callback,
which finalises acquired needs.

Therefore today's POP tick must consume the previous completed market
cycle's fulfillment result:

    day N food shortage
        -> day N market clearing
        -> day N+1 survival-stress update

This gives an explicit deterministic causal lag and prevents partially
cleared market state from influencing population response.

## Future demographic modelling

Survival-needs stress is an input to future models, not the model itself.

### Fertility

A modern fertility model should use age-specific fertility rates:

    births =
        sum_age(
            female_population_age
            * ASFR_age
            * socioeconomic_modifier
            * survival_stress_modifier
        )

Stress effects should be empirically calibrated.

### Mortality

Mortality should eventually use baseline demographic mortality plus
cause-specific excess hazards:

    mortality_hazard =
        baseline_age_specific_hazard
        + nutrition_hazard
        + disease_interaction
        + temperature_hazard
        + conflict_hazard
        + health_system_effect

IPC crude-death-rate observations can be used for calibration and
validation of extreme food crises, but an IPC mortality threshold must
not be misused as an equation converting food deficit directly into deaths.

### Migration

Migration should be a constrained household response.

Expected migration pressure should eventually depend on:

- expected destination income;
- employment prospects;
- food security;
- physical safety;
- transport cost and distance;
- border/legal accessibility;
- social networks;
- information;
- household assets;
- relief availability.

This matters because severe deprivation can simultaneously increase the
desire to migrate and reduce the household's ability to migrate.

### Population accounting

When the demographic domain is expanded, authoritative accounting remains:

    population_(t+1)
        = population_t
        + births
        - deaths
        + immigration
        - emigration

## Persistence boundary

`survival_needs_stress_exposure` is history-dependent authoritative state.

It cannot generally be reconstructed from only the most recent
life-needs-fulfillment value after loading a campaign, because the exposure
contains information about preceding days.

004A3 does not introduce or redesign campaign serialization. When native
POP campaign persistence is implemented or connected, the authoritative
exposure value must be serialized and restored with the POP.

`last_survival_needs_stress_update` is causal/provenance information. Exact
save/load inspector continuity may justify persisting it as well, but the
minimum state required to preserve simulation causality is
`survival_needs_stress_exposure`.
## Scope boundary

004A3 adds only accumulated survival-needs exposure.

It does not add:

- direct starvation deaths;
- fertility changes;
- births;
- migration;
- displacement;
- disease;
- malnutrition prevalence;
- calories;
- nutrient classes;
- age cohorts;
- militancy;
- government relief;
- humanitarian assistance;
- IPC phase classification.

Each of those requires its own native mechanism and empirical calibration.
