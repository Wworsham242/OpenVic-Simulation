# ECONOMY-004 â€” Deliverable supply / market accessibility seam

ECONOMY-004 introduces the first explicit boundary between physical supply and market-accessible supply.

## Architectural rule

The market-clearing kernel does **not** own geography, logistics, sanctions, tariffs, insurance, border policy, or transport networks.

Instead:

```text
physical supply
        â†“
domain-native access / logistics mechanisms
        â†“
DeliverableSupply
        â†“
deliverable quantity
        â†“
market order quantity
        â†“
existing GoodMarket clearing
```

`DeliverableSupply` is a quantity envelope, not a causal model.

Its inputs may eventually be produced by several independent mechanisms.

## Envelope

```text
physical_supply
Ã— accessible_fraction
capped by delivery_capacity
gated by access_allowed
= deliverable_quantity
```

This is intentionally coarse.

Examples of future contributors:

- shipping capacity;
- rail or pipeline capacity;
- port throughput;
- sanctions;
- embargoes;
- export controls;
- customs/border closure;
- insurance/finance denial;
- geographic isolation;
- regional market membership.

Those causes remain outside the envelope.

## Market bridge change

`AggregateProducerMarketBridge::make_input_buy_order()` now accepts an optional `max_deliverable_quantity`.

The requested market quantity becomes:

```text
min(
    physical input shortfall,
    max deliverable quantity
)
```

If no delivery/access cap is supplied, legacy ECONOMY-002/003 behavior is unchanged.

## Proven behavior

ECONOMY-004 tests prove:

1. physical supply can exceed deliverable supply;
2. access fractions and delivery capacity combine deterministically;
3. denied access produces zero deliverable quantity;
4. a buyer with an 8-unit physical shortfall but only 3 deliverable units places only a 3-unit market order;
5. abundant global supply does not override denied access;
6. an access/capacity shock propagates through actual acquired inventory into lower production;
7. no scripted production penalty is required.

## Important limitation

This slice does **not** yet distinguish multiple seller origins inside one `GoodMarket`.

That is deliberate.

The new seam establishes where source-specific routing/accessibility will constrain orders later, without prematurely rewriting the market kernel.

## Next

Before implementing tariffs or sanctions directly, ECONOMY-005 should introduce **market nodes / source-destination access identity** at the lowest sufficient resolution.

The purpose is to answer:

```text
seller/source node
        â†“
buyer/destination node
        â†“
what delivery path/access rule applies?
```

Only after that identity seam exists should transport networks, regional market segmentation, sanctions, tariffs, and route capacity attach to concrete source-destination relationships.