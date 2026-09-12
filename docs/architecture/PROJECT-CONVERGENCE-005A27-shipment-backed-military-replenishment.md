# PROJECT-CONVERGENCE-005A27 - Shipment-Backed Military Replenishment

## Purpose

Use the generic logistics, shipment, transport-execution, and aggregate transport-demand mechanisms in an actual military replenishment consumer.

005A27 proves that military equipment does not become assigned merely because a logistics route exists.

Equipment must physically leave source stock, exist in transit, arrive, and then transfer into formation assignment authority.

## Native Repository Basis

005A20 provides persistent military equipment assignment.

005A21 provides shortfall-only replenishment.

005A22 resolves replenishment through the generic LogisticsGraph.

005A23 provides persistent shipment state.

005A24 provides scarce transport-execution capacity.

005A25 connects shipment dispatch to transport reservation.

005A26 derives aggregate transport demand at whole-game-safe resolution.

005A27 composes those existing mechanisms rather than creating a parallel military logistics system.

## External Reference Review

Real military sustainment distinguishes requirement, source stock, transportation capability, in-transit materiel, arrival, and final distribution to the supported formation.

Shipment visibility and transportation capacity are therefore distinct from final unit possession.

The selected mechanism preserves those authority transitions explicitly.

## Chosen Mechanism

005A27 introduces MilitaryEquipmentShipmentBinding and MilitaryEquipmentShipmentState.

The binding records only:

- generic shipment identity;
- target formation identity;
- equipment item identity;
- completion state.

It does not own stock, transport capacity, routes, or equipment assignment.

## Dispatch Chain

One military replenishment batch resolves as:

persistent formation shortfall
-> generic network delivery request
-> graph-resolved deliverable quantity
-> aggregate transport-demand derivation
-> transport execution reservation
-> authoritative source-stock draw
-> persistent generic shipment
-> military shipment binding.

Formation assignment remains unchanged while the shipment is in transit.

## Arrival Chain

After the generic shipment reaches ARRIVED state:

shipment authority
-> military allocation result
-> persistent assignment authority.

The external depot or national stock provider is not called again during arrival.

This prevents duplicate physical stock draw.

## Authority Boundary

Before dispatch, source stock owns the equipment.

After dispatch and before arrival acceptance, LogisticsShipmentState owns the physical quantity.

After accepted arrival, MilitaryEquipmentAssignmentState owns the assigned quantity.

No stage duplicates authoritative ownership.

## Whole-Game Scale Constraint

005A27 operates on strategically meaningful replenishment batches.

It does not create one shipment object per vehicle, crate, pallet, container, round, or individual equipment item.

A shipment may represent a large aggregate movement constrained by strategic network and transport capacity.

Persistent military binding overhead is therefore proportional to active meaningful replenishment batches, not physical object count.

## Transport Occupation

Shipment arrival and transport-resource release remain independent.

Equipment may reach the formation while transport resources remain occupied by return movement, turnaround, recovery, repositioning, maintenance, or crew-cycle effects.

This preserves the causal impact of long routes on future carrying capacity.

## Calibration Status

005A27 adds no new empirical coefficients.

Route capacity, transport-demand profiles, transit time, occupation time, source stock, and formation requirements remain external or previously defined authorities.

## Causal Integration

The military equipment chain is now capable of resolving as:

equipment requirement
-> outstanding shortfall
-> route capacity
-> transport demand
-> execution reservation
-> stock draw
-> in-transit shipment
-> arrival
-> formation assignment
-> equipment condition/readiness effects.

This converts logistics from an instantaneous availability modifier into a physical causal process.

## Reality-Output Principle

A formation cannot receive equipment simply because national stock exists.

The equipment must be physically deliverable and supported by transport capacity.

Likewise, equipment already removed from source stock cannot be drawn a second time when it arrives.

Poor military sustainment can therefore emerge from route bottlenecks, damaged infrastructure, insufficient lift, long transport cycles, or depleted source stock.

## Victoria-Specific Boundary

005A27 does not depend on Victoria armies, countries, goods, POPs, RGOs, or historical supply abstractions.

Military equipment item identities remain opaque.

## Modern-Specific Boundary

005A27 does not assume trucks, aircraft, ships, container freight, modern depots, or any specific military era.

The same chain can represent arrows, muskets, artillery, armored vehicles, aircraft parts, missiles, medical equipment, or fictional materiel.

## Validation

Focused validation verifies:

- graph capacity limits military replenishment quantity;
- source stock is drawn at dispatch;
- equipment remains unassigned while in transit;
- transport execution capacity is occupied during movement;
- arrived equipment becomes persistent assignment;
- arrival does not perform a second source-stock draw;
- transport occupation can continue after equipment arrival.

Military and generic logistics regressions remain passing.

## Scope Boundary

005A27 handles one strategically meaningful formation/item replenishment batch per dispatch call.

It does not yet batch many formation requests into one convoy, train, ship, or airlift movement.

It does not yet prioritize multiple competing military replenishment shipments through shared execution capacity.

It does not yet model consumable sustainment such as fuel, ammunition expenditure, food, water, medical supplies, or repair parts.

Those should reuse this same generic shipment and transport machinery rather than creating parallel logistics systems.

## Result

005A27 proves that the generic logistics architecture functions in a real military consumer at strategic-game resolution.
