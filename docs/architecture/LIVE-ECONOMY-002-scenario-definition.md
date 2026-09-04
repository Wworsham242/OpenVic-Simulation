# LIVE-ECONOMY-002 â€” Scenario-owned live economy definition

LIVE-ECONOMY-002 removes the temporary runtime bootstrap that selected the
first three tradeable goods and manufactured its own production processes.

Configuration now belongs to `LiveEconomyScenarioDefinition`.

It specifies:

- upstream production process;
- downstream production process;
- upstream capacity/utilization;
- downstream capacity/utilization;
- source inflow good;
- source inflow per daily tick;
- source market node;
- destination market node;
- ordered coarse corridor legs.

`LiveEconomyRuntime` owns only mutable runtime state.

`InstanceManager::setup()` no longer scans the goods registry or creates
temporary `ProductionType` objects. It asks `EconomyManager` for an explicitly
configured scenario. No configured scenario means the live aggregate economy
remains disabled rather than silently inventing world state.

Tests prove that scenario-owned corridor capacity and source inflow change
actual market trade and downstream production.

This slice intentionally does not add a text-file parser yet. It establishes
the definition/ruleset ownership boundary first so a later loader can populate
the scenario without changing the runtime again.