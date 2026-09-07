# PROJECT-CONVERGENCE-002E â€” Alternative routing

Status: explicit alternate-path logistics resilience proof.

## Purpose

Extend 002D shared-capacity logistics with explicit alternate routes for resource
sources.

The engine now distinguishes:

```text
physical source capacity
primary route capacity
shared bottleneck capacity
alternate route capacity
```

An alternate path can preserve delivered supply when the primary path is
insufficient or unavailable.

## Mechanism

Each resource source may have:

```text
primary ResourceSourceRoute
+ zero or more ResourceAlternativeRoute entries
```

An alternate route has:

- source identity;
- route identity;
- its own `TransportCorridor`;
- its own access state;
- its own accessibility fraction.

The route contributes usable delivery capacity only while accessible.

Delivered resource flow remains capped by physical source availability inside
the resource network, so multiple paths cannot manufacture supply.

## Causal proof

The proof uses two mines.

```text
mine A physical supply = 2
mine A primary route = 1
mine A alternate route = 1
mine B physical supply = 2
mine B primary route = 2
```

With the alternate route open:

```text
mine A deliverable = 2
mine B deliverable = 2
total delivered = 4
industrial output = 4
```

When only mine A's alternate route closes:

```text
mine A physical supply remains 2
mine A primary deliverable = 1
mine B deliverable = 2
total delivered = 3
unmet input = 1
industrial output = 3
```

The source is not damaged.

The shortage is caused by the loss of routing optionality.

## Architectural rule

Alternative routing is a logistics mechanism, not a production modifier.

Do not model rerouting resilience as:

```text
industry resilience +25%
```

Instead preserve:

```text
source
â†’ available paths
â†’ usable path capacity
â†’ delivered physical flow
â†’ inventory/buffer
â†’ industrial consequence
```

This keeps causal inspection meaningful.

## Scope boundary

002E does not yet provide dynamic graph search.

Routes are explicit configured alternatives.

This is intentional: it proves the causal behavior without introducing a full
routing solver before shared capacity, transit, and allocation semantics are
stable.

Not yet modeled:

- automatic shortest path;
- path cost;
- path travel time;
- in-transit inventory;
- multimodal transfer;
- dynamic graph topology;
- congestion queues;
- policy-based path preference;
- route discovery.

## Next architectural pressure

The next useful extension is a small logistics graph with deterministic route
selection.

A likely progression is:

```text
nodes + edges
â†’ edge capacity / availability
â†’ deterministic route candidates
â†’ shared-edge contention
â†’ flow allocation
â†’ alternate path selection
â†’ delivered inventory
```

That should remain coarse-grained and strategic rather than simulating vehicles
individually.