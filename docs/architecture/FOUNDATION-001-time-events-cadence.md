# FOUNDATION-001 â€” Generalized simulation time, cadence, and scheduled events

Status: first additive proof slice.

## Purpose

Remove the architectural assumption that simulation execution must be organized around one global daily tick without disturbing the existing Victoria-compatible runtime before the replacement foundation is proven.

This slice preserves behavior/invariants from prior WargameEngine work but is a fresh OpenVic-native C++ implementation. No Rust implementation is copied into the runtime.

## Boundaries

Three concepts remain separate:

1. `SimTime` â€” scenario-agnostic ordered engine time. A tick has no calendar meaning in the core.
2. `Cadence` â€” deterministic periodic due-time calculation, including stable staggering by semantic key/domain.
3. `SimulationEventScheduler` â€” deterministic ownership and ordering of exceptional/scheduled wake-ups.

None of these replaces the OpenVic ECS system DAG. The ECS scheduler remains responsible for conflict-aware ordering and parallel execution.

## Deliberately not changed in this slice

- `InstanceManager::tick()`
- Victoria daily gameplay cadence
- ECS scheduling or schedule hashing
- domain state storage
- persistence format
- Godot bridge
- market, population, political, military, or other domain mechanics

This avoids a flag-day conversion and makes regression isolation possible.

## Required invariants

- simulation time ordering uses integer ticks only;
- checked time advance cannot overflow silently;
- cadence period/phase validation is explicit;
- staggered cadence is stable for a semantic key/domain and does not depend on memory addresses or registration order;
- equal-time events execute in monotonic insertion-sequence order;
- selective event retrieval does not disturb unrelated due events;
- event snapshots are canonical and restore rejects noncanonical ordering;
- all primitives are independent of ECS storage and can orchestrate specialist stores later.

## Migration rule

Future integration should preserve these behaviors, not these exact classes, if a cleaner native integration is discovered. No domain conversion may depend on these primitives until compile/test gates pass and an integration review confirms they do not perturb existing deterministic ECS behavior.