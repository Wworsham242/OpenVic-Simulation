# PROJECT-CONVERGENCE-005A22 — Network-Resolved Equipment Delivery

## Purpose

005A22 connects military equipment replenishment to the existing generic strategic logistics network.

Equipment can no longer be assumed to move from available stock to a formation merely because the stock exists.

Physical delivery is now constrained by network topology and transport capacity.

## Native Repository Basis

Before 005A22, the repository already contained:

- TransportLeg;
- TransportCorridor;
- LogisticsGraph;
- DeliverableSupply;
- MarketNodeAccess;
- shared transport-capacity mechanisms;
- deterministic alternate-route resolution.

These are generic logistics mechanisms and remain authoritative.

005A22 does not create a separate military logistics network.

It creates an adapter between military replenishment demand and the existing generic LogisticsGraph.

## External Reference Review

Real logistics practice treats distribution as a network and capacity problem rather than a single efficiency scalar.

Relevant real-world constraints include:

- ports;
- roads;
- rail;
- transportation assets;
- handling capacity;
- infrastructure availability;
- route access;
- personnel;
- throughput;
- distance;
- alternate routes.

Defense logistics also distinguishes physical distribution capacity from inventory availability.

Therefore:

existing stock != deliverable stock.

The mechanism selected for 005A22 is network-constrained flow through existing repository logistics primitives.

## Chosen Mechanism

A military equipment delivery request contains:

- formation identity;
- equipment item identity;
- source logistics node;
- destination logistics node;
- requested physical quantity.

The source and destination meanings remain external and data-defined.

The delivery resolver submits those demands to LogisticsGraph.

LogisticsGraph determines:

- whether a path exists;
- path bottleneck capacity;
- shared-edge contention;
- residual alternate-route capacity.

The resulting physically deliverable quantity then caps the equipment quantity offered to the existing allocation and stock-draw authority.

## Generic Logistics Conservation Correction

005A22 exposed an existing fixed-point conservation defect in LogisticsGraph.

The prior shared-capacity allocation calculated:

request * (capacity / total_request)

Because fixed-point division truncates, a capacity ratio such as 4 / 10 could become slightly less than 0.4 before being multiplied by the request.

The graph now calculates:

request * capacity / total_request

This preserves physical quantities more accurately and ensures that a corridor with capacity 4 can deliver 4 when demand permits it.

This correction is generic and benefits all future consumers of LogisticsGraph.

## Authority Boundaries

Authoritative responsibilities remain separated.

External stock authority owns:

- unassigned physical stock.

MilitaryEquipmentAssignmentState owns:

- persistent equipment assigned to formations.

LogisticsGraph owns:

- transport topology;
- open/closed edges;
- effective transport capacity;
- shared physical contention;
- routing.

MilitaryEquipmentDeliveryResult is derived.

It is not:

- persistent inventory;
- a second logistics ledger;
- a second stock authority.

## Causal Sequence

The replenishment chain is now:

declared requirement

→ persistent assignment

→ actual shortfall

→ logistics source/destination binding

→ network route resolution

→ shared transport capacity

→ physically deliverable quantity

→ stock draw

→ allocation

→ persistent formation assignment

→ equipment condition

→ readiness.

## Port-Loss Example

A forward port may be represented by a transport edge or network connection.

If that connection becomes unavailable:

main route

→ closed

→ route solver attempts another feasible path

→ alternate path may have lower capacity

→ less equipment is physically deliverable

→ formation replenishment falls.

The engine does not apply an arbitrary generic "port destroyed = logistics -X%" modifier.

The outcome follows from changed network state.

## Shared Capacity

Multiple formations using the same corridor compete for the same physical capacity.

The existing LogisticsGraph proportionally constrains flows across shared edges.

Therefore a strategic chokepoint may constrain multiple military consumers simultaneously.

## Durable Equipment Versus Consumable Flow

005A22 is currently integrated with durable equipment replenishment.

Durable equipment includes examples such as:

- vehicles;
- aircraft;
- launchers;
- sensors;
- major weapon systems.

Persistent formation assignment remains appropriate for those assets.

Consumables should not automatically use the same persistence semantics.

Examples include:

- fuel;
- ammunition;
- food;
- water;
- medical supplies;
- many spare parts.

Those should later use:

network delivery

→ local inventory

→ consumption

→ endurance / operational effect.

Both durable assets and consumables should use the same underlying physical logistics network.

## Calibration Status

005A22 introduces no empirical coefficient.

Network capacities, availability, topology and source/destination bindings remain:

- scenario-defined;
- derived from authoritative infrastructure state;
- or later calibrated from empirical data.

The test quantities are proof values only.

## Causal Integration

Authoritative inputs:

- persistent equipment shortfall;
- external source/destination binding;
- LogisticsGraph topology;
- edge availability;
- edge capacity;
- external stock.

Mechanism:

- deterministic graph routing;
- shared-capacity allocation;
- alternate-path resolution;
- stock allocation.

Derived output:

- requested quantity;
- deliverable quantity;
- unmet quantity;
- selected route;
- alternate route.

Downstream consumers:

- external stock draw;
- persistent equipment assignment;
- equipment condition;
- readiness.

Timing:

005A22 is still an instantaneous capacity-resolution pass.

Transit time is explicitly deferred.

## Scope Boundary

005A22 does not yet implement:

- shipment persistence;
- transit time;
- convoy travel;
- transport-cycle occupancy;
- depots as stored intermediate inventories;
- fuel consumption by transport;
- perishable goods;
- cold-chain requirements;
- ammunition consumption;
- food consumption;
- local supply inventories;
- maintenance;
- repair;
- interdiction losses in transit;
- personnel requirements for logistics;
- command/staff throughput;
- territorial permission mechanics.

Those mechanisms should feed or extend the same logistics network rather than creating parallel logistics systems.

## Validation

005A22 focused tests verify:

- network bottlenecks cap equipment replenishment before stock draw;
- closed routes can block physical delivery;
- alternate routes restore constrained delivery;
- shared corridor capacity constrains competing formations;
- unconstrained networks permit full shortfall delivery.

The generic LogisticsGraph regression verifies the corrected conserved-quantity calculation.

Predecessor military allocation and assignment regressions remain passing.

The complete military regression suite remains passing.

Full CTest is required before certification.

## Result

005A22 establishes:

> Existing stock is not equivalent to deliverable stock.

Military replenishment must now pass through actual strategic transport topology and physical network capacity before equipment can become assigned to a formation.

This preserves the broader engine rule:

> The engine executes mechanics correctly; bad logistics emerge from bad world state.
