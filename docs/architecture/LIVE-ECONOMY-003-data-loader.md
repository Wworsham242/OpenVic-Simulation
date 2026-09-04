# LIVE-ECONOMY-003 â€” Optional mod-data loader

LIVE-ECONOMY-003 connects the scenario-owned live economy definition to
OpenVic's existing moddable data-loading pipeline.

## File

The loader looks for:

`common/live_economy.txt`

The file is optional. If absent, native/base OpenVic data continues loading
unchanged and the live aggregate economy remains disabled.

## Load order

The file is loaded immediately after `common/production_types.txt`.

That means it can safely resolve:

- goods;
- production processes.

It does not need to duplicate those definitions.

## Initial schema

Example:

```text
upstream_process = scenario_upstream
downstream_process = scenario_downstream

upstream_capacity = 4
upstream_utilization = 1
downstream_capacity = 4
downstream_utilization = 1

source_inflow_good = feedstock
source_inflow_per_daily_tick = 4

source_node = 11
destination_node = 22

corridor_capacities = { 10 6 8 }
```

Each corridor capacity becomes one ordered coarse `TransportLeg` with
availability 1 and `open = true`.

Dynamic degradation/closure remains runtime state and is intentionally not
encoded into the initial scenario file yet.

## Architectural meaning

The live economy path is now:

```text
mod/ruleset text
â†’ EconomyManager scenario definition
â†’ InstanceManager
â†’ LiveEconomyRuntime
â†’ aggregate producers
â†’ corridor/access
â†’ real GoodMarket
â†’ observable status
```

No parallel configuration subsystem is introduced.

The next slice should place an actual modern ruleset data root/file into the
application layer and expose the resulting live status through the existing
Godot presentation bridge.