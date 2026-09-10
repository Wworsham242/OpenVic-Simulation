# PROJECT-CONVERGENCE-005A5 — Optional Population Capability Contract

## Purpose

005A5 proves that one existing non-age-sex population mechanism can be absent without replacing or weakening the authoritative OpenVic population and employment state.

The selected capability is persistent nutrition-health burden.

The increment does not generalize the entire `Pop` class.

## Native Repository Basis

OpenVic population authority currently remains centered on `Pop`.

Important authoritative state includes:

- `Pop::size`;
- `Pop::employed`;
- `Pop::get_unemployed()`;
- `Pop::hire()`;
- province POP collections;
- POP aggregation;
- the workforce adapters and employment-allocation mechanisms already built around those POPs.

Those mechanisms remain unchanged.

Before 005A5, every `Pop` also carried:

- `nutrition_health_burden`;
- `last_nutrition_health_burden_update`.

Those fields represented history-dependent 004A5 health state even though not every possible game or scenario requires that health capability.

## External Reference Review

No new health model is introduced.

005A5 reuses the existing 004A5 mechanism and its prior calibration:

- nutrition-health deterioration characteristic time: 30 days;
- nutrition-health recovery characteristic time: 60 days.

Those values remain simulation calibration anchors rather than universal biological constants.

The architectural question in 005A5 is capability presence, not biological recalibration.

## Chosen Mechanism

005A5 introduces `OptionalNutritionHealthCapability`.

The capability owns only:

- whether the nutrition-health mechanism is enabled;
- persistent nutrition-health burden;
- the most recent nutrition-health update.

It does not own:

- population count;
- employment count;
- workforce allocation;
- needs acquisition;
- cash;
- market demand;
- province membership;
- demographic age-sex structure.

When disabled:

- persistent nutrition-health state is absent;
- nutrition-health updates are skipped;
- burden reads as the neutral zero value;
- the nullable latest-update accessor returns null.

When enabled:

- the existing `update_nutrition_health_burden()` mechanism remains authoritative for this capability.

## Construction Contract

`PopDeps` now carries:

`enable_nutrition_health_capability`

Existing OpenVic-compatible construction defaults this value to `true`.

This preserves existing behavior.

General/native callers can explicitly set the capability to `false`.

The `Pop` constructor materializes nutrition-health state only when requested.

This is a bounded capability switch, not a universal package framework.

## Calibration Status

Existing 004A5 calibration is retained unchanged.

No coefficient, threshold, response time, probability, or biological assumption is added or modified by 005A5.

Calibration status therefore remains the same as 004A5.

## Causal Integration

### Existing enabled path

life-needs fulfillment

→ survival-needs stress

→ basic-resource health-vulnerability pressure

→ nutrition-health burden update

→ stored nutrition-health history.

### Disabled path

life-needs fulfillment and all other POP behavior continue normally.

The nutrition-health capability consumes no pressure and stores no update history.

Disabling the capability does not modify population size or employment.

## Authoritative-State Boundary

005A5 creates no second population ledger.

The authoritative population total remains `Pop::size`.

The authoritative employment state remains the existing POP employment accounting.

`Pop::hire()` remains the mutation authority used by workforce allocation.

The optional nutrition-health object is supplemental capability state attached to an existing POP.

## Integration Proof

The existing real-POP economy fixture was extended so a `Pop` can be constructed with nutrition-health capability explicitly disabled.

The integration test constructs a real 40-person POP with the capability disabled and proves:

- nutrition-health capability is absent;
- burden remains neutral;
- no latest nutrition update exists;
- authoritative POP size remains 40;
- initial unemployed population remains 40;
- `Pop::hire(15)` still succeeds through the existing authority;
- POP size remains 40;
- unemployed population becomes 25;
- nutrition-health capability remains absent.

Therefore optional health state does not replace, shadow, or interfere with population/employment authority.

## Legacy Compatibility

Existing callers of aggregate initialization:

`PopDeps { artisan_deps, market, aggregates }`

continue to enable nutrition-health behavior because the new field defaults to true.

Existing 004A5 tests remain green.

Thus 005A5 does not silently remove health behavior from legacy OpenVic-compatible simulation.

## Remaining Mandatory Population Dimensions

005A5 deliberately does not claim that OpenVic population is now fully generic.

`PopBase` and `Pop` still require or contain substantial inherited ontology, including:

- `PopType`;
- culture;
- religion;
- militancy;
- consciousness;
- rebel type;
- province location;
- strata-related behavior;
- ideology/political support;
- reform/policy support;
- military regiment support;
- artisan/economic compatibility behavior.

These remain future generalization boundaries.

The existence of one optional capability must not be misrepresented as complete POP-schema generalization.

## Victoria-Specific Boundary

005A5 does not remove or reinterpret Victoria-specific POP dimensions.

It isolates only one capability that was added by the convergence work and that does not need to be universal.

The inherited POP ontology remains intact for compatibility.

## Modern-Specific Boundary

Nutrition-health burden is also not declared universal merely because it is useful in a modern-world simulation.

A Bronze Age scenario, military-operational scenario, abstract economic scenario, or another game may choose to enable it, disable it, or later supply a different population-health capability.

The generic lesson is capability optionality, not mandatory modern health simulation.

## Determinism

When enabled, the existing deterministic fixed-point nutrition-health update remains unchanged.

When disabled, update calls are inert and do not mutate capability state.

No RNG, new cadence, asynchronous process, or alternate population transition path is introduced.

## Validation

005A5 validation includes:

- focused optional-capability unit tests;
- real `Pop` integration test with capability disabled;
- actual `[economy][native-workforce]` regression suite;
- existing `[convergence][004a5]` nutrition regressions;
- existing `[convergence][geography][grouping]` regressions;
- full CTest;
- `git diff --check`;
- exact staged-file verification;
- local/remote HEAD equality after push.

The earlier attempted `[convergence][workforce]` selector matched zero tests and is not counted as validation evidence.

## Scope Boundary

005A5 does not:

- replace `Pop`;
- replace `PopBase`;
- replace province POP storage;
- alter `Pop::size`;
- alter `Pop::hire()`;
- alter employment allocation;
- alter workforce conservation;
- alter POP aggregation;
- alter age-sex demographics;
- add demographic evolution;
- add mortality;
- add disease simulation;
- change nutrition-health calibration;
- create a generic population registry;
- create a second population store;
- generalize all Victoria POP dimensions.

## Result

005A5 proves a narrow but important rule:

> Population mechanisms may be optional capabilities attached to authoritative population state rather than mandatory fields of every population representation.

This establishes the capability boundary without destabilizing the existing population, employment, economy, or demographic authority.
