# RECONCILIATION-001 â€” Foundation freeze and vertical-entry rules

Status: foundation surgery is frozen after FOUNDATION-010, subject only to demonstrated integration defects.

## 1. What is now real

The production `InstanceManager` owns:

- generalized simulation timeline;
- authority registry;
- ordered accepted-command runtime;
- command-admission runtime.

One real gameplay action, mobilization, can already travel through:

```text
actor
 -> authority
 -> jurisdiction
 -> accepted durable command
 -> legacy compatibility adapter
 -> existing GameAction queue
 -> existing gameplay mutation
```

This proves that legacy OpenVic mechanics can be migrated incrementally rather than rewritten wholesale.

## 2. Persistence reconciliation

`LiveCommandTimelineSnapshot` is the production continuity package for the generalized owners
currently wired into `InstanceManager`:

```text
SimulationTimeline
OrderedCommandRuntime
```

Capture and restore are exact and restore is transactional across both owners.

This is deliberately **not** named or treated as a complete campaign save.

`CampaignStateSnapshot` remains the broader eventual campaign composition that also has homes for:

- named RNG streams;
- ECS identity;
- later domain-store state.

Do not claim a complete deterministic campaign save until every authoritative owner is composed.

## 3. Actor and jurisdiction bootstrap ownership

Freeze this rule:

> Stable actor, office, institution, command, corporation, and jurisdiction definitions belong to
> moddable ruleset/bootstrap data, not hard-coded engine enums and not numeric legacy country indices.

Runtime flow:

```text
mod/ruleset definitions
      |
      v
stable actor + jurisdiction identities
      |
      v
session bootstrap
      |
      v
AuthorityRegistry
```

Legacy `country_index_t` may be carried by compatibility payloads during migration, but it does not
become the generalized geopolitical identity system.

## 4. Truth and perceived state seam

Freeze this rule:

```text
AUTHORITATIVE WORLD TRUTH
        |
        v
observation / sensors / institutions / reports
        |
        v
ACTOR KNOWLEDGE STATE
        |
        +--> AI decisions
        |
        `--> player presentation
```

There is one authoritative world, not one cloned world per actor.

Actor knowledge stores observations, beliefs, reports, estimates, timestamps, reliability, confidence,
and provenance only where strategically useful.

No UI or AI subsystem should gain unrestricted authoritative truth merely because it is convenient.

Implementation is deferred until the first vertical requires it. Do not build a universal information
simulator in advance.

## 5. Transaction boundary rule

Transactions are required when one logical action must preserve invariants across multiple authoritative owners.

Examples likely to require transactions:

- money leaves treasury while inventory ownership changes;
- securities trade changes cash and asset positions;
- mobilization reserves personnel while formations/readiness change;
- logistics transfer consumes source inventory and creates destination/in-transit inventory;
- save/restore swaps multiple authoritative runtime owners.

Transactions are **not** required for every field mutation inside one owner.

Rule:

> One owner: use its sanctioned mutation API.
> Multiple owners with one invariant: use a transaction/commit boundary.

## 6. Vertical-entry classification

Every inherited OpenVic system is now reviewed under:

- KEEP â€” already suitable mechanism/structure;
- GENERALIZE â€” useful skeleton but Victoria assumptions must be lifted;
- REWRITE â€” strategically necessary domain but causal implementation is insufficient;
- REMOVE â€” not needed at target resolution or conflicts with architecture.

Initial vertical order:

1. nation/actor/state-capacity skeleton;
2. economy/production/market skeleton;
3. population/demographic substrate;
4. politics/institutions;
5. military force/readiness/mobilization;
6. logistics/infrastructure;
7. finance/monetary;
8. information/intelligence;
9. energy/resources;
10. operational warfare and later specialist domains.

This ordering may change when dependency analysis says otherwise.

## 7. Existing OpenVic vertical tests

The inherited broad test groups are now migration guardrails:

```text
A_001_file_tests.cpp
A_002_economy_tests.cpp
A_003_military_unit_tests.cpp
A_004_networking_tests.cpp
A_005_nation_tests.cpp
A_006_politics_tests.cpp
```

Do not delete them merely because their Victoria-era expectations eventually change.

During vertical modernization:

1. identify what invariant the test represents;
2. preserve useful generic invariants;
3. rewrite obsolete Victoria-specific expectations;
4. add modern generalized tests beside them;
5. remove only tests for behavior explicitly retired.

## 8. Foundation freeze

Do not create FOUNDATION-011, FOUNDATION-012, etc. merely because another abstraction could be useful.

New engine primitives must now be justified by a vertical implementation that actually needs them.

Next work is a vertical audit and modernization slice.