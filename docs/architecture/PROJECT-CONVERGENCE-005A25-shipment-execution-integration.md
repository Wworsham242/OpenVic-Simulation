# PROJECT-CONVERGENCE-005A25 - Shipment Execution Integration

## Purpose

Connect persistent shipment state to scarce transport-execution capacity without introducing duplicate physical authority.

A shipment may dispatch only after its declared execution-resource requirements are successfully reserved.

## Native Repository Basis

005A23 provides persistent logistics shipment state, source draw, transit timing, arrival, and destination acceptance.

005A24 provides scarce transport-execution resources and time-bounded reservations.

005A25 connects those two mechanisms using their existing authority boundaries rather than creating a second shipment or transport ledger.

## External Reference Review

Real movement control requires transport capability to be allocated before movement is executed.

U.S. Army movement-control doctrine treats movement as the allocation, scheduling, and regulation of transportation capability against movement requirements.

The selected engine sequence therefore reserves execution capacity before source cargo is physically removed.

## Chosen Mechanism

005A25 introduces LogisticsDispatchExecutionCoordinator.

The coordinator performs an ordered movement transaction:

1. validate shipment request;
2. reserve required execution resources;
3. draw physical cargo from source authority;
4. create persistent shipment state;
5. bind the transport reservation to the new shipment identity.

If source draw produces zero cargo, the planned transport reservation is cancelled.

## Preflight

LogisticsShipmentState now exposes can_dispatch as a non-mutating shipment validation seam.

TransportExecutionState now exposes can_reserve for non-mutating execution-capacity validation.

These checks allow impossible movement requests to fail before physical source state changes.

## Planned Reservation

TransportExecutionState may create an active planned reservation before a shipment identity exists.

A planned reservation temporarily uses shipment_unique_id zero.

After successful physical dispatch, the reservation is bound to the newly created shipment identity.

If dispatch fails or produces zero cargo, the reservation is cancelled.

## Atomicity Boundary

005A25 prevents the most dangerous partial state transition:

cargo removed from source without transport capacity.

Insufficient execution capacity fails before the source draw provider is called.

Rollback cancellation results are explicitly checked rather than ignored.

## Partial Physical Dispatch

The source authority may provide less physical cargo than requested.

Only the physically drawn quantity becomes shipment state.

The shipment therefore cannot represent cargo that did not actually leave source inventory.

Execution requirements in 005A25 remain explicit movement-plan requirements.

They are not automatically scaled to the partial cargo quantity because different resource types may have fixed, nonlinear, threshold, or cargo-specific requirements.

Derivation of execution requirements from cargo and mode belongs to a later mechanism.

## Shipment and Transport Time

Shipment transit duration and transport-resource occupation duration are deliberately independent.

Cargo may arrive before a transport resource is released.

For example, unloading may complete while a ship, truck fleet, aircraft, crew, escort, or handling system remains committed to return travel, repositioning, turnaround, maintenance, or crew-cycle requirements.

This distinction enables distance to reduce effective throughput through resource occupation rather than arbitrary penalties.

## Calibration Status

005A25 introduces no empirical transport coefficients.

Execution-resource requirements and occupation duration are supplied externally.

Future calibrated mechanisms may derive them from cargo characteristics, transport mode, route, handling requirements, vehicle capacity, crew requirements, maintenance, and operating conditions.

## Causal Integration

The generic logistics chain is now:

physical source stock
-> route feasibility
-> execution-resource feasibility
-> planned transport reservation
-> physical source draw
-> persistent shipment creation
-> reservation binding
-> shipment transit
-> transport-resource release
-> shipment arrival
-> destination acceptance

This is a causal movement transaction rather than a supply modifier.

## Reality-Output Principle

A movement cannot occur merely because cargo exists.

It also requires the physical execution capacity needed to move that cargo.

Likewise, transport capacity cannot create cargo that does not exist.

Both conditions must independently succeed.

## Victoria-Specific Boundary

005A25 does not depend on Victoria markets, countries, POPs, RGOs, historical goods, land armies, or naval-unit semantics.

Cargo and execution-resource identities remain opaque.

## Modern-Specific Boundary

005A25 does not assume trucks, ships, aircraft, container freight, petroleum logistics, or modern military organizations.

Any scenario capable of defining physical cargo, a route, movement duration, and execution-resource requirements can use the same transaction.

## Validation

Focused tests verify:

- insufficient transport capacity prevents physical source draw;
- successful dispatch binds reservation identity to shipment identity;
- zero source stock cancels planned transport capacity;
- invalid shipment requests do not reserve transport;
- transport occupation may outlast shipment transit;
- partial source draw creates only matching partial shipment state;
- partial shipment does not duplicate source stock;
- explicit movement-plan reservations remain authoritative after partial cargo draw.

005A24, 005A23, generic logistics, and complete military regressions remain passing.

## Scope Boundary

005A25 does not yet derive execution-resource requirements from shipment mass, volume, cargo class, route, transport mode, vehicle characteristics, handling requirements, or distance.

It also does not yet connect the military replenishment request directly through this complete shipment-execution transaction.

Those are subsequent integration seams.

## Result

005A25 establishes that physical cargo movement requires both cargo authority and transport execution authority to succeed in the correct order.
