# FOUNDATION-002 â€” Timing/event â†” ECS integration proof

Status: isolated proof; no production runtime integration.

## Question

Can the generalized timing primitives introduced by FOUNDATION-001 cooperate with the existing OpenVic ECS scheduler without changing schedule topology, worker-count determinism, serial/parallel parity, or requiring all state to live in ECS?

## Result required before InstanceManager integration

The proof test composes the systems without adding a new authoritative runtime:

- `SimTime` supplies generalized ordered time.
- `Cadence` is consulted from an ECS `should_run` predicate through a small test-only clock singleton.
- `SimulationEventScheduler` remains outside ECS storage and deterministically delivers due events into a bounded test signal before a tick.
- OpenVic ECS continues to own execution ordering and parallel dispatch.
- `schedule_hash()` remains unchanged by cadence skip patterns.
- world output remains identical across worker counts and serial/parallel execution.
- event snapshot/restore reproduces the same future event delivery.

## Architectural implication

This is intentionally a composition proof, not a commitment to put the event scheduler, demographic tables, ledgers, market books, networks, or other specialist stores into ECS.

The intended boundary remains:

```text
generalized time / due events
          |
          v
bounded orchestration
          |
          v
OpenVic ECS DAG for systems that benefit from ECS execution
          |
          +---- specialist non-ECS stores may be called through explicit owners
```

## Non-goals

This slice does not:

- alter `InstanceManager::tick()`;
- reinterpret OpenVic `Date`;
- change Victoria gameplay cadence;
- add campaign persistence fields;
- expose generalized time to Godot;
- port WargameEngine runtime code;
- make ECS the universal state representation.

If this proof fails or reveals awkward coupling, the integration mechanism should be rewritten rather than forcing FOUNDATION-001's exact implementation into OpenVic.