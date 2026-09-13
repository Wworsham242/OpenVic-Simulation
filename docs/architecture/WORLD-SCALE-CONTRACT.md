# WORLD-SCALE-CONTRACT

**Status:** Provisional performance constitution pending 006A2 measurement  
**Applies to:** current C++ / OpenVic-derived world simulation engine  
**Supersedes:** implicit reuse of older WargameEngine performance targets

## Purpose

The engine must simulate a strategically useful world on a high-end consumer gaming PC. Performance and memory are architectural requirements, not end-stage optimization tasks.

This contract separates:

1. **scale targets** — what a representative world must be capable of containing;
2. **workload rules** — how often different state is allowed to require expensive work;
3. **measurement requirements** — what must be instrumented;
4. **provisional budgets** — inherited targets that remain hypotheses until 006A2 establishes evidence;
5. **failure rules** — when feature development stops for substrate redesign.

## Representative world target

The current synthetic world target is:

- approximately 200 sovereign-scale actors/equivalents;
- 100,000 authoritative geographic/network points;
- up to 1,000,000 aggregate socioeconomic/population records;
- up to 50,000 maneuver formations;
- persistent physical inventories, flows and shipments;
- multi-domain causal provenance at bounded depth;
- enough scheduled political/economic/environmental/military activity to represent both ordinary and crisis periods.

These are simultaneous targets, not separate peak demonstrations.

They are workload anchors, not permanent hard-coded engine maxima.

## Resolution rule

The engine may use different representations at different causal scales, but:

- camera position, UI zoom and player observation must never change authoritative outcomes;
- detail may increase because causal relevance, activity or systemic importance increases;
- inactive detail should remain aggregated when finer resolution would not change strategically relevant outcomes;
- all resolution changes must preserve conservation and deterministic continuation.

## Cadence rule

No subsystem may default to the fastest global cadence simply because such a tick exists.

At minimum the architecture must support:

- operational/high-frequency work;
- daily work;
- weekly/planning work;
- monthly/structural work;
- event-triggered work.

A record should be touched because its owning mechanism requires work, not because every record is scanned every hour.

## Work-amplification rule

Every major domain must be capable of reporting:

- records present;
- records scanned;
- records changed;
- dirty records;
- scheduled events processed;
- cache hits/misses where material;
- domain-specific work units.

A design that repeatedly scans 1,000,000 records to update a small active subset is a performance defect until measurement proves the scan cheaper than maintaining an index/dirty set.

## Memory rule

Raw state count is not the primary concern. Per-record heap objects, strings, maps, pointer-heavy ownership, duplicate ledgers and unbounded histories are.

Memory reviews therefore examine:

- bytes of authoritative state;
- allocation count and fragmentation;
- index/cache bytes;
- history/provenance bytes;
- duplicated identifiers/strings;
- temporary peak memory.

## Historical provisional budgets

The older Rust WargameEngine proposed:

- normal retained engine memory: <= 4 GiB;
- safety ceiling: <= 6 GiB;
- ordinary hourly p95: <= 100 ms;
- heavy hourly p99: <= 500 ms;
- 10-year synthetic campaign: <= 12 wall-clock hours;
- routine actor decision: <= 10 ms plus bounded search;
- operational plan revision: <= 250 ms;
- strategic review: <= 2 seconds;
- full save: <= 2 GiB;
- save/load: <= 60 seconds.

For the current C++ engine these are **reference hypotheses only**.

006A2 must measure the present architecture before any value becomes a binding acceptance threshold.

## 006A2 workload ladder

Tier 0:
- 10 actors
- 1,000 geography/network points
- 10,000 socioeconomic aggregates
- 500 formations

Tier 1:
- 40 actors
- 10,000 geography/network points
- 100,000 socioeconomic aggregates
- 5,000 formations

Tier 2:
- 100 actors
- 50,000 geography/network points
- 500,000 socioeconomic aggregates
- 25,000 formations

Tier 3:
- 200 actors
- 100,000 geography/network points
- 1,000,000 socioeconomic aggregates
- 50,000 formations

## Activity profiles

Initial synthetic controls:

| Profile | formations active | network dirty | socioeconomic dirty |
|---|---:|---:|---:|
| quiet | 1% | 2% | 1% |
| ordinary | 10% | 5% | 5% |
| crisis | 35% | 20% | 15% |

These are benchmark controls, not empirical world constants.

## Required measurements

Each run must record, where supported:

- build/commit/configuration;
- seed;
- object counts;
- wall time;
- phase times;
- p50/p95/p99 scheduled-work duration;
- working-set/retained memory;
- temporary peak memory;
- records scanned/changed;
- dirty-set size;
- events processed;
- route searches and cache statistics;
- shipments advanced;
- formations evaluated;
- provenance records emitted;
- snapshot/save bytes and times;
- deterministic state hash.

## Determinism

Two runs with identical authoritative inputs and seed must produce identical authoritative summary hashes.

Thread count may change execution order but may not change authoritative results.

## Failure conditions

Pause feature development and fix the substrate if:

- Tier 3 cannot be represented within a plausible consumer-memory envelope;
- ordinary workloads require broad high-frequency full-world scans;
- performance depends on camera/UI visibility;
- histories or provenance grow without bounds;
- deterministic replay diverges under identical inputs;
- a new domain creates a second authoritative ledger to gain speed;
- a performance optimization changes authoritative semantics without an explicit approximation contract.

## Updating this contract

006A2 establishes the first evidence-based current-C++ budgets.

Every later MCV domain must add representative state/work to the same harness and re-run the contract. A domain is not MCV-certified merely because its isolated tests are fast.
