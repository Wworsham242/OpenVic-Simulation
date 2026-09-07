# PROJECT-CONVERGENCE-002H â€” Allocation-aware rerouting

Status: residual-flow rerouting proof after shared-edge allocation.

## Purpose

Extend shared graph-edge allocation so capacity-constrained flow can use spare
capacity on an alternate path before its shortfall becomes downstream unmet
demand.

This joins:

- deterministic route selection;
- shared edge contention;
- residual rerouting.

## Allocation sequence

002H preserves 002G primary allocation first.

For each graph-routed flow:

```text
requested physical flow
â†’ deterministic preferred path
â†’ shared-edge allocation
â†’ primary allocated quantity
â†’ residual shortfall
```

Only the residual is then considered for rerouting.

The rerouting stage:

1. calculates graph-edge capacity remaining after primary allocations;
2. excludes the constrained flow's primary path;
3. excludes edges with no remaining physical capacity;
4. finds another deterministic route;
5. constrains residual flow by remaining capacity on the alternate path;
6. reserves that alternate capacity;
7. adds the rerouted quantity to delivered flow.

## Causal proof

One source has:

```text
physical supply = 2
preferred route capacity = 1
alternate route capacity = 1
```

Primary allocation:

```text
requested = 2
preferred route delivers = 1
residual = 1
```

Residual routing then discovers the alternate:

```text
alternate spare capacity = 1
rerouted = 1
total delivered = 2
unmet = 0
```

When one alternate edge closes:

```text
preferred route still delivers = 1
alternate unavailable
residual = 1
unmet = 1
```

The live-economy proof therefore moves from full production to constrained
production only when routing optionality is actually lost.

## Determinism

Residual rerouting is processed in stable flow-id order.

This is intentionally simple. It avoids nondeterministic dependence on vector
layout or hash iteration while the strategic flow semantics are still being
built.

## Architectural rule

Rerouting does not create supply or capacity.

Each stage operates on remaining physical quantities:

```text
source accessible supply
â†’ primary path allocation
â†’ remaining source flow
â†’ remaining edge capacities
â†’ alternate path allocation
â†’ delivered physical supply
```

The resource domain owns supply.

The logistics domain owns route selection and capacity consumption.

Industry remains a consumer of delivered inventory.

## Relationship to earlier convergence

002F:
```text
find one deterministic route
```

002G:
```text
multiple routes overlap
â†’ shared edge capacity constrains them
```

002H:
```text
primary allocation leaves residual
â†’ residual searches unused alternate capacity
```

Together these now form a minimal strategic logistics flow substrate rather
than a modifier system.

## Scope boundary

002H is still intentionally limited.

Residual rerouting currently:

- uses at most one alternate path per flow in this pass;
- processes flows sequentially in stable flow-id order;
- excludes the entire primary path when searching the alternate;
- does not iterate repeatedly until all possible residual flow is exhausted.

Not yet modeled:

- multiple successive alternate paths;
- globally optimal multi-commodity flow;
- policy priorities;
- military/civilian classes;
- reserved capacity;
- queue order;
- transit time;
- in-transit inventory;
- transport costs;
- mode switching;
- congestion feedback.

## Next architectural pressure

The next decision should not automatically be "add another routing feature."

The logistics substrate is now substantial enough to pause for a convergence
review:

```text
resource source
â†’ availability
â†’ buffer
â†’ graph path
â†’ shared edge allocation
â†’ residual rerouting
â†’ delivered supply
â†’ production
```

The useful next step is to inspect whether the old explicit corridor,
alternative-route, and standalone shared-capacity seams should begin collapsing
behind the graph substrate, while retaining compatibility where necessary.

That review should also decide whether the next implementation pressure is:

- transit time / in-transit inventory;
- transport cost and mode;
- priority allocation;
- infrastructure damage/repair;
- or another resource/energy domain mechanism.

Do not continue adding routing complexity merely because the graph now exists.