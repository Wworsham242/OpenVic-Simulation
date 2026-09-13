# PROJECT-CONVERGENCE-006A2 — Current-Engine World-Scale Synthetic Harness

**Status:** Next implementation increment  
**Predecessor:** `PROJECT-CONVERGENCE-006A1`  
**Canonical starting commit:** `c278d287636bf7baf461750cddc6c9051fb86948`

## Purpose

Establish the actual scale envelope of the current OpenVic-derived C++ engine before adding more domain depth.

006A2 is a measurement and architecture-proof increment. It must not add gameplay rules merely to make the benchmark interesting.

The harness must answer:

1. How much authoritative state fits in memory?
2. Which phases dominate CPU time?
3. How much work is caused by active entities versus empty scanning?
4. How do routing, dirty recomputation, events, provenance and persistence scale?
5. Which existing state layouts or cadences become unacceptable before the target workload?
6. Can the current architecture support the intended world on an ordinary modern gaming PC without a server-class machine?

## Non-goals

006A2 does **not**:

- implement finance;
- implement politics;
- add new military mechanics;
- finish demographics;
- add AI decision depth;
- create a second simulation engine;
- port the old Rust WargameEngine;
- optimize by changing authoritative outcomes;
- reduce detail based on camera zoom;
- invent target performance numbers and then tune to them.

## Native Repository Basis

Reuse the current engine's actual managers, records, graphs, ECS/runtime, fixed-point types and domain state wherever practical.

Synthetic data may be generated programmatically, but it must populate real production types rather than benchmark-only substitutes when the real type is the object under measurement.

A benchmark-only compact surrogate is allowed only when measuring a future planned category that does not yet have a native implementation. Such surrogates must be clearly labeled and excluded from claims that the corresponding domain has been implemented.

## Workload ladder

The harness must support staged scales so the point of failure is visible.

### Tier 0 — smoke
- 10 actors
- 1,000 geography/network points
- 10,000 socioeconomic aggregates
- 500 formations

### Tier 1 — regional
- 40 actors
- 10,000 geography/network points
- 100,000 socioeconomic aggregates
- 5,000 formations

### Tier 2 — large
- 100 actors
- 50,000 geography/network points
- 500,000 socioeconomic aggregates
- 25,000 formations

### Tier 3 — world target
- 200 sovereign-scale actors/equivalents
- 100,000 authoritative geography/network points
- up to 1,000,000 aggregate socioeconomic records
- 50,000 formations

The harness should permit independent scaling of each category.

## Activity ratios

The world target must not assume every object performs expensive work every tick.

At minimum measure:

- quiet world;
- ordinary world;
- crisis world.

Example initial activity profiles, explicitly provisional:

### Quiet
- 1% formations operationally active
- 2% geography/network points dirty
- 1% socioeconomic aggregates needing recomputation
- low event arrival rate

### Ordinary
- 10% formations active
- 5% geography/network points dirty
- 5% socioeconomic aggregates needing recomputation
- moderate event arrival rate

### Crisis
- 35% formations active
- 20% geography/network points dirty
- 15% socioeconomic aggregates needing recomputation
- high event arrival rate and route invalidation

These percentages are workload controls, not empirical claims.

## Cadence model

The harness must distinguish at least:

- high-frequency operational work;
- daily work;
- weekly/planning work;
- monthly structural work;
- event-triggered work.

It must report how many records are merely present versus how many are actually touched by each cadence.

A design that performs a million-record scan every simulated hour must be visible as such.

## Required measured phases

Where present in the current engine, separately time:

- event scheduling/dispatch;
- command admission/replay;
- ECS scheduling;
- production;
- markets;
- population/economic updates;
- environmental/population causal bridges;
- logistics routing;
- capacity allocation;
- shipment advancement;
- military formation/sustainment work;
- dirty recomputation;
- provenance creation;
- snapshot/save work;
- aggregate/index maintenance.

Unknown or inherited work may be grouped initially, but the harness should make it possible to split any phase that becomes material.

## Metrics

For every tier/profile report:

### Memory
- process retained/working-set memory;
- estimated authoritative state bytes where measurable;
- temporary peak memory;
- per-major-category object counts;
- bytes per record where measurable.

### CPU
- wall-clock runtime;
- p50/p95/p99 duration of the principal simulation step or scheduled work batch;
- per-phase cumulative time;
- per-phase maximum time;
- work units per phase.

### Work amplification
- records present;
- records scanned;
- records changed;
- dirty records;
- scheduled events;
- route searches;
- route cache hits/misses if present;
- shipments advanced;
- formations evaluated;
- provenance records emitted.

### Persistence
When supported:
- snapshot size;
- save time;
- load time;
- deterministic post-load continuation check.

## Determinism

Run identical generated workloads at least twice with the same seed/configuration.

The harness must verify matching deterministic summary hashes for authoritative outputs.

If multithreading is used, it may not alter authoritative results.

## Reporting

Emit a machine-readable result file plus a concise human report.

Suggested machine-readable format:

`world_scale_result.json`

Minimum fields:

- git commit;
- build type;
- compiler;
- machine information supplied by runtime where reasonably available;
- workload tier;
- activity profile;
- seed;
- object counts;
- timings;
- memory;
- work-amplification counters;
- deterministic result hash.

Do not embed user-identifying information.

## Acceptance criteria

006A2 passes as an **architecture measurement increment** when:

1. Tier 0–Tier 3 can be requested deterministically.
2. Real current-engine objects are used for implemented domains under test.
3. Memory and timing are measured rather than guessed.
4. Records-scanned vs records-changed counters expose empty work.
5. At least quiet, ordinary and crisis activity profiles run.
6. repeated identical runs produce the same authoritative summary hash.
7. no new gameplay mechanic is required merely to make the harness pass.
8. results identify the first current bottleneck(s).
9. full existing regression tests remain passing.
10. the architecture document records what the benchmark does **not** prove.

## Decision rules after measurement

### If Tier 3 memory is comfortably within consumer limits but CPU is high
Prioritize:
- event-driven scheduling;
- dirty sets;
- cached aggregates;
- routing caches;
- lower-frequency structural updates;
- SoA/compact hot-state layouts where profiling supports them.

### If memory is the dominant failure
Profile actual owners before reducing fidelity. Attack:
- strings/heap allocation;
- maps/unordered maps per record;
- duplicated identifiers/state;
- unbounded history;
- pointer-heavy sparse structures;
- cached data that can be recomputed cheaply.

### If both are acceptable
Proceed to `006A3 — Bronze Age / Modern Capability Composition Proof`.

### If Tier 3 cannot be represented without severe redesign
Stop feature development and redesign the failing substrate before proceeding.

## Historical performance budgets

Older WargameEngine documents proposed, for an ordinary modern 16 GiB gaming PC:

- normal retained engine memory target <= 4 GiB;
- safety ceiling 6 GiB;
- ordinary hourly p95 <= 100 ms;
- heavy hourly p99 <= 500 ms.

These are **historical provisional targets**, not automatic acceptance thresholds for the present C++ engine. 006A2 should produce evidence from which a current hardware contract can be adopted.

## Scope boundary

006A2 proves scale behavior of the state and mechanisms that currently exist.

It does **not** prove that absent finance, politics, intelligence, energy or health systems will be free.

Therefore the harness must remain extensible so each later MCV domain adds representative synthetic state/work and re-runs the same world-scale contract.
