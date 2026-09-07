# PROJECT-CONVERGENCE-002F â€” Deterministic logistics graph

Status: graph-derived route selection proof.

## Purpose

Replace manually enumerated alternate routes with a small deterministic
logistics graph capable of deriving a usable path between coarse market nodes.

This is the first step from explicitly configured corridors toward a strategic
network model.

## Graph model

The graph consists of coarse directed edges.

Each edge has:

- stable edge identity;
- source node;
- destination node;
- one `TransportLeg`;
- nominal capacity;
- availability;
- open/closed state.

The graph does not simulate individual vehicles.

## Deterministic route selection

The route finder uses:

1. usable/open edges only;
2. fewest hops;
3. lexicographically smallest edge-id sequence as the deterministic tie-break.

This makes route selection stable for replay and debugging.

The path result includes:

```text
found/not found
selected edge IDs
path bottleneck capacity
```

## Causal proof

Mine A has two possible paths to industry.

Primary:

```text
mine A
â†’ node 31
â†’ destination

capacity = 2
```

Alternate:

```text
mine A
â†’ node 41
â†’ destination

capacity = 1
```

Mine B has a direct path of capacity 2.

Initially:

```text
graph chooses mine A primary path
mine A delivered = 2
mine B delivered = 2
total delivered = 4
industrial output = 4
```

When one primary edge closes:

```text
primary path becomes unusable
â†’ graph derives alternate path
â†’ mine A delivered = 1
â†’ mine B delivered = 2
â†’ total delivered = 3
â†’ unmet input = 1
â†’ industrial output = 3
```

No manual alternate-route toggle is required.

## Compatibility

002F does not remove the 002C/002D/002E route abstractions.

For sources configured with graph routing, the graph-derived path capacity
overrides the explicit route capacity at the resource-delivery boundary.

Sources not yet migrated to graph routing continue using the existing explicit
corridor and alternate-route mechanisms.

This allows incremental convergence rather than a disruptive rewrite.

## Architectural rule

The logistics graph owns route discovery.

The resource domain does not perform path search.

Industry still consumes delivered physical inputs and remains unaware of how
the logistics route was selected.

The causal chain remains:

```text
resource source
â†’ logistics graph
â†’ selected path
â†’ bottleneck capacity
â†’ delivered supply
â†’ inventory/buffer
â†’ production
```

## Scope boundary

002F does not yet model:

- simultaneous shared-edge capacity consumption;
- path reservations;
- route cost;
- travel time;
- in-transit inventory;
- queues;
- multimodal transfers;
- path priorities;
- military/civilian allocation;
- dynamic infrastructure construction;
- path cache invalidation.

The current finder identifies one deterministic usable path and its bottleneck.

## Next architectural pressure

The next important step is to make graph edges true shared physical capacities.

Multiple flows selecting overlapping graph edges should compete for that edge's
throughput.

That causal progression is:

```text
multiple source/consumer flows
â†’ graph-derived paths
â†’ overlapping edges
â†’ shared edge capacity allocation
â†’ delivered quantities
â†’ buffers/inventories
â†’ downstream consequences
```

At that point, the graph, shared-capacity model, and alternative routing work
from 002Dâ€“002F can begin converging into one logistics network substrate.