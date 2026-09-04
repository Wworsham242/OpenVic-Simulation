# LIVE-ECONOMY-001 â€” Runtime economy bootstrap

LIVE-ECONOMY-001 moves the aggregate economy from isolated mechanism tests into the actual `InstanceManager` campaign runtime.

## What changes

`InstanceManager` now owns an optional `LiveEconomyRuntime`.

During `setup()` it selects the first three loaded goods that are:

- tradeable;
- not money.

Those goods become a temporary bootstrap chain:

```text
loaded good 1
â†’ upstream aggregate producer
â†’ loaded good 2
â†’ real GoodInstance / GoodMarket
â†’ downstream aggregate producer
â†’ loaded good 3
```

This is intentionally a bootstrap scenario, not final scenario data.

## Daily live tick

The existing live tick already does:

```text
SimulationTimeline +24 ticks
â†’ legacy Date +1 day
â†’ world tick
â†’ MarketInstance::execute_orders()
```

LIVE-ECONOMY-001 inserts:

```text
pre-market live economy phase
â†’ real MarketInstance clearing
â†’ post-market live economy phase
```

The pre-market phase:

1. injects 4 units of temporary raw feedstock;
2. runs upstream aggregate production;
3. calculates corridor deliverability;
4. submits the upstream sell order;
5. submits the downstream buy order into the real loaded `GoodInstance`.

The existing `MarketInstance` then clears orders normally.

The post-market phase:

1. clears completed callback contexts;
2. runs downstream aggregate production from actual purchased inventory;
3. updates observable live status.

## Observable status

`InstanceManager::get_live_economy_status()` exposes:

- configured state;
- completed daily economy ticks;
- upstream output;
- downstream desired output;
- downstream actual output;
- downstream shortage flag;
- intermediate inventories;
- final inventory;
- corridor capacity;
- deliverable intermediate quantity;
- intermediate market price;
- previous-day supply;
- previous-day demand;
- previous-day traded quantity.

This is the first direct presentation seam for a later Godot/UI view.

## Architectural meaning

This is the threshold where the modern economy becomes part of the running game world rather than only a tested library.

The raw-feedstock injection and automatic selection of three loaded goods are temporary bootstrap devices. They exist to prove runtime ownership and cadence before scenario/ruleset loading is generalized.

## Acceptance

The tests prove repeated daily cycles:

```text
day 1:
4 upstream intermediate
â†’ corridor capacity 4
â†’ 4 market-traded
â†’ downstream needs 8
â†’ downstream produces 2

day 2:
repeat
â†’ final inventory accumulates from 2 to 4
```

The status object reads the actual GoodMarket price and previous-day market statistics.

## Next

LIVE-ECONOMY-002 should remove the arbitrary "first three goods" bootstrap and instantiate live producers/nodes/corridors from explicit ruleset/scenario definitions.

After that, OpenVic/Godot should expose the live status through the existing presentation bridge so the running economy becomes visible in the game UI.