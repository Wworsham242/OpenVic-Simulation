# PROJECT-CONVERGENCE-005A24 - Transport Execution Capacity

## Purpose

Introduce generic scarce transport-execution resources whose capacity can be reserved for the duration of physical movement.

005A24 distinguishes infrastructure throughput from the transportation assets, personnel, and handling capacity required to execute movement.

## Native Repository Basis

The repository already provides TransportLeg and LogisticsGraph for coarse infrastructure routing and throughput.

TransportLeg is explicitly not a vehicle-level object. It may represent rail corridors, ports, pipelines, chokepoints, border crossings, and similar aggregate infrastructure seams.

005A24 therefore adds a parallel but distinct execution-capacity layer rather than overloading graph-edge capacity.

005A23 already provides persistent shipment state and native Date / Timespan handling.

## External Reference Review

Real logistics capacity depends on more than infrastructure.

U.S. Army sustainment doctrine distinguishes transportation infrastructure from the transportation assets, material-handling equipment, personnel, vehicles, and movement-control capabilities required to execute distribution.

Longer routes also keep transport assets and crews committed for longer periods, reducing effective throughput elsewhere even when nominal cargo capacity is unchanged.

The selected simulation model therefore treats execution resources as scarce capacity that can be occupied over time.

## Chosen Mechanism

005A24 introduces:

- TransportExecutionResource
- TransportExecutionRequirement
- TransportExecutionReservation
- TransportExecutionState

Each resource is identified by an opaque resource_id and has nominal capacity, availability fraction, and enabled state.

The core does not assign semantic meaning to resource identities.

Scenario or higher-level systems may define resources such as truck lift, drivers, aircraft lift, sealift, handling crews, cranes, or other execution capabilities.

## Reservation Semantics

A movement may require multiple execution resources simultaneously.

Reservation is atomic.

If any required resource is unknown, invalid, duplicated, disabled, unavailable, or insufficient, no reservation is created.

Successful reservation reduces available execution capacity until its release date.

## Time Occupancy

Execution capacity is occupied for an explicit Timespan.

Therefore two otherwise identical movements can have different strategic effects if one occupies transport resources longer.

Example:

near route -> same lift released quickly

distant route -> same lift unavailable for longer

This allows distance to reduce effective theater throughput without inventing an arbitrary logistics penalty.

## Capacity Boundary

005A24 separates three distinct concepts:

1. infrastructure capacity - represented by TransportLeg / LogisticsGraph;
2. cargo or shipment state - represented by LogisticsShipmentState;
3. execution-resource capacity - represented by TransportExecutionState.

These are causally connected but remain separate authorities.

## Availability

Execution resources expose an availability fraction separate from active reservations.

Availability may later be driven by maintenance, personnel readiness, damage, fuel, weather, fatigue, command decisions, or other world state.

Reservations represent committed use, not intrinsic readiness.

## Fixed-Point Arithmetic

005A24 validation exposed an important numeric constraint in the native Q48.16 fixed-point type.

Binary fractions such as 1/2 and 3/4 are exactly representable, while decimal-like fractions such as 4/5 are not.

Tests therefore use exactly representable binary fractions where exact equality is required.

The transport mechanism itself was not changed to compensate for representational truncation.

## Calibration Status

005A24 introduces no empirical fleet sizes, lift rates, crew ratios, maintenance rates, loading rates, or handling coefficients.

All such quantities remain scenario-defined, empirically calibrated, or derived by future mechanics.

## Causal Integration

The logistics chain can now resolve as:

physical demand
-> infrastructure route
-> infrastructure throughput
-> execution-resource requirements
-> resource reservation
-> shipment dispatch
-> persistent transit
-> resource release
-> arrival
-> destination acceptance

This supports later causal effects from fleet shortages, maintenance losses, driver shortages, port handling limitations, airlift scarcity, sealift scarcity, or similar constraints.

## Reality-Output Principle

Poor logistics performance should emerge from explicit load versus capacity.

A state with too few trucks, insufficient drivers, damaged handling equipment, unavailable aircraft, or overcommitted sealift should perform badly because the required execution resources are actually unavailable.

The engine should not apply an unexplained global logistics-efficiency penalty.

## Victoria-Specific Boundary

005A24 does not depend on Victoria goods, POPs, countries, RGOs, markets, armies, or naval-unit semantics.

Execution resources are opaque and data-defined.

## Modern-Specific Boundary

005A24 does not require trucks, aircraft, ships, container cranes, diesel fuel, or modern military organizations.

The same mechanism can represent animal teams, carts, sailing capacity, rail rolling stock, aircraft lift, shipping, or hypothetical transport systems.

## Validation

Focused tests verify:

- scarce capacity remains reserved until release date;
- longer occupation keeps transport capacity unavailable longer;
- multiple movements share finite capacity;
- multi-resource reservation fails atomically;
- availability modifies effective capacity independently of reservations;
- malformed or unknown resource requirements cannot create reservations.

005A23, generic logistics, and complete military regression suites remain passing.

## Scope Boundary

005A24 does not yet calculate execution-resource requirements from shipment mass, volume, mode, distance, terrain, route, or cargo type.

It also does not yet implement loading queues, convoy composition, vehicle-level movement, maintenance state, fuel consumption, crew fatigue, handling-node queues, border delay, or interdiction.

Those later mechanisms should drive resource demand, availability, occupation time, and shipment progression without replacing the generic reservation model.

## Result

005A24 establishes the rule that transportation capacity is a scarce, time-occupied physical resource rather than a static percentage modifier.
