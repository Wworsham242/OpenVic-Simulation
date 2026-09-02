# FOUNDATION-003 â€” Controlled runtime integration

Status: first production-runtime seam; compatibility mode only.

## Purpose

Introduce generalized simulation time and scheduled-event ownership into the real `InstanceManager`
runtime without changing the existing Victoria-compatible gameplay order.

## Important distinction

OpenVic already has `SimulationClock`, but that class controls wall-clock pause/speed pacing. It
does not represent authoritative simulated-world time.

FOUNDATION-003 therefore adds a separate `SimulationTimeline`:

- `SimulationClock` remains presentation/runtime pacing.
- `SimulationTimeline` owns generalized elapsed simulation time and scheduled wake-ups.
- `Date today` remains the legacy Victoria calendar authority during compatibility migration.

These roles must not be collapsed.

## Compatibility adapter

For this proof slice only, one successful legacy daily tick advances generalized time by 24 engine
ticks. The value `24` belongs to the legacy `InstanceManager` adapter, not to `SimTime` or
`SimulationTimeline`; the generalized core remains unitless.

The ordering is:

```text
legacy tick requested
      |
      v
advance generalized timeline by 24
      |
      v
advance Date by one legacy day
      |
      v
run unchanged country/map/market/unit sequence
```

If generalized time cannot advance, the legacy tick is refused before Date or gameplay state changes.

## Deliberately NOT done

- no existing gameplay system reads `SimulationTimeline`;
- no scheduled event is automatically dispatched into gameplay;
- no ECS storage requirement is introduced;
- no Date/calendar conversion API is introduced;
- no bookmark persistence format is changed;
- no Godot API is changed;
- no old manager ordering is removed;
- no economy, population, politics, military, market, or map behavior is migrated.

## Why this is safe

This creates a one-way record of generalized elapsed execution only. It cannot affect existing domain
outcomes because no existing domain system consumes it. Full OpenVic-Simulation compilation and the
complete test suite are required before commit.

The next migration slice must prove persistence/replay ownership before any gameplay feature begins
scheduling durable events through this timeline.