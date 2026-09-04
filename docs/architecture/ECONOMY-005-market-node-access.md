# ECONOMY-005 â€” Source/destination market-node access identity

ECONOMY-005 introduces the missing identity seam for directional market access.

## Problem

ECONOMY-004 established:

```text
physical supply
â†’ deliverable quantity
â†’ market order
```

But the deliverable envelope was not yet attached to a concrete origin and destination.

Modern trade constraints are directional:

```text
source A â†’ destination B
```

is not necessarily equivalent to:

```text
source B â†’ destination A
```

and two destinations may have very different access to the same source.

## Market node identity

A new strong typed index is added:

```text
market_node_index_t
```

It uses OpenVic's existing typed-index mechanism.

A market node is intentionally abstract and ruleset-defined.

It may represent, at the lowest sufficient resolution:

- a national market;
- a regional market;
- a port complex;
- a pipeline zone;
- a strategic industrial cluster;
- a major isolated island/system;
- another economically meaningful trading locus.

The engine does not hard-code those categories.

## Access key

```text
MarketAccessKey {
    source_node
    destination_node
}
```

The pair is directional and deterministic.

## Access table

`MarketNodeAccessTable` maps a source/destination pair to a `DeliverableSupply`.

It answers:

```text
source
  â†“
destination
  â†“
what quantity is currently deliverable?
```

The table does **not** solve routes.

It does not contain:

- rail graphs;
- shipping lanes;
- pipelines;
- tariffs;
- sanctions logic;
- customs;
- insurance;
- finance;
- military interdiction.

Those mechanisms may later calculate the envelope stored for each pair.

## Market integration

The existing ECONOMY-004 order cap remains the market seam:

```text
MarketNodeAccessTable
        â†“
source/destination deliverable quantity
        â†“
AggregateProducerMarketBridge
        â†“
max_deliverable_quantity
        â†“
GoodMarket
```

`GoodMarket` remains unaware of geopolitical/logistics semantics.

## Proven behavior

ECONOMY-005 tests prove:

1. sourceâ†’destination identity is directional;
2. two destinations can have different access to the same source;
3. the pair-derived deliverable quantity caps a real aggregate market order;
4. one blocked source does not block another source to the same destination;
5. lookup results are invariant to insertion order.

## Next

ECONOMY-006 should add the first **route-capacity composition** above this identity seam.

Do not build per-truck logistics.

A first useful model is a coarse path/corridor capacity calculation:

```text
source node
â†’ one or more transport/access legs
â†’ bottleneck capacity
â†’ destination node
â†’ DeliverableSupply
```

That can later represent pipelines, rail corridors, ports, shipping chokepoints, border crossings, and sanctions/interdiction without changing the production or market-clearing kernels.