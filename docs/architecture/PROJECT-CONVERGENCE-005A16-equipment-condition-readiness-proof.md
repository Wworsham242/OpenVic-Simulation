# PROJECT-CONVERGENCE-005A16 — Equipment Condition as Second Readiness Cause

## Purpose

005A16 proves that military readiness can be constrained by more than one independent authoritative cause.

005A15 established sustainment as a readiness ceiling.

005A16 adds authoritative aggregate equipment condition as a second independent readiness constraint.

## Native Repository Basis

`MilitaryFormationInstance` already carries:

- readiness;
- sustainment;
- placement;
- hosting;
- support relationships.

005A16 adds one bounded state variable:

`equipment_condition`.

No second readiness system or equipment inventory ledger is introduced.

## External Reference Review

Real military readiness depends on both support and the physical condition of equipment.

A formation can be well supplied but poorly ready because vehicles, aircraft, ships, sensors, weapons, or other equipment are unavailable or degraded.

Likewise, equipment can be in good condition while poor sustainment constrains readiness.

Therefore sustainment and equipment condition must remain distinct causes.

## Chosen Mechanism

`MilitaryFormationInstance` gains authoritative aggregate equipment condition in [0,1].

The default value is 1.0 for compatibility with formations that do not yet have detailed equipment mechanics.

The readiness transition from 005A15 now computes:

`effective target = min(desired readiness, sustainment, equipment condition)`.

## Equipment Condition Is Not an Inventory

Equipment condition is deliberately aggregate.

It does not represent:

- individual vehicles;
- aircraft;
- ships;
- weapon counts;
- spare parts;
- maintenance personnel;
- repair queues;
- stockpiles.

Those remain future causal producers of this state.

## Independent Causal Role

Equipment condition constrains readiness independently of sustainment.

Example:

sustainment = 1.0

equipment condition = 0.5

desired readiness = 1.0

effective readiness target = 0.5.

## Multi-Causal Readiness

The readiness path is now explicitly multi-causal.

Inputs include:

- desired readiness from other causes;
- sustainment;
- equipment condition.

The lowest active constraint determines the effective readiness target.

This demonstrates that no single military variable owns readiness.

## History-Bearing State

Equipment condition itself is authoritative persistent state.

Readiness does not instantly equal equipment condition.

A sudden equipment degradation lowers the target, while readiness declines through the bounded transition mechanism from 005A15.

This preserves temporal history.

## Recovery

Improved equipment condition raises the attainable readiness ceiling.

Actual readiness then recovers over subsequent bounded updates rather than instantaneously.

## Validation

Equipment condition must remain within [0,1].

Invalid values are rejected without mutating the previous authoritative state.

## Victoria-Specific Boundary

005A16 does not depend on:

- regiments;
- ships;
- armies;
- navies;
- unit branch enums;
- Victoria organization mechanics;
- Victoria equipment abstractions.

## Modern-Specific Boundary

005A16 does not hard-code:

- tanks;
- fighter aircraft;
- carriers;
- missile systems;
- modern maintenance doctrine;
- mission-capable-rate categories.

Equipment condition is a generic bounded state applicable to different periods and military forms.

## Calibration Status

No empirical equipment-condition coefficients are introduced.

The values used in tests are proof fixtures.

Future equipment and maintenance models must determine the authoritative condition state using domain-appropriate mechanisms and calibrated data.

## Causal Integration

005A16 extends the military causal chain:

support infrastructure

→ sustainment

equipment state

→ equipment condition

then

sustainment + equipment condition + other readiness causes

→ effective readiness target

→ bounded readiness transition

→ actual readiness.

## Scope Boundary

005A16 does not implement:

- equipment inventories;
- equipment classes;
- unit equipment tables;
- attrition;
- maintenance queues;
- spare-parts consumption;
- repair capacity;
- production replacement;
- equipment transfer;
- combat damage;
- equipment-specific mission availability.

Those remain later increments.

## Validation Performed

Focused tests verify:

- equipment condition defaults healthy;
- equipment condition independently constrains readiness;
- sustainment and equipment operate as independent constraints;
- equipment degradation affects readiness gradually;
- equipment recovery permits readiness recovery;
- invalid equipment values do not mutate authoritative state.

005A15 and 005A14 regressions remain passing.

The broader military suite remains passing.

Full CTest is required before closure.

## Result

005A16 establishes:

> Readiness is now demonstrably multi-causal.

The current structure is:

support network

→ sustainment

equipment state

→ equipment condition

other causes

→ desired readiness

then

min(desired readiness, sustainment, equipment condition)

→ bounded readiness transition

→ authoritative readiness.
