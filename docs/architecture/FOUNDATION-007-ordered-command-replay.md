# FOUNDATION-007 â€” Ordered command and replay runtime

Status: generic accepted-command log and replay cursor established; legacy actions untouched.

## Purpose

Introduce the deterministic command-ordering primitive that will eventually sit between all
human/AI/script intent and authoritative simulation mutation.

```text
human / AI / institution / script
             |
          intent
             |
   [future validation layers]
             |
       accepted command
             |
    OrderedCommandRuntime
             |
       deterministic log
             |
     authoritative mutation
```

FOUNDATION-007 implements the accepted-command/log portion only.

## Command envelope

Each accepted record contains:

- stable contiguous sequence number;
- submission `SimTime`;
- opaque actor id;
- command type id;
- opaque byte payload.

Actor identity is intentionally not country-specific. A human-controlled office, AI-controlled
office, central bank, legislature, corporation, military command, or script can all enter through
the same eventual path.

## Ordering rule

Acceptance sequence is authoritative. `submitted_at` is recorded provenance, not a secondary sort
key. This avoids unstable reordering when several actors submit at the same simulation time.

## Replay

Replay owns a cursor into the immutable accepted-command prefix. Capture/restore preserves both the
log and cursor, and restore validates a contiguous canonical sequence.

## Relationship to legacy GameAction

`GameActionManager` is not removed or rewritten in this slice.

Later compatibility work can translate a legacy `game_action_t` into a command envelope, then execute
the existing visitor after the command has passed the new validation/order path. Modern command types
can be added without making the old Victoria action variant the permanent architecture.

## Explicit non-goals

FOUNDATION-007 does not yet implement:

- authority or jurisdiction checks;
- institutional process/delay;
- command rejection/modification semantics;
- actor registry;
- legacy `GameAction` adapter;
- network/multiplayer command transport;
- physical serialization format;
- domain-specific command schemas.

Those are separate layers and should not be conflated with deterministic ordering.