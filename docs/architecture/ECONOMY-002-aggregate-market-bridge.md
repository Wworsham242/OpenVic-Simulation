# ECONOMY-002 â€” Aggregate producer market bridge

ECONOMY-002 connects the physical aggregate producer introduced in ECONOMY-001 to OpenVic's existing market-order kernel.

```text
input shortfall -> BuyUpToOrder -> GoodMarket clearing -> actual purchased quantity -> input inventory
production -> output inventory -> MarketSellOrder -> GoodMarket clearing -> actual sold quantity removed
```

The producer still does not own a market, country, transport network, credit system, or balance sheet.

The caller supplies money available for an input order. The bridge records cleared spending and sales revenue only as transaction observations.

Tests use the real `GoodMarket::execute_orders()` path and prove:
- shortfalls derive from recipe requirements and desired output;
- full fills become inventory;
- partial fills add only actual purchased quantity;
- partial fills constrain subsequent production;
- only actually sold output leaves inventory;
- zero/non-input orders are rejected.

Next: ECONOMY-003 should connect at least two aggregate producers through the market to prove an upstream/downstream supply chain before adding spatial trade accessibility or logistics.