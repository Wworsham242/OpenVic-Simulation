# PROJECT-CONVERGENCE-002B â€” Resource network and buffer

Status: plural/spatial resource-source and resilience proof.

## Purpose

Extend the 002A resource causal substrate from one scalar source into multiple
identified sources with location/node identity and a finite buffer stockpile.

The goal is to represent resilience as an emergent property of physical supply
diversification and stored inventory rather than as a generic percentage bonus.

## Resource network

The resource domain now supports:

```text
source A
source B
source C
   |
   +-- source identity
   +-- node/location identity
   +-- nominal supply
   +-- source-specific availability
   |
   v
aggregate accessible source flow
   |
   +-- direct consumer delivery
   +-- finite buffer stock/store
   |
   v
delivered physical flow
```

Each source remains independently disruptable.

The node identity is carried now so later work can make source-to-consumer
access depend on pipelines, rail, ports, grids, corridors, sanctions, and other
network constraints. 002B does not yet solve route selection.

## Buffer behavior

The buffer is a physical stock.

For each demand step:

1. accessible source flow is used first;
2. a shortfall draws the buffer;
3. a surplus can replenish the buffer;
4. any remaining shortfall is explicit unmet demand.

This means a disruption can be masked temporarily by inventory.

## Causal proof

The proving configuration has two sources:

```text
mine A = 2 units/tick
mine B = 2 units/tick
buffer = 4 units
industrial demand = 4 units/tick
```

When mine A is disabled:

```text
Day 1:
mine B 2 + buffer 2
â†’ delivered 4
â†’ industrial output remains 4
â†’ buffer falls to 2

Day 2:
mine B 2 + buffer 2
â†’ delivered 4
â†’ industrial output remains 4
â†’ buffer falls to 0

Day 3:
mine B 2 + buffer 0
â†’ delivered 2
â†’ unmet feedstock 2
â†’ industrial output falls to 2
```

No delayed scripted modifier exists. The delay arises because physical inventory
absorbs the supply shock until it is exhausted.

## Backward compatibility

The 002A single-source path remains represented as a default one-source
`ResourceSupplyNetwork`.

Existing callers can still set the legacy live resource availability, which now
delegates to the default scenario source.

This avoids introducing a second resource model.

## Scope boundary

002B does not yet model:

- route-level source accessibility;
- source quality or grade;
- processor/feedstock compatibility;
- strategic allocation priorities;
- reserve policy;
- replenishment policy;
- spoilage or storage losses;
- carrying cost;
- extraction investment;
- power grids;
- water;
- substitution.

Those should be added through specific domain mechanisms rather than expanding
the resource network into a universal simulation object.

## Next architectural pressure

The next meaningful extension should use the source node identities rather than
merely storing them.

A likely next proof is:

```text
multiple physical sources
â†’ different transport corridors / access conditions
â†’ disruption or bottleneck on one path
â†’ accessible delivered supply changes
â†’ buffer absorbs part of the shock
â†’ downstream production reacts
```

That is the bridge from resource plurality into real logistics and energy
network behavior.