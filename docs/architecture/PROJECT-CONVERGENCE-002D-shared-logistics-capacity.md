# PROJECT-CONVERGENCE-002D â€” Shared logistics capacity

Status: shared strategic logistics bottleneck proof.

## Purpose

Extend 002C from independent source-specific corridors to a shared physical
capacity constraint that can be consumed by multiple resource flows.

The goal is to represent competition for rail lines, pipeline segments, ports,
border crossings, shipping chokepoints, and other strategic logistics capacity
without applying generic production penalties.

## Domain ownership

Resource domain owns:

```text
physical source capability
source availability
source identity/location
```

Route/corridor logistics owns:

```text
source-specific path capacity
route access
route availability
```

Shared-capacity logistics owns:

```text
one physical shared segment
effective capacity
allocation among competing flows
```

Industry remains a consumer of delivered physical inputs.

## Shared-capacity mechanism

`SharedTransportCapacity` receives requested physical flow volumes.

If total demand is less than or equal to physical capacity, all requests are
fully served.

If demand exceeds capacity, 002D allocates the available throughput
proportionally to requested volume.

Example:

```text
mine A requests 2
mine B requests 2
shared rail capacity = 3

total request = 4
allocated A = 1.5
allocated B = 1.5
```

Total delivered throughput remains physically conserved at 3.

The proportional rule is intentionally neutral. Future policy, priority,
military allocation, contractual rights, or market bidding mechanisms can
change requested volumes or replace the allocation policy.

The capacity object itself should not decide political priority.

## Causal proof

Two mines are physically intact and each can supply 2 units/tick.

Each source-specific route can carry 2 units.

Both routes depend on the same `shared_rail` capacity of 3 units.

The result is:

```text
physical supply = 4
individual route capacity = 4
shared physical capacity = 3
        â†“
deliverable resource flow = 3
        â†“
unmet physical input = 1
        â†“
industrial output = 3
```

No source is damaged and no route is individually closed.

The shortage is caused solely by shared infrastructure contention.

## Architectural rule

Shared capacity is a logistics-domain physical constraint.

Do not model the same effect as:

```text
industry modifier = -25%
```

or:

```text
resource availability = 75%
```

Those would erase the causal distinction between:

- source failure;
- route failure;
- shared bottleneck congestion;
- allocation policy.

Preserving those distinctions is necessary for later player intervention and
causal inspection.

## Scope boundary

002D does not yet model:

- alternative path finding;
- rerouting;
- transit time;
- in-transit inventory;
- congestion queues;
- transport cost;
- mode choice;
- priority classes;
- military/civilian allocation;
- contracts;
- infrastructure repair;
- dynamic route discovery.

Those remain future logistics-domain mechanisms.

## Next architectural pressure

The next useful extension is alternative routing:

```text
primary route
â†’ shared bottleneck congested or closed
â†’ alternative route exists
â†’ some flow reroutes
â†’ alternative capacity/cost/time becomes binding
â†’ remaining unmet flow propagates downstream
```

That will turn the current coarse shared bottleneck into the beginning of a
real strategic logistics network without requiring microscopic vehicle-level
simulation.