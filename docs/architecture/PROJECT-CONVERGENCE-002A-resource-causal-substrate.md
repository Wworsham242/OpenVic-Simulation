# PROJECT-CONVERGENCE-002A â€” Resource causal substrate

Status: first cross-domain causal coupling proof.

## Purpose

Introduce the first reusable cross-domain causal substrate after native runtime
ownership was proven in 001D/001E.

The target is not a refinery mechanic. The target is a general resource-domain
boundary that can later represent extraction, depletion, weather, sanctions,
outages, war damage, environmental constraints, energy availability, and other
causes without embedding those causes inside the economy.

## Resource-domain ownership

The resource domain now exposes:

```text
nominal physical supply
        x
availability fraction
        =
accessible physical flow
```

`ResourceSupplyState` owns this result.

The resource primitive deliberately does not decide why availability changed.
Future resource, energy, environmental, infrastructure, military, or policy
mechanisms may compute that state.

## Economy coupling

The live economy no longer treats source inflow as necessarily unconditional.

Instead:

```text
ResourceSupplyState
        |
        v
accessible source inflow
        |
        v
AggregateProducer inventory
        |
        v
physical production
        |
        v
TransportCorridor / MarketNodeAccess
        |
        v
GoodMarket
        |
        v
downstream producer
```

The economy consumes physical availability and retains its own native mechanics.

No scripted production modifier is required.

## What the proof demonstrates

The convergence test begins with full resource availability and verifies normal
upstream production.

It then changes source availability to zero through the typed resource coupling.

The next authoritative simulation tick shows:

- nominal source capability remains unchanged;
- availability falls to zero;
- accessible physical inflow falls to zero;
- upstream industrial output falls to zero through normal input constraints.

This is a genuine causal propagation path:

```text
resource-domain state change
â†’ accessible physical supply change
â†’ inventory/input constraint
â†’ production change
```

## Scope boundary

This slice is intentionally minimal.

It does not yet model:

- reserves or depletion;
- extraction capital;
- grades or resource quality;
- electricity;
- refinery/process compatibility;
- water;
- strategic stockpiles;
- substitutions;
- multiple source nodes;
- policy allocation;
- military priority;
- environmental causes.

Those are future domain mechanisms built on top of the coupling boundary, not
reasons to expand `ResourceSupplyState` into a universal world model.

## Architectural rule

Use domain-native causes and typed consequences.

Examples:

```text
drought
â†’ hydro availability
â†’ accessible electricity
â†’ industrial uptime

sanctions
â†’ import access
â†’ accessible feedstock
â†’ refinery/chemical output

mine disruption
â†’ ore availability
â†’ smelter input constraint
â†’ metal output

grid failure
â†’ accessible power
â†’ process capacity
â†’ industrial output
```

The receiving domain should generally not need to know the originating cause.

Do not turn this into one universal causal DAG or one generic modifier system.