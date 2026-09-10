# Wargame / OpenVic Engineering Instructions

## Mandatory first read

Before designing, modifying, or reviewing simulation mechanics, read:

`docs/architecture/REAL-WORLD-MODEL-REFERENCE-FRAMEWORK.md`

This is a mandatory project engineering rule.

Do not begin simulation implementation from game intuition alone.

## Required implementation method

Every new simulation mechanic must be evaluated against four sources of
evidence where relevant:

1. Existing authoritative OpenVic mechanics and repository architecture.
2. Real-world empirical, scientific, engineering, or policy methodology.
3. Relevant defense, strategic, causal, or operational simulation practice.
4. Relevant game abstractions where they help usability or appropriate
   simplification.

The implementation must be a reconciliation of these sources rather
than a copy of any one source.

## Authority rule

GitHub/repository reality is authoritative.

Inspect the current branch and existing native systems before proposing
new authoritative state.

Prefer extending existing native state and mechanisms over creating
parallel ledgers or duplicate systems.

## Real-world causality rule

Use domain-appropriate real mechanisms.

Examples:

- economics:
  inventories, markets, accounting identities, firms, input-output
  relationships, balance sheets;

- demographics:
  cohort-component methods, fertility schedules, mortality schedules,
  migration;

- disease:
  epidemiological models appropriate to the disease;

- logistics:
  network flow, capacity, inventories, transit time, queues;

- electricity:
  generation, transmission, capacity, fuel and network constraints;

- hydrology:
  precipitation, storage, runoff, groundwater, river flow and demand;

- agriculture:
  land, water, weather, soil, inputs, labor, crop response and logistics;

- politics:
  actors, institutions, preferences, leverage, legitimacy, state capacity
  and implementation;

- military:
  manpower, equipment, readiness, maintenance, training, doctrine,
  logistics, sensors, command and operational geography.

Do not replace domain mechanics with one universal propagation formula.

## Causal integration rule

New mechanics must identify:

- authoritative input state;
- transformation/mechanism;
- authoritative output state;
- downstream consumers;
- timing/lag;
- units or normalization;
- provenance/explainability;
- tests proving the causal seam.

Important results should eventually be able to answer:

"Why did this happen?"

## No universal causal graph

A causal/provenance graph may be derived for:

- explanation;
- debugging;
- sensitivity analysis;
- intervention analysis;
- player legibility.

It is not the authoritative physics of the simulation.

Native domain systems remain authoritative.

## No player privilege

Player-controlled and AI-controlled actors obey the same simulation
rules.

Only information presentation and input differ.

## Authoritative versus perceived state

Maintain separation between:

- true authoritative world state;
- actor-perceived state.

Perception may differ because of:

- intelligence coverage;
- reporting delays;
- uncertainty;
- deception;
- censorship;
- analytical capability;
- institutional data quality.

## Empirical versus calibrated values

Every coefficient must be identified as one of:

- empirically sourced;
- derived from authoritative data;
- scenario-defined;
- provisional simulation calibration.

Never present a provisional calibration constant as an empirical fact.

## Performance rule

High causal fidelity does not require microscopic simulation.

Prefer appropriate aggregation:

- POPs/cohorts rather than individual citizens;
- formations rather than bullets;
- infrastructure assets/corridors rather than every vehicle;
- crop regions rather than plants;
- systemic firms/institutions rather than every business.

Use:

- sparse updates;
- cadence scheduling;
- dirty recomputation;
- localized propagation;
- aggregation;
- deterministic replay;
- data-oriented storage where useful.

## Required research gate for convergence work

Every new `PROJECT-CONVERGENCE-*` architecture document implementing a
substantive simulation mechanic should contain these headings:

- `## Native Repository Basis`
- `## External Reference Review`
- `## Chosen Mechanism`
- `## Calibration Status`
- `## Causal Integration`
- `## Scope Boundary`

Older certified convergence documents are grandfathered and do not need
to be retroactively rewritten solely to satisfy this rule.

## Completion lifecycle

For engineering increments:

inspect authoritative repo
-> research real mechanism
-> compare relevant simulation/game references
-> define narrow design
-> implement
-> targeted tests
-> regression tests
-> full CTest
-> diff review
-> commit
-> push
-> verify exact remote HEAD

Do not leave partially integrated authoritative systems without an
explicit stopping boundary.
