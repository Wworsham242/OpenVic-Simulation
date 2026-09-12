# PROJECT-CONVERGENCE-005A30 - Sustainment Availability Signal

## Purpose

Derive bounded sustainment fulfillment signals from physically requested and physically consumed formation supplies.

005A30 provides an operationally meaningful shortage signal without directly mutating readiness, combat power, movement, or equipment condition.

## Native Repository Basis

005A28 provides persistent formation-level sustainment stock.

005A29 converts formation activity and elapsed time into requested physical consumption and records actual consumption plus unmet demand.

Existing military runtime architecture already keeps sustainment, equipment condition, and readiness as separate causal concepts.

005A30 therefore derives a separate availability result rather than overwriting readiness.

## External Reference Review

Military sustainment doctrine treats supply as enabling operational reach, endurance, and freedom of action.

Different shortages can constrain different operational functions.

A fuel shortage, ammunition shortage, medical shortage, and repair-parts shortage therefore should not automatically map to one universal combat penalty.

The engine preserves the physical fulfillment facts and leaves their downstream interpretation to domain-specific mechanics.

## Chosen Mechanism

005A30 introduces:

- MilitarySustainmentAvailabilityEntry;
- MilitarySustainmentAvailabilityResult;
- MilitarySustainmentAvailabilityDeriver.

For each sustainment item with positive physical demand:

fulfillment_fraction = consumed_quantity / requested_quantity.

The fraction is bounded to [0,1].

Zero-demand resources are fully satisfied by definition and do not constrain the aggregate signal.

## Per-Resource Signal

Each resource retains its own fulfillment fraction.

This preserves causal information needed by later mechanics.

For example, a future movement system may care strongly about one resource while a weapons-employment system may depend on another.

005A30 does not collapse those distinctions into a single authoritative effect.

## Limiting Fraction

The result also exposes a conservative limiting_fraction equal to the minimum fulfillment fraction among resources with positive demand.

This is a diagnostic and planning signal only.

A limiting_fraction of 0.5 does not mean that a formation has 50 percent combat power, movement, readiness, or effectiveness.

It means only that at least one positively demanded sustainment resource was physically fulfilled at 50 percent during the evaluated activity interval.

## Whole-Game Scale Constraint

005A30 performs one bounded division per relevant sustainment item.

It does not add new persistent inventories, per-object simulation, vehicle loops, soldier loops, or microscopic logistics entities.

The computational cost remains proportional to the small number of sustainment resources already involved in the activity result.

## Calibration Status

005A30 introduces no empirical penalty curves, readiness coefficients, combat modifiers, movement penalties, mission-abort thresholds, or recovery rates.

It derives physical fulfillment only.

Domain-specific interpretation remains future data and mechanism work.

## Causal Integration

The sustainment chain now resolves as:

formation activity
-> physical consumption demand
-> formation sustainment stock
-> actual physical consumption
-> explicit unmet quantity
-> per-resource fulfillment
-> limiting sustainment availability signal.

Future systems may consume either individual fulfillment fractions or the conservative limiting signal depending on their causal requirements.

## Reality-Output Principle

The engine reports what actually happened physically.

It does not infer that every type of shortage has the same operational consequence.

This allows different formations, technologies, activities, doctrines, and eras to react differently to identical numerical shortages when appropriate.

## Victoria-Specific Boundary

005A30 does not depend on Victoria military supply abstractions, armies, countries, POPs, goods, or historical modifier systems.

## Modern-Specific Boundary

005A30 does not assume fuel, ammunition, medical supplies, spare parts, batteries, food, missiles, aviation fuel, or any other modern supply ontology.

Resource identities remain opaque.

## Validation

Focused validation verifies:

- fully supplied demand produces fulfillment 1;
- partial supply produces proportional fulfillment;
- multiple resources preserve independent fulfillment values;
- limiting fraction exposes the weakest positively demanded resource;
- zero-demand resources do not lower availability;
- no readiness or other downstream military state is mutated.

Military regression remains passing.

## Scope Boundary

005A30 does not persist historical availability across activity intervals.

It does not yet calculate endurance over future time.

It does not map shortages to movement, maintenance, readiness, weapons employment, combat power, sortie generation, or mission continuation.

It does not assign different importance weights to sustainment items.

Those mechanisms should consume the explicit per-resource fulfillment values produced here rather than replacing them.

## Result

005A30 establishes a generic, bounded, causal sustainment availability signal while preserving downstream domain autonomy.
