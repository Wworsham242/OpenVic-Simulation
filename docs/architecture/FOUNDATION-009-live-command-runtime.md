# FOUNDATION-009 â€” Live command runtime integration

Status: generalized authority and ordered-command primitives are now owned by the production `InstanceManager`.

## Why this slice exists

FOUNDATION-001 through FOUNDATION-008 proved generalized time, events, persistence, RNG,
commands, replay, authority, and jurisdiction mostly as isolated primitives.

FOUNDATION-009 stops that accumulation and connects the command path to the live runtime.

## Production ownership

`InstanceManager` now owns:

```text
SimulationTimeline
AuthorityRegistry
OrderedCommandRuntime
CommandAdmissionRuntime
```

`CommandAdmissionRuntime` references the owned registry and ordered-command runtime, so there is
one live authority source and one live accepted-command log per game instance.

## Submission path

```text
caller
  |
  | actor_id
  | command_type
  | jurisdiction_id
  | payload
  v
InstanceManager::submit_authorized_command()
  |
  | authoritative SimulationTimeline::current_time()
  v
CommandAdmissionRuntime
  |
  +-- invalid / unknown / unauthorized --> rejected, no log mutation
  |
  `-- authorized
        |
        v
OrderedCommandRuntime
        |
        v
durable accepted-command sequence
```

The caller does not supply simulation time. The authoritative game instance stamps accepted commands.

## Compatibility boundary

Legacy `game_action_queue` and `GameActionManager::execute_game_action()` remain unchanged.

FOUNDATION-009 intentionally does **not** translate or execute legacy game actions through the new
path. That is the next controlled migration step.

## Why no synthetic InstanceManager runtime test

Constructing `InstanceManager` requires the full Victoria definition/runtime dependency graph.
A fake partial instance would test a configuration that production never uses.

Instead:

1. the production constructor must compile with the owned reference-based command runtime;
2. compile-time API requirements guard the public generalized seam;
3. the full existing OpenVic test executable remains the regression gate.

## Next

FOUNDATION-010 should route one real legacy action through:

```text
actor
 -> authority
 -> jurisdiction
 -> accepted command
 -> compatibility adapter
 -> existing GameAction execution
 -> existing game-state effect
```

Only after that proof should broader `GameAction` migration begin.