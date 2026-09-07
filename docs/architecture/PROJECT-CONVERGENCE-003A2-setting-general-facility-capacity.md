# PROJECT-CONVERGENCE-003A2 â€” Setting-general facility capacity

Status: second OpenVic industrial-substrate generalization.

## Purpose

Reuse inherited OpenVic building definitions and instances as the setting-general
facility/capacity substrate instead of creating a parallel facility hierarchy.

## Change

`BuildingType` now supports `capacity_per_level`.

Legacy Victoria buildings default to zero and retain their existing semantics.

A building/facility with positive `capacity_per_level` is a setting-general
capacity asset.

Installed capacity is derived from:

```text
capacity_per_level Ã— current building level
```

Native facility content is loaded through the existing `EconomyManager` and can
link directly to a setting-general `PROCESS` from PROJECT-CONVERGENCE-003A1.

The native facility catalog also reuses inherited OpenVic:

- maximum level;
- goods construction cost;
- construction time;
- production-type linkage;
- `BuildingInstance` level state;
- expansion runtime.

## Proof

The native steel capacity asset:

- links to `native_ore_to_steel`;
- has five possible levels;
- provides ten capacity units per level;
- has physical goods construction requirements;
- has a 180-day construction duration;
- reports installed capacity from a real `BuildingInstance` level.

## Architectural significance

The intended reusable shape is now:

```text
ProductionType / PROCESS
        â†“
BuildingType / capacity asset
        â†“
BuildingInstance / installed level
        â†“
installed production capacity
```

This can support, at coarse strategic resolution:

- refineries;
- power generation;
- semiconductor fabs;
- steel and aluminum capacity;
- fertilizer and chemical plants;
- mines and processing facilities;
- ports and terminals;
- warehouses;
- defense production;
- other strategic facilities.

## Scope boundary

This milestone does not yet make facility capacity authoritative over production
output.

Construction goods exist in the inherited definition, but the old
`BuildingInstance::expand()` path still contains a TODO for charging/consuming
construction costs.

Those are future causal connections, not claimed as complete here.

## Next

Connect installed facility capacity to the authoritative production runtime so
output cannot exceed installed facility capacity.