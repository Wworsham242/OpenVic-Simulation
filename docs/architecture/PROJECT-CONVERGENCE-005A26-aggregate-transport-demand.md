# PROJECT-CONVERGENCE-005A26 - Aggregate Transport Demand

## Purpose

Derive transport-execution requirements for strategically meaningful movement batches without introducing microscopic vehicle, crew, pallet, or container simulation.

005A26 establishes shared data-defined transport demand profiles that transform physical cargo quantity and route occupation time into aggregate execution-resource requirements.

## Native Repository Basis

005A24 provides generic scarce transport-execution resource pools.

005A25 connects those execution resources to persistent shipment dispatch.

005A26 provides the missing derivation seam between physical movement demand and aggregate execution-resource requirements.

## External Reference Review

Real transport planning combines fixed movement overhead with workload that scales with physical demand.

Examples include movement-control effort, staging, loading and handling, carrying capacity, crew requirements, turnaround, and route occupation.

The engine models these at aggregate strategic resolution rather than enumerating individual vehicles or personnel.

## Chosen Mechanism

005A26 introduces:

- TransportDemandFactor;
- TransportDemandProfile;
- TransportDemandResult;
- TransportDemandDeriver.

A demand factor may contain:

- fixed_capacity: paid once per movement batch;
- capacity_per_quantity: scales with physical cargo quantity.

A profile contains a small set of those factors plus a minimum aggregate occupation time.

Profile and resource identifiers remain opaque to core.

## Whole-Game Scale Constraint

005A26 is explicitly an aggregate strategic mechanism.

The engine must not require one persistent simulation object per truck, driver, railcar, aircraft, ship, pallet, container, crate, animal, or physical ton.

Transport demand is evaluated per movement batch against shared resource pools.

One movement batch may represent a convoy, rail movement, sealift allocation, airlift package, caravan, pipeline operating allocation, or other scenario-defined aggregate movement.

The computational cost of derivation is proportional to the small number of demand factors in a shared profile, not to the real-world object count represented by the profile.

## Resolution Hierarchy

The intended whole-game resolution hierarchy is:

ordinary economic movement -> aggregated flow;

constrained corridor -> route and throughput resolution;

strategically consequential transit -> persistent shipment batch;

scarce carrying or handling capability -> execution reservation;

individual carrier -> generally not simulated.

Finer resolution should only be introduced where it changes meaningful strategic outcomes.

## Shared Definitions

TransportDemandProfile is definition-like data and should be reused across many movements.

Derivation produces only the small transient requirement vector needed by the execution reservation mechanism.

005A26 therefore does not introduce a new persistent world-state ledger.

## Fixed and Scaled Demand

Not every transport requirement scales proportionally with cargo.

A movement may have a fixed requirement such as movement-control or staging capacity while carrying capacity scales with quantity.

This avoids the incorrect assumption that every physical resource requirement is a simple linear fraction of cargo quantity.

## Occupation Time

Route-derived occupation time remains external.

A profile may impose a minimum occupation time representing aggregate turnaround, loading, recovery, staging, crew-cycle, or similar commitment.

The final occupation time is the greater of route-derived occupation time and the profile minimum.

## Calibration Status

005A26 introduces mechanism but no real-world transport coefficients.

Profiles, fixed requirements, scaled requirements, and occupation minima remain scenario-defined or empirically calibrated.

The engine does not assume modern road, rail, air, or maritime values.

## Causal Integration

The logistics chain may now resolve as:

physical movement demand
-> shared transport demand profile
-> aggregate execution requirements
-> transport-resource feasibility
-> reservation
-> source draw
-> persistent shipment
-> transit
-> resource release
-> destination acceptance

This allows different transport systems to create different resource pressures without changing core code.

## Reality-Output Principle

Scarcity should emerge from physical demand competing for finite aggregate capability.

The engine should not simulate microscopic detail merely because the real world contains that detail.

Resolution is increased only when the additional state changes strategic causality or player decisions.

## Victoria-Specific Boundary

005A26 does not depend on Victoria goods, POPs, countries, RGOs, historical transport types, or military-unit classes.

## Modern-Specific Boundary

005A26 contains no closed truck, rail, airlift, sealift, pipeline, or container mode enumeration.

The same mechanism can represent carts, pack animals, sailing transport, rail freight, strategic airlift, autonomous systems, or fictional transport systems through data-defined profiles.

## Performance Boundary

005A26 must remain batch-oriented.

It should not create persistent carrier entities during demand derivation.

It should not require per-tick recomputation for every possible movement in the world.

Demand should be derived only when a movement is planned, changed, or otherwise becomes causally relevant.

Shared profiles should be reused rather than copied into every shipment.

## Validation

Focused validation verifies:

- fixed and quantity-scaled demand compose correctly;
- fixed movement overhead does not scale with cargo quantity;
- minimum occupation time is respected;
- arbitrary data-defined profiles work without closed transport-mode enums.

The generic logistics regression remains passing.

## Scope Boundary

005A26 does not yet select the appropriate profile for a route or cargo.

It does not yet model cargo compatibility, route-mode compatibility, multimodal transfer, queueing, loading-node congestion, maintenance state, fuel consumption, crew fatigue, or transport losses.

It also does not instantiate individual vehicles or crews.

Those mechanisms should only be added where their causal value justifies their simulation cost.

## Result

005A26 establishes a whole-game-safe abstraction in which strategically meaningful movement batches consume derived aggregate transport capacity without microscopic carrier simulation.
