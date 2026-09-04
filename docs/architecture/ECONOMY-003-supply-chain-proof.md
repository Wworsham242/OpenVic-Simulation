# ECONOMY-003 â€” Aggregate supply-chain proof

ECONOMY-003 deliberately adds no new universal supply-chain runtime.

Instead, it proves that ECONOMY-001 and ECONOMY-002 already compose into a causal multi-stage production chain through the existing OpenVic market-clearing kernel.

## Proven chain

```text
upstream input inventory
        â†“
upstream AggregateProducer
        â†“
intermediate output inventory
        â†“
AggregateProducerMarketBridge
        â†“
GoodMarket clearing
        â†“
downstream input inventory
        â†“
downstream AggregateProducer
        â†“
final output inventory
        â†“
second GoodMarket
        â†“
external demand
```

## Core scenario

The upstream sector produces 4 units of an intermediate good.

The downstream sector has capacity to produce 4 units of final output, but its recipe requires 2 units of intermediate input per unit of output.

Therefore:

```text
downstream desired output = 4
intermediate required = 8
intermediate actually acquired = 4
downstream actual output = 2
```

No scripted shortage penalty is used.

The reduced output is a physical consequence of the upstream supply constraint.

## Additional proof

A second integration case proves that downstream output can itself be offered into another `GoodMarket`.

If downstream produces 4 units and final demand clears only 3:

```text
3 units sold
1 unit remains in downstream inventory
```

The final case applies an upstream capacity shock and proves that the reduction propagates quantitatively into downstream production through market-cleared physical inventory.

## Architectural decision

Do not add an `AggregateSupplyChainManager`.

The existing primitives already compose:

- `ProductionType`
- `AggregateProducer`
- `AggregateProducerMarketBridge`
- `GoodMarket`

A generalized orchestration layer should only be introduced when a concrete runtime scheduling, ownership, batching, or performance requirement demands it.

## What remains absent

This proof intentionally has no:

- transport capacity;
- route geography;
- tariffs;
- sanctions;
- market-access restrictions;
- trade finance;
- corporate balance sheets;
- labor constraints;
- energy-grid constraints.

Those mechanisms should constrain this causal chain later rather than replace it.

## Next

ECONOMY-004 should address **market accessibility / deliverable supply** before we build more production detail.

The next architectural question is:

```text
physical supply exists somewhere
        â†“
what portion is actually accessible to this buyer?
```

That is the correct seam for transport capacity, trade barriers, sanctions, and regional market segmentation without discarding the existing clearing kernel.