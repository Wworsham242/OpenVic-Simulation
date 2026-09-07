# PROJECT-CONVERGENCE-002C â€” Resource logistics access

Status: resource/logistics cross-domain coupling proof.

## Purpose

Use the source node identities introduced in 002B to make delivered resource
supply depend on logistics-domain access and bottleneck capacity.

The resource source may remain physically intact and productive while logistics
prevents some or all of its output from reaching the consuming process.

## Domain ownership

Resource domain owns:

```text
source identity
node/location identity
nominal physical supply
source availability
```

Logistics owns:

```text
route/corridor
route openness
route availability
bottleneck capacity
```

The typed coupling is `ResourceSourceAccess`.

The resource network consumes the logistics result and computes deliverable
supply. It does not infer why the route is constrained.

## Causal path

```text
physical source
        |
        v
source-specific logistics route
        |
        +-- bottleneck capacity
        +-- accessibility
        +-- route open/closed
        |
        v
deliverable physical supply
        |
        +-- buffer draw
        +-- unmet demand
        |
        v
producer inventory
        |
        v
industrial output
```

No generic "-production" logistics modifier is applied.

## Proof

Two mines each physically produce 2 units/tick.

Each mine has its own coarse `TransportCorridor` into the consuming node.

Mine A remains fully available, but its route is denied.

The result:

```text
physical source availability = unchanged
route A access = denied
route B access = open
deliverable flow = 2
```

A 2-unit buffer preserves full 4-unit industrial input for one tick.

On the following tick the buffer is empty, unmet feedstock becomes 2, and
industrial output falls to 2.

The production loss therefore emerges from:

```text
transport access loss
â†’ deliverable supply loss
â†’ stockpile drawdown
â†’ buffer exhaustion
â†’ unmet physical input
â†’ lower production
```

## Reuse of existing logistics machinery

002C reuses the existing `TransportCorridor` abstraction and its bottleneck
capacity logic.

A corridor remains a coarse strategic mechanism suitable for:

- rail;
- pipeline;
- port;
- shipping chokepoint;
- border crossing;
- road corridor;
- transmission-like capacity seams where appropriate.

002C does not create a second logistics runtime.

## Scope boundary

This slice does not yet solve:

- route search;
- multiple alternative paths per source;
- shared-capacity contention;
- transit time;
- in-transit inventory;
- transport cost;
- mode switching;
- infrastructure damage repair;
- congestion;
- convoy/escort;
- maritime risk;
- grid load flow.

Those should remain logistics-domain mechanisms.

## Next architectural pressure

The next meaningful extension is shared capacity and alternative routing.

A likely next proof is:

```text
source A and source B
â†’ share one constrained corridor segment
â†’ competing flows
â†’ bottleneck allocation
â†’ one consumer/source cannot receive full requested volume
â†’ buffers and inventories determine delayed downstream impact
```

That begins moving from independent corridors toward actual logistics-network
competition without abandoning coarse-grained simulation.