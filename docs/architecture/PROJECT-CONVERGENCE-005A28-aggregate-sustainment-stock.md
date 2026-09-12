# PROJECT-CONVERGENCE-005A28 - Aggregate Sustainment Stock

## Purpose

Provide persistent aggregate consumable holdings for military formations without introducing microscopic supply-object simulation.

005A28 establishes generic formation-level stock authority for data-defined sustainment items.

## Native Repository Basis

Existing military formation instances provide the persistent runtime identity to which sustainment holdings attach.

005A27 proves that physical materiel can move through generic logistics before becoming formation-owned.

005A28 adds the corresponding persistent authority for consumable holdings at formation level.

## External Reference Review

Military sustainment distinguishes multiple supply classes and consumable categories such as fuel, ammunition, food, medical materiel, and repair parts.

Their actual consumption rates vary with mission, activity, environment, doctrine, and equipment.

The selected mechanism therefore keeps item identity data-defined and defers activity-driven consumption rates to a later increment.

## Chosen Mechanism

005A28 introduces MilitarySustainmentStock and MilitarySustainmentStockState.

Each aggregate holding records:

- formation identity;
- opaque sustainment item identity;
- target quantity;
- physical storage capacity;
- current physical quantity.

The state provides bounded receipt, bounded consumption, measurable target shortfall, and remaining storage capacity.

## Authority Boundary

MilitarySustainmentStockState is authoritative only for consumable quantity physically held by the formation.

It does not own external depot stock, transport capacity, shipment state, production, procurement, or economic inventory.

Receipt increases formation-held quantity only up to physical storage capacity.

Consumption reduces only quantity physically on hand and cannot create negative stock.

## Whole-Game Scale Constraint

005A28 is aggregate by design.

The engine does not create one persistent object per round, liter, meal, fuel can, medical item, spare part, package, pallet, or container.

One sustainment stock record represents the strategically meaningful aggregate quantity of one item class for one formation.

Persistent state therefore scales approximately with formations multiplied by the small number of sustainment classes actually relevant to those formations.

## Target Versus Capacity

Target quantity and storage capacity are deliberately distinct.

Target quantity represents desired operating stock.

Storage capacity represents the hard physical upper bound.

A formation may therefore hold more than its target when physical capacity allows, while resupply planning can still use target shortfall as its normal demand signal.

## Shortage Semantics

Consumption requests may exceed physical stock.

In that case, the state consumes only what is actually available and reports the amount consumed.

This preserves physical conservation and gives later activity mechanics an explicit shortage signal.

## Calibration Status

005A28 introduces no real-world stock quantities, days-of-supply values, fuel capacities, ammunition basic loads, ration scales, or spare-parts factors.

Targets, capacities, starting stocks, and item identities remain scenario-defined or empirically calibrated.

## Causal Integration

The future sustainment chain can now resolve as:

activity demand
-> requested consumption
-> physical formation stock
-> actual consumption
-> shortage
-> capability/readiness/endurance effect
-> replenishment demand
-> logistics shipment
-> stock receipt.

005A28 implements the persistent physical stock authority required by that chain.

## Reality-Output Principle

A formation cannot consume supplies it does not physically possess.

Likewise, receiving supplies cannot exceed the formation's storage capacity.

Shortage effects should later emerge from unmet physical consumption rather than arbitrary supply penalties.

## Victoria-Specific Boundary

005A28 does not depend on Victoria military supply goods, armies, countries, POPs, RGOs, or historical logistics assumptions.

## Modern-Specific Boundary

005A28 does not hard-code fuel, ammunition, food, water, medicine, batteries, repair parts, or any modern supply ontology.

Item identifiers remain opaque and may represent any era or fictional sustainment resource.

## Validation

Focused validation verifies:

- persistent aggregate quantity;
- target shortfall calculation;
- remaining physical storage capacity;
- receipt capped by capacity;
- consumption capped by stock on hand;
- no negative quantity;
- duplicate formation/item holdings are rejected;
- arbitrary data-defined sustainment identities are accepted.

Military regression remains passing.

## Scope Boundary

005A28 does not calculate consumption rates.

It does not yet distinguish idle, moving, training, combat, maintenance, or other activity states.

It does not yet convert shortages into readiness or capability effects.

It does not yet automatically request or route replenishment.

Those mechanisms should build on this stock authority rather than bypass it.

## Result

005A28 establishes persistent, conserved, whole-game-safe consumable sustainment stock for military formations.
