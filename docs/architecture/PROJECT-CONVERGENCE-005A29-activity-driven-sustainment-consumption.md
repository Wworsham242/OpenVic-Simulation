# PROJECT-CONVERGENCE-005A29 - Activity-Driven Sustainment Consumption

## Purpose

Convert aggregate military activity over elapsed time into physical sustainment consumption demand.

005A29 connects data-defined activity profiles to the persistent aggregate sustainment stock established in 005A28.

## Native Repository Basis

005A28 provides persistent formation-level consumable holdings with bounded receipt and consumption.

Existing Timespan provides deterministic elapsed simulation time.

005A29 composes those existing mechanisms without introducing a separate stock authority or new simulation clock.

## External Reference Review

Military logistics forecasting commonly derives sustainment requirements from consumption rates, expected usage or activity, and elapsed time.

Consumption rates vary by mission, operational tempo, environment, doctrine, force composition, and equipment rather than being universal constants.

The engine therefore models activity-specific aggregate consumption profiles while leaving their calibrated rates in data.

## Chosen Mechanism

005A29 introduces:

- MilitarySustainmentConsumptionFactor;
- MilitarySustainmentConsumptionProfile;
- MilitarySustainmentConsumptionResultEntry;
- MilitarySustainmentConsumptionResult;
- MilitarySustainmentConsumer.

Each factor binds one opaque sustainment item identity to an aggregate quantity-per-day rate.

Each activity profile contains a small set of those factors.

Requested consumption is derived as:

quantity_per_day * elapsed_days.

Actual consumption remains constrained by physical formation stock.

## Activity Identity

Activity identities are opaque to core.

The engine does not define a closed enumeration such as idle, movement, training, combat, sortie generation, patrol, or siege.

Those meanings belong to scenario data, doctrine, operational mechanics, or extension packages.

## Shortage Semantics

Requested quantity and consumed quantity are recorded separately.

Unmet quantity is the difference between requested and physically consumed quantity.

005A29 does not directly translate unmet quantity into readiness, combat power, movement limits, maintenance effects, or other penalties.

It exposes the physical shortage so later causal mechanics can interpret it.

## Whole-Game Scale Constraint

005A29 is intentionally aggregate.

One activity interval performs work proportional to the small number of sustainment factors in its profile.

It does not iterate individual soldiers, vehicles, weapons, rounds, liters, meals, spare parts, or containers.

The intended cost is approximately formations with active consumption multiplied by relevant sustainment classes.

Consumption should be evaluated on meaningful simulation cadence or activity transitions rather than through microscopic per-object ticks.

## Time Representation

Timespan uses a 64-bit day count, while fixed_point_t intentionally accepts only smaller integral constructor types.

005A29 validates that elapsed day count fits within int32_t before converting it for fixed-point multiplication.

This prevents silent truncation or overflow while retaining deterministic fixed-point arithmetic.

## Calibration Status

005A29 provides mechanism only.

No fuel burn rates, ammunition expenditure rates, ration requirements, maintenance demand, medical consumption, or other empirical quantities are hard-coded.

All consumption coefficients remain data-defined or scenario-calibrated.

## Causal Integration

The sustainment chain can now resolve as:

formation activity
-> activity consumption profile
-> elapsed time
-> requested physical consumption
-> formation stock authority
-> actual consumption
-> explicit unmet demand.

Future increments can convert unmet demand into endurance, capability, readiness, maintenance, or mission effects.

## Reality-Output Principle

A formation operating at higher activity may demand more sustainment because its activity profile says so.

If the required physical stock is unavailable, the engine records the shortage rather than inventing supply or hiding the deficit behind a generic modifier.

This allows operational consequences to emerge from real physical depletion.

## Victoria-Specific Boundary

005A29 does not depend on Victoria military supply categories, countries, armies, POPs, goods, or historical military ontology.

## Modern-Specific Boundary

005A29 does not hard-code fuel, ammunition, food, medicine, batteries, repair parts, aviation fuel, missiles, or other modern sustainment identities.

The same mechanism can represent fodder, arrows, coal, water, reactor consumables, or fictional resources.

## Validation

Focused validation verifies:

- elapsed time scales aggregate requested consumption;
- sufficient physical stock satisfies demand;
- shortage is explicit when demand exceeds stock;
- stock cannot become negative;
- one activity can consume multiple independent resources;
- activity identities remain data-defined.

Military regression remains passing.

## Scope Boundary

005A29 does not select a formation's current activity profile.

It does not yet vary consumption by equipment composition, weather, terrain, distance moved, combat intensity, doctrine, damage, or maintenance condition.

It does not yet convert shortages into capability or readiness effects.

It does not yet automatically generate replenishment shipments.

Those mechanisms should consume the explicit requested, consumed, and unmet quantities produced here.

## Result

005A29 establishes deterministic, aggregate, activity-driven physical sustainment consumption without hard-coded supply classes or microscopic simulation.
