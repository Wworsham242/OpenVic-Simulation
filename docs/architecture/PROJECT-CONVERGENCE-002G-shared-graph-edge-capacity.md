# PROJECT-CONVERGENCE-002G â€” Shared graph-edge capacity

Status: graph-routed shared physical edge contention proof.

## Purpose

Unify the two major logistics advances from earlier convergence slices:

- graph-derived route selection;
- shared physical capacity.

Multiple independently routed resource flows can now select paths that overlap
on the same logistics edge. Those flows then compete for the physical
throughput of that shared edge.

## Domain ownership

Resource domain owns:

```text
physical source capability
source availability
source identity
```

Logistics graph owns:

```text
route discovery
selected path
edge identity
edge capacity
edge open/closed state
shared edge contention
```

Industry owns:

```text
production from actually delivered inputs
```

The resource domain supplies the requested physically accessible quantity to
the logistics allocator. It does not infer route capacity.

## Batch graph allocation

Each graph-routed source provides:

```text
flow id
source node
destination node
requested physical quantity
```

The graph:

1. derives one deterministic path for each flow;
2. records each edge used by every path;
3. sums requested quantities on each edge;
4. compares aggregate demand with that edge's effective capacity;
5. computes the capacity fraction imposed by that edge;
6. constrains each flow by the tightest edge fraction on its path.

This makes shared infrastructure emerge from overlapping route selections.

## Causal proof

Two mines each have 2 units of physically accessible supply.

Their feeder edges are independent:

```text
mine A â†’ feeder A â”
                  â”œâ†’ shared trunk â†’ industry
mine B â†’ feeder B â”˜
```

Feeder capacity:

```text
mine A feeder = 2
mine B feeder = 2
```

Shared trunk:

```text
capacity = 3
```

Demand on trunk:

```text
mine A = 2
mine B = 2
total = 4
```

The graph allocator constrains both flows proportionally:

```text
mine A delivered = 1.5
mine B delivered = 1.5
total delivered = 3
```

At the live-economy boundary:

```text
physical resource supply = 4
shared graph-edge capacity = 3
â†’ delivered input = 3
â†’ unmet input = 1
â†’ industrial output = 3
```

No source fails and no route is manually tagged with a separate shared-capacity
identifier.

The overlap is discovered from the actual graph paths.

## Architectural rule

Graph edges are physical logistics capacities.

Do not represent the same outcome as:

```text
mine output -25%
```

or:

```text
industry efficiency -25%
```

Those would erase whether the actual cause was:

- source failure;
- route closure;
- alternate-path exhaustion;
- shared edge contention;
- allocation policy.

Preserving those distinctions is required for causal inspection and meaningful
player intervention.

## Relationship to 002D

002D introduced `SharedTransportCapacity` as an explicit shared bottleneck.

002G proves the more natural network form:

```text
shared capacity is attached to the edge itself
```

The old explicit shared-capacity abstraction remains available for
compatibility and specialized coarse seams, but graph-routed flows no longer
need a separate manual declaration merely to share a graph edge.

## Scope boundary

002G still uses a deliberately simple allocation rule:

```text
proportional allocation by requested physical quantity
```

Not yet modeled:

- priority classes;
- military versus civilian preference;
- contractual reservations;
- bidding;
- queue order;
- transit time;
- in-transit inventories;
- partial rerouting after allocation;
- iterative flow redistribution;
- congestion cost;
- transport mode economics.

## Next architectural pressure

The most important next extension is allocation-aware rerouting.

Current 002G behavior:

```text
select path
â†’ discover shared bottleneck
â†’ reduce delivered flow
```

Next:

```text
select preferred path
â†’ shared bottleneck constrains flow
â†’ residual unmet flow searches alternate path
â†’ alternate edges consume their own shared capacity
â†’ only remaining shortfall reaches downstream systems
```

That will connect routing optionality and shared contention into a more complete
strategic flow solver while remaining coarse-grained and deterministic.