# PROJECT-CONVERGENCE-005A15 — Sustainment to Readiness Coupling Proof

## Purpose

005A15 connects authoritative sustainment to authoritative readiness without making sustainment the master military-state variable.

Sustainment constrains the attainable readiness target.

Readiness changes through bounded transitions rather than being overwritten immediately.

## Native Repository Basis

005A14 introduced authoritative sustainment derived from optional military support dependencies.

`MilitaryFormationInstance` already carried a separate readiness state.

005A15 preserves that separation and adds a narrow runtime transition mechanism.

No new military subsystem or parallel state ledger is introduced.

## External Reference Review

Real military readiness does not instantaneously equal logistics or sustainment condition.

A force may remain temporarily ready after support degradation because it still possesses:

- fuel;
- ammunition;
- maintained equipment;
- rested personnel;
- local stocks;
- residual operational capacity.

Likewise, good sustainment does not by itself guarantee high readiness.

Training, personnel, fatigue, equipment condition, command effectiveness, morale, and operational demands may independently constrain readiness.

The engine therefore requires delayed, multi-causal readiness state rather than direct assignment from sustainment.

## Chosen Mechanism

005A15 introduces:

`adjust_readiness_toward(...)`

Inputs:

- formation identity;
- desired readiness;
- maximum adjustment per update.

The runtime calculates:

`effective target = min(desired readiness, sustainment)`.

Readiness then moves toward that target by no more than `max_adjustment`.

## Sustainment as Constraint

Sustainment acts as a ceiling on the readiness requested by other causes.

Example:

desired readiness = 1.0

sustainment = 0.5

effective target = 0.5.

This does not mean readiness is instantly assigned 0.5.

## Bounded State Transition

If readiness is above the effective target, it declines toward the target.

If readiness is below the effective target, it recovers toward the target.

Each update is limited by `max_adjustment`.

Example:

readiness = 0.9

sustainment = 0.5

max adjustment = 0.2

first update → 0.7

second update → 0.5.

## Multi-Causal Readiness

The desired readiness input is intentionally external to this function.

Future mechanisms may derive it from:

- personnel availability;
- equipment condition;
- training;
- fatigue;
- morale;
- command effectiveness;
- operational tempo;
- combat effects;
- other military state.

Therefore sustainment does not own readiness.

## Recovery

When sustainment is healthy and the desired readiness is high, low readiness can recover incrementally.

This permits later recovery mechanics to operate through cadence rather than instantaneous resets.

## Degradation

A sudden support failure may reduce sustainment immediately while readiness remains temporarily above the new sustainment ceiling.

Subsequent readiness updates cause degradation over time.

This preserves history-bearing state.

## Validation

Desired readiness must be within [0,1].

Maximum adjustment must be within (0,1].

Invalid requests are rejected without changing authoritative readiness.

## Victoria-Specific Boundary

005A15 does not depend on:

- armies;
- navies;
- regiments;
- ships;
- Victoria organization mechanics;
- Victoria supply modifiers.

## Modern-Specific Boundary

005A15 does not hard-code:

- NATO readiness categories;
- sortie rates;
- modern maintenance doctrine;
- fuel-day assumptions;
- modern force-generation models.

Readiness and sustainment remain generic bounded state variables.

## Calibration Status

No empirical readiness-loss or recovery coefficients are introduced.

The transition values used in tests are deterministic proof fixtures.

Final adjustment rates must later be calibrated by formation type, doctrine, support structure, operational tempo, and empirical evidence where available.

## Causal Integration

005A15 extends the causal chain:

support target availability

→ sustainment

→ readiness ceiling

→ bounded readiness transition.

This produces a history-bearing military state rather than a direct support-to-combat modifier.

## Scope Boundary

005A15 does not implement:

- personnel readiness;
- training state;
- equipment condition;
- fatigue;
- morale;
- ammunition state;
- fuel state;
- combat power;
- sortie generation;
- maintenance backlog;
- operational tempo;
- cadence ownership.

Those remain later increments.

## Validation Performed

Focused tests verify:

- healthy sustainment permits bounded readiness recovery;
- sustainment constrains readiness ceiling;
- sustainment loss degrades readiness gradually;
- repeated updates converge to the sustainment ceiling;
- a lower non-sustainment readiness target remains authoritative;
- invalid inputs do not mutate readiness.

005A14 and 005A13 regressions remain passing.

The broader military test suite remains passing.

Full CTest is required before closure.

## Result

005A15 establishes:

> Sustainment constrains readiness, but does not define readiness.

The resulting causal structure is:

support infrastructure

→ sustainment

→ readiness constraint

plus

other readiness causes

→ desired readiness

then

desired readiness + sustainment constraint

→ bounded authoritative readiness transition.
