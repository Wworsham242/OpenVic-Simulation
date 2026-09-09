# PROJECT-CONVERGENCE-004A5 - Nutrition Health Burden

## Purpose

004A5 extends the causal chain:

    environmental water stress
        -> lower native farm output
        -> lower native food supply
        -> lower POP life-needs fulfillment
        -> lagged survival-needs stress
        -> nutrition-side health vulnerability
        -> accumulated nutritional health burden

004A5 creates the first history-dependent health state.

It still does not create:

- disease cases;
- malnutrition prevalence;
- deaths;
- births;
- migration;
- rebellion;
- war.

## Why a separate health burden exists

004A3 survival-needs stress records a history of failure to acquire
basic survival consumption.

That is an exposure state.

Health condition is not identical to food access.

A person or population can remain nutritionally impaired after food
availability improves, and infectious disease can worsen nutritional
condition independently of food availability.

IPC acute-malnutrition analysis therefore treats food consumption and
health status as interacting immediate causes of acute malnutrition.

WHO likewise describes wasting as commonly arising from inadequate
food intake and/or infectious disease.

004A5 therefore introduces a separate history-dependent nutritional
health burden rather than treating survival-needs stress itself as
malnutrition.

References:

- IPC Acute Malnutrition analytical framework:
  https://www.ipcinfo.org/ipc-manual-interactive/ipc-acute-malnutrition-protocols/function-2-classify-severity-and-identify-key-drivers/protocol-21-converge-evidence-using-the-ipc-analytical-framework/en/

- WHO Malnutrition:
  https://www.who.int/news-room/fact-sheets/detail/malnutrition/

- WHO Strategic framework for health and nutrition action during food crises:
  https://www.who.int/publications/i/item/9789240122703

## State

The authoritative stored health quantity is:

    nutrition_health_burden in [0,1]

It is history-dependent and therefore differs architecturally from the
004A4 `BasicResourceStressResponse`, which remains derived on demand.

The update also exposes causal facts:

    previous_burden
    health_vulnerability_pressure
    response_days
    daily_change
    burden

These make the transition inspectable and deterministic.

## Update equation

Let:

    B_t = prior nutritional health burden
    H_t = current 004A4 health vulnerability pressure

Both are clamped to [0,1].

For deterioration, when:

    H_t > B_t

the initial model uses:

    tau = 30 days

For recovery, when:

    H_t < B_t

the initial model uses:

    tau = 60 days

Then:

    delta_t = (H_t - B_t) / tau

and:

    B_(t+1) =
        clamp(B_t + delta_t, 0, 1)

When:

    H_t = B_t

the burden remains unchanged.

## Interpretation of the timescales

The 30-day deterioration and 60-day recovery values are initial
simulation calibration anchors.

They are not WHO clinical constants.

Their purpose is to encode three empirically appropriate structural
properties:

1. nutritional condition does not jump instantly to the current food
   deprivation level;
2. sustained deprivation accumulates health burden;
3. recovery need not occur at the same speed as deterioration.

WHO guidance demonstrates that wasting is a treatable condition and
that nutritional recovery is a process rather than an instantaneous
state change. The exact future rates should be calibrated separately by:

- age;
- severity;
- disease burden;
- treatment availability;
- food composition;
- maternal status;
- WASH;
- health-system capability.

## Why mortality is not implemented

IPC explicitly places acute malnutrition before mortality in the causal
sequence.

WHO also identifies undernutrition as increasing susceptibility to
disease and death rather than defining a universal deterministic
conversion from food shortage to death.

A credible future mortality model needs additional state not yet
available in native OpenVic, especially:

- age composition;
- baseline mortality;
- disease prevalence;
- health-system access;
- sanitation;
- treatment;
- pregnancy and maternal risk.

Therefore 004A5 does not modify POP size.

## Future health model

A later health domain can treat 004A5 burden as one causal input:

    nutritional burden
        + disease exposure
        + age
        + WASH
        + treatment access
        + health-system capacity
        -> morbidity / malnutrition state
        -> excess mortality hazard

This preserves the project's rule that every domain retains its own
native causal mechanism.

## Native authority

OpenVic remains authoritative for:

- POP size;
- life-needs consumption;
- militancy;
- employment;
- migration accounting.

004A5 adds only the missing history-dependent nutritional-health state.

It creates no second:

- food market;
- POP needs ledger;
- migration ledger;
- mortality ledger.

## Causal timing

The timing is:

    day N:
        food shortage clears through native market
        -> POP life-needs fulfillment is reduced

    day N+1 POP tick:
        004A3 consumes completed life-needs fulfillment
        -> survival-needs stress updates
        -> 004A5 consumes current health vulnerability
        -> nutritional health burden updates

This preserves the existing causal lag.

## Recovery

When food supply returns, native POP life-needs fulfillment recovers
first.

Because 004A3 survival stress contains memory, nutritional pressure can
remain elevated temporarily.

004A5 health burden can therefore continue rising briefly even after
food supply has been restored.

As survival stress decays and falls below health burden, the burden
enters recovery.

This is intentional causal hysteresis rather than an instant reset.

## Scope boundary

004A5 does not implement:

- age cohorts;
- wasting prevalence;
- stunting;
- micronutrient deficiency;
- infectious disease;
- morbidity;
- mortality;
- fertility;
- health facilities;
- treatment;
- humanitarian feeding;
- actual migration;
- political violence.

Those remain future domain-specific increments.
