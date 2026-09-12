# PROJECT-CONVERGENCE-005A17 — Equipment Requirement Authority Bridge

## Purpose

005A17 makes formation equipment condition derive from externally authoritative equipment quantities rather than from direct manual assignment.

The military runtime declares what a formation requires.

Another authoritative domain owns the actual stock or assigned quantity.

The military runtime derives equipment condition from that external state.

## Native Repository Basis

Repository reconnaissance found existing physical-stock mechanisms outside the military runtime.

`AggregateProducer` already owns goods-keyed inventory and physically consumes production inputs.

`ResourceSupplyNetwork` already provides bounded buffer storage, draw, replenishment, accessibility, and unmet-demand behavior.

Therefore 005A17 does not introduce a military-specific inventory ledger.

`MilitaryFormationInstance` continues to own only military state:

- readiness;
- sustainment;
- equipment condition;
- placement;
- hosting;
- support relationships.

## External Reference Review

Military formations depend on equipment availability, but the equipment itself belongs to broader procurement, production, storage, transport, maintenance, and allocation systems.

A force structure should therefore declare equipment requirements without becoming the authoritative owner of every factory stockpile, depot inventory, logistics buffer, or national equipment reserve.

The appropriate separation is:

external stock authority

→ assigned or available quantity

→ formation requirement fulfillment

→ equipment condition

→ readiness.

## Chosen Mechanism

`MilitaryFormationDefinition` gains optional equipment requirements.

Each requirement contains:

- a content-defined item identifier;
- a required quantity.

The item identifier is intentionally semantic-light.

It may later resolve to:

- an economy good;
- a historical equipment category;
- a vehicle type;
- a weapon family;
- another authoritative stock identity.

The core military system does not define those meanings.

## External Quantity Provider

`MilitaryFormationInstanceManager::evaluate_equipment_condition(...)` consumes a read-only quantity provider.

The provider maps:

equipment item identifier

→ authoritative quantity available or assigned to the formation.

The military runtime does not store that quantity.

## Optional Equipment Capability

A formation with no declared equipment requirements remains valid.

For such a formation:

equipment condition = 1.

The external provider is not queried.

This prevents the engine from imposing industrial-era or modern equipment structures on every possible military formation.

## Requirement Fulfillment

For each declared requirement:

availability fraction =
available quantity / required quantity.

The fraction is capped at 1.

Example:

required = 10

available = 5

equipment contribution = 0.5.

## Multiple Equipment Requirements

Different required equipment items are currently treated as complementary bottlenecks.

The lowest fulfillment fraction determines aggregate equipment condition.

Example:

primary equipment fulfillment = 0.8

secondary equipment fulfillment = 0.5

equipment condition = 0.5.

This is a narrow proof rule, not the final equipment-combat model.

## Excess Equipment

Quantities above requirement do not increase equipment condition above 1.

Surplus stock remains meaningful to its authoritative owning domain but does not create readiness above the healthy condition ceiling.

## Readiness Integration

005A16 established equipment condition as an independent readiness constraint.

005A17 now supplies that condition from real external quantities.

The resulting path is:

external equipment quantity

→ requirement fulfillment

→ equipment condition

→ readiness ceiling

→ bounded readiness transition.

## Validation

Equipment requirements must have:

- non-empty identifiers;
- positive required quantities;
- no duplicate item identifiers within one formation definition.

Provider quantities must be nonnegative.

Invalid provider values are rejected without mutating the previous authoritative equipment condition.

## No Duplicate Inventory

005A17 explicitly does not introduce:

- `MilitaryEquipmentInventory`;
- `MilitarySupplyInventory`;
- `MilitaryStockpile`;
- military-owned duplicate economy goods;
- a second authoritative logistics ledger.

Actual equipment stock remains outside this mechanism.

## Victoria-Specific Boundary

005A17 does not depend on:

- regiment templates;
- ship classes;
- legacy land/naval branch enums;
- Victoria military supply rules;
- Victoria reinforcement mechanics.

## Modern-Specific Boundary

005A17 does not hard-code:

- tanks;
- fighters;
- missiles;
- trucks;
- rifles;
- carriers;
- spare-parts categories.

All equipment semantics remain data-defined.

## Calibration Status

No empirical equipment tables or military coefficients are introduced.

Test quantities are deterministic proof fixtures.

The current minimum-fulfillment aggregation is provisional.

Future equipment modeling may require:

- weighted requirements;
- substitutable equipment;
- role-specific equipment;
- mission-specific requirements;
- damaged versus serviceable counts;
- maintenance states;
- replacement flows;
- mobilization reserves.

## Causal Integration

005A17 extends the causal chain:

production / procurement / storage / logistics

→ authoritative equipment quantities

→ formation equipment requirements

→ equipment condition

→ readiness.

This connects the military layer to the physical economy without allowing the military layer to duplicate economy state.

## Scope Boundary

005A17 does not implement:

- procurement;
- equipment production orders;
- national stockpile ownership;
- depot inventories;
- allocation policy;
- transport delivery;
- damaged equipment;
- maintenance queues;
- repair;
- spare parts;
- attrition;
- replacement;
- capture;
- transfer;
- consumption.

Those remain later increments.

## Validation Performed

Focused tests verify:

- equipment requirements remain optional;
- externally supplied quantity derives equipment condition;
- multiple required items produce a bottleneck;
- excess equipment is capped at healthy condition;
- derived condition constrains readiness through the existing 005A16 path;
- invalid provider results do not mutate state;
- invalid and duplicate requirement definitions are rejected.

005A16 and 005A15 regressions remain passing.

The broader military suite remains passing.

Full CTest is required before closure.

## Result

005A17 establishes:

> Military formations declare equipment needs, but authoritative equipment quantities remain owned by the physical economy, logistics, or allocation domain.

The military layer therefore consumes real external state instead of creating a parallel inventory system.
