# VERTICAL-002 â€” Session position bootstrap

Status: position occupancy is connected to the live `InstanceManager` command/authority runtime.

## Live ownership

`InstanceManager` now owns `PlayerManager` alongside:

- `AuthorityRegistry`;
- `OrderedCommandRuntime`;
- `CommandAdmissionRuntime`.

The legacy `CountryInstance*` inside `PlayerManager` remains for compatibility.

## Bootstrap

Before session start, ruleset/session code can install:

```text
PositionOccupancy
+
AuthorityGrant[]
```

The stable `position_id` becomes the authority actor identity. `controller_id` never does.

## Command submission

`submit_occupied_position_command()` derives actor and default jurisdiction from the occupied position.

The caller supplies command type and payload, not sovereign identity.

## Mobilization compatibility proof

`queue_occupied_legacy_mobilise()` wraps the existing authorized mobilization bridge:

```text
controller
 -> occupied position
 -> derived authority actor/jurisdiction
 -> command admission
 -> durable command log
 -> legacy set_mobilise GameAction
```

## No-player-privilege invariant

Human and AI controllers occupying the same position receive the same authority identity and grants.

## Lifecycle boundary

Bootstrap is only permitted before the game session starts.

Elections, appointments, resignations, coups, delegation, vacancy, multiplayer reassignment, and
mid-session transfers belong to later politics/institutional lifecycle work.

## Next

VERTICAL-003 begins the polity/state-capacity vertical. It must introduce a concrete named capacity
required by a real subsystem, not a universal `state_capacity` master scalar.