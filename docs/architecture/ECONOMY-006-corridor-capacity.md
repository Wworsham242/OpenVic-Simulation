# ECONOMY-006 â€” Coarse transport corridor capacity

ECONOMY-006 adds the first actual logistics mechanism above the market-node/access seam.

This is no longer merely an identity or interface slice. It calculates a causal transport constraint that can reduce real market purchases and downstream production.

## Model

A `TransportCorridor` connects:

```text
source market node
        â†“
transport/access leg
        â†“
transport/access leg
        â†“
...
        â†“
destination market node
```

Each coarse `TransportLeg` has:

```text
nominal_capacity
Ã— availability_fraction
gated by open/closed
= effective_leg_capacity
```

The corridor capacity is:

```text
minimum effective capacity of all legs
```

The tightest leg is therefore the bottleneck.

## Intended resolution

A leg may represent:

- a major rail corridor;
- a port complex throughput step;
- a pipeline segment;
- a shipping chokepoint;
- a border crossing;
- a strategic bridge/tunnel/ferry system;
- another aggregate transport seam.

It does not represent individual trucks, trains, ships, containers, or wagons.

## Integration

The corridor produces a `DeliverableSupply` and publishes it into the directional `MarketNodeAccessTable`.

```text
TransportCorridor
        â†“
bottleneck capacity
        â†“
DeliverableSupply
        â†“
MarketNodeAccessTable[source,destination]
        â†“
aggregate producer buy-order cap
        â†“
GoodMarket
        â†“
actual inventory acquired
        â†“
production
```

## Proven behavior

Tests prove:

1. the tightest leg determines corridor capacity;
2. degraded leg availability reduces route capacity;
3. a closed leg closes the corridor;
4. corridor output publishes directionally to source/destination access;
5. a 4-unit corridor bottleneck limits a buyer with 10-unit demand to a 4-unit real market purchase;
6. the resulting inventory shortage limits actual production to 4 without a scripted penalty;
7. physical scarcity remains binding when supply is below route capacity.

## What this still does not do

This slice does not yet choose between multiple possible routes.

It also does not model:

- congestion competition between multiple goods/users;
- route cost;
- transit time;
- tariffs;
- sanctions;
- interdiction probability;
- inventory in transit;
- port queues;
- multimodal transfer costs.

Those should be added only when their vertical becomes necessary.

## Stop condition / gameplay direction

The economy vertical now has enough causal skeleton that we should avoid spending many more iterations on isolated abstractions.

After ECONOMY-006, the preferred next move is not an endless ECONOMY-007/008/009 architecture chain.

The next step should be a **live economy bootstrap vertical** that instantiates a tiny playable/test world:

```text
real session
â†’ a few market nodes
â†’ aggregate producers
â†’ goods
â†’ corridors
â†’ recurring simulation cadence
â†’ observable production / prices / shortages
```

That will move the work from tested mechanisms toward an actual running game state.