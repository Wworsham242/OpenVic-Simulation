# PROJECT-CONVERGENCE-005A23 - Persistent Logistics Shipment State

## Purpose

Introduce history-bearing generic shipment state so physical material can leave one authoritative inventory, remain in transit for a non-zero period, arrive, and only then transfer into destination authority.

This removes the instantaneous-delivery assumption without introducing military-specific transport semantics.

## Native Repository Basis

005A23 reuses the existing LogisticsGraphPath, market-node identity, fixed-point quantities, Date, Timespan, and external authority-provider seams.

No parallel military clock, route graph, or inventory authority is introduced.

## External Reference Review

Real transportation systems distinguish dispatch, transit, arrival, and receipt.

U.S. Army sustainment practice treats long-distance distribution as a time-distance and capacity problem affected by travel time, loading and unloading, transport availability, maintenance, personnel, route conditions, staging, permissions, communications, and support nodes.

The selected engine model therefore separates material at source, material in transit, arrived material, and material accepted by destination authority.

Transport resources constrain movement but are not properties of the cargo itself.

## Chosen Mechanism

005A23 introduces LogisticsShipmentRequest, LogisticsShipment, LogisticsShipmentStatus, and LogisticsShipmentState.

A shipment records opaque content identity, source node, destination node, physical quantity, resolved path, dispatch date, expected arrival date, and shipment status.

Lifecycle:

SOURCE INVENTORY -> IN TRANSIT -> ARRIVED -> DELIVERED

Dispatch physically removes quantity from source authority before persistent transit state is created.

Arrival occurs only when authoritative simulation time reaches the expected arrival date.

Destination acceptance is separate from arrival. Rejected receipt leaves the shipment in ARRIVED state.

## Authority Boundary

Before dispatch, source inventory owns the quantity.

After dispatch and before destination acceptance, LogisticsShipmentState owns the quantity.

After successful acceptance, destination authority owns the quantity.

The same material therefore cannot simultaneously exist at source, in transit, and at destination.

DELIVERED is historical shipment state, not a second inventory.

## Transit Time

005A23 stores transit duration but does not calculate transport speed.

Future mechanisms may derive duration from distance, transport mode, infrastructure condition, weather, congestion, borders, handling, staging, crews, maintenance, hostile interference, and organizational execution.

This keeps shipment persistence generic while allowing later causal systems to determine actual travel time.

## Transport Resource Boundary

A shipment is not its truck, railcar, aircraft, ship, driver, crew, pipeline slot, crane, or warehouse team.

Those are independent scarce capacities and should constrain shipment dispatch or progression through later transport-execution mechanics.

005A23 therefore does not embed transport-resource ownership into LogisticsShipment.

## Calibration Status

005A23 introduces no empirical transport coefficients.

Transport speed, handling time, delay, failure rate, congestion, and similar quantities remain external and must later be empirically or scenario calibrated.

## Causal Integration

The physical chain is now:

production -> available stock -> logistics demand -> dispatch -> in-transit material -> arrival -> destination acceptance -> usable destination stock

For military equipment this can later continue through persistent formation assignment, equipment condition, capability, and readiness.

For consumables it can instead continue into local inventory, consumption, endurance, and operational effects.

## Reality-Output Principle

The shipment mechanism does not randomly decide that cargo is late.

Later causal mechanisms determine delay from actual conditions such as distance, route state, transportation capacity, maintenance, personnel, permissions, congestion, weather, and disruption.

The engine executes those causes deterministically from authoritative world state.

## Victoria-Specific Boundary

The shipment mechanism does not require countries, POPs, RGOs, factories, Victoria markets, land armies, naval units, or historical Victoria goods.

Content identity remains opaque and data-defined.

## Modern-Specific Boundary

The shipment mechanism does not require trucks, aircraft, railroads, container shipping, petroleum logistics, modern formations, or modern states.

Ancient caravans, sailing vessels, industrial rail, modern sealift, or other content can use the same lifecycle if higher-level data supplies appropriate routes and durations.

## Validation

Focused tests verify persistent dispatch, source draw, partial draw, zero draw, native transit time, early-versus-late arrival, arrival persistence, destination rejection, destination acceptance, terminal delivery, and invalid-request rejection.

005A22, generic logistics, and complete military regression suites remain passing before closure.

## Scope Boundary

005A23 does not yet implement transport fleets, vehicle occupancy, convoy formation, rolling stock, airlift, sealift, pipeline flow, handling capacity, storage capacity, congestion, route reservations over time, interdiction, rerouting, shipment loss, weather effects, border permissions, logistics personnel, or organizational execution capacity.

Those should act on this shipment lifecycle rather than replace it.

## Result

005A23 establishes the rule that material leaving a source does not exist at its destination until the logistics system actually gets it there.
