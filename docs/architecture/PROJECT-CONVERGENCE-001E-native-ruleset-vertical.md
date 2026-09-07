# PROJECT-CONVERGENCE-001E â€” Native ruleset vertical

Status: implementation proof.

## Purpose

Prove that application-owned native content can configure and run the
authoritative OpenVic-Simulation C++ runtime without a Victoria II data root,
legacy bookmark/history bootstrap, or the Rust ScenarioHost.

This is the first end-to-end native-content vertical after 001D removed the
bookmark requirement from `InstanceManager` construction.

## Proven ownership chain

```text
application-owned native files
        |
        +-- goods
        +-- aggregate production
        +-- live-economy scenario
        |
        v
GameManager
        |
        v
DefinitionManager / EconomyManager
        |
        v
InstanceManager
        |
        +-- PositionSessionBootstrap
        +-- AuthorityRegistry
        +-- SimulationTimeline / SimulationClock
        +-- LiveEconomyRuntime
        |
        v
authoritative simulation advances
```

No second world owner, scheduler, command runtime, or economy is introduced.

## Native causal proof

The proving package contains a deliberately small production chain:

```text
native_iron_ore
      |
      v
native_primary_steel
      |
      v
native_industrial_machinery
```

The scenario also supplies source inflow, a transport corridor, and producer
capacities. The vertical test proves that after native setup and session start:

- the live economy is configured from application-owned data;
- authoritative simulation time advances;
- the live economy completes at least one daily tick;
- upstream production is positive;
- intermediate trade is positive.

This is a causal state/flow proof, not a scripted modifier or event-chain proof.

## Position and authority

The same vertical binds an application-defined position and jurisdiction before
session start, then submits a command through that occupied position.

Authority belongs to the position rather than the human controller. This
preserves the project rule that human and AI occupants use the same simulation
authority model.

## Scope boundary

`GameManager::load_native_economy_bootstrap()` is a convergence seam, not the
final ruleset/package API.

Its fixed filenames and economy-only shape are intentionally narrow so the
project can prove native ownership before designing a generalized package
manifest. Do not expand this method into a universal loader.

The next ruleset work should generalize package composition only when another
native domain needs it.

## Causal architecture rule

Future domains should follow the same pattern:

```text
domain-owned state / capacity / inventory / decisions
        |
        v
domain-native mechanism
        |
        v
typed cross-domain state or flow
        |
        v
another domain reacts through its own mechanism
```

Do not replace this with one imperial causal framework or a universal world DAG.

## Regression coverage

001E also adds a direct regression for the empty-domain `IndexedFlatMap`
behavior repaired during 001D. Minimal native rulesets are allowed to contain
empty definition domains without manufacturing fake Victoria-era content.