# PROJECT-CONVERGENCE-005A4 — Geographic Grouping Boundary Proof

## Scope

005A4 proves that geographic membership can be represented independently of OpenVic's fixed Region and State semantics.

The increment does not replace OpenVic geography.

It introduces a narrow generic representation for named spatial group membership and proves that the existing OpenVic `Region` can be adapted into that representation without changing authoritative legacy map structures.

## Native Repository Basis

The existing repository contains several distinct geographic concepts.

`ProvinceDefinition` is the map-definition location unit and contains:

- immutable province identity;
- adjacency;
- map position;
- terrain;
- climate;
- continent;
- a direct `Region const*`.

`Region` is substantially a named `ProvinceSet`.

`ProvinceInstance` contains runtime province state and a direct `State*`.

`State` is not merely a geographic collection. It additionally contains:

- runtime province membership;
- capital;
- POP aggregation;
- colony status;
- industrial power;
- ownership-derived behavior.

`StateManager::generate_states()` builds runtime states from province gamestate and legacy regions.

These structures remain authoritative for existing OpenVic behavior.

## External Reference Review

No new geographic science, demographic model, political boundary model, hydrological model, or military model is introduced.

005A4 is an architectural composition proof.

The real-world requirement motivating the boundary is that one physical location may participate simultaneously in multiple spatial classifications, for example:

- administrative area;
- watershed;
- electricity service region;
- logistics zone;
- market zone;
- ecological region;
- military theater;
- hazard zone.

These classifications need not form a single hierarchy and may overlap.

No empirical coefficients or calibrated equations are required for this increment.

## Chosen Mechanism

005A4 introduces `SpatialGrouping`.

A `SpatialGrouping` contains:

- `grouping_id`;
- `grouping_kind`;
- a canonical sorted unique set of `member_location_ids`.

The generic representation deliberately does not define:

- province;
- state;
- country;
- county;
- watershed;
- theater;
- market area;
- electricity grid;
- administrative hierarchy.

Those concepts are supplied by data or domain adapters.

Independent `SpatialGrouping` values naturally permit the same location identifier to belong to multiple groups.

## Determinism

`SpatialGrouping::canonicalize()`:

1. sorts member location identities;
2. removes duplicates;
3. stores the resulting deterministic sequence.

`is_canonical()` verifies:

- non-empty grouping identity;
- non-empty grouping kind;
- non-empty member identities;
- sorted membership;
- unique membership.

`contains()` performs lookup against the canonical sorted representation without allocating a temporary member string.

## Legacy Region Adapter

005A4 introduces `LegacyRegionGroupingAdapter`.

It converts the existing OpenVic:

`Region`

→ `ProvinceDefinition` membership

→ generic `SpatialGrouping`.

The adapter emits qualified legacy identities:

- grouping: `legacy.region:<identifier>`;
- grouping kind: `legacy.region`;
- member locations: `province:<identifier>`.

These prefixes belong to the compatibility adapter.

They are not universal engine geography ontology.

## Calibration Status

Not applicable.

No coefficients, rates, probabilities, distances, capacities, or empirical thresholds are introduced.

## Causal Integration

### Inputs

Generic grouping consumes:

- grouping identity;
- grouping kind;
- member location identities.

Legacy adaptation consumes:

- authoritative `Region`;
- its existing `ProvinceDefinition` membership.

### Mechanism

Generic group membership is canonicalized independently for each group.

There is no requirement that:

- a location belong to only one group;
- two groups of different kinds be mutually exclusive;
- groups form parent-child relationships;
- a group correspond to political ownership;
- a group correspond to a Victoria State.

### Outputs

The result is a deterministic read-only grouping value suitable for later domain composition and lookup.

### Downstream

005A4 does not yet redirect existing systems to consume `SpatialGrouping`.

Future systems may use the generic boundary where appropriate, while legacy Region/State behavior remains intact until explicitly converged.

### Timing

Definition/composition-time representation only.

No new simulation tick or cadence is introduced.

### Units

String identities and collection membership only.

### Provenance

The legacy adapter's output is directly derived from existing authoritative `Region` province membership.

No second authoritative province or state ledger is created.

### Tests

005A4 proves:

- malformed generic grouping values are rejected as non-canonical;
- canonicalization is deterministic;
- duplicate members collapse deterministically;
- membership lookup is correct;
- two independent grouping kinds may overlap on the same location;
- existing OpenVic Region membership can be adapted to generic grouping;
- 005A3 target-aware authority behavior remains intact;
- full repository regression remains intact.

## Generality Result

The general engine can now express:

location

→ zero or more independently defined spatial groups.

This removes the architectural assumption that all useful geography must be expressed through a single mandatory:

province

→ state

→ country

hierarchy.

The generic mechanism can support many game/scenario definitions without hardcoding their semantics.

## Victoria-Specific Boundary

The following remain legacy/OpenVic concepts:

- `ProvinceDefinition::region`;
- `ProvinceInstance::state`;
- `Region`;
- `State`;
- `StateSet`;
- `StateManager`;
- colonial-state semantics;
- State POP aggregation;
- State industrial power;
- State capital logic.

005A4 does not redefine or remove them.

`State` is specifically not promoted into the generic geography layer because it contains substantial Victoria-specific runtime semantics beyond geographic membership.

## Modern-Specific Boundary

005A4 also does not hardcode modern geographic concepts such as:

- counties;
- electrical grids;
- FEMA regions;
- NATO commands;
- congressional districts;
- ISO administrative levels;
- modern national borders.

These may later be represented as data-defined grouping kinds rather than core classes.

## Scope Boundary

005A4 does not:

- replace ProvinceDefinition;
- replace Region;
- replace State;
- alter StateManager generation;
- change province adjacency;
- change raster/map rendering;
- introduce geographic hierarchy rules;
- introduce ownership;
- introduce administrative authority;
- introduce logistics routing;
- introduce climate mechanics;
- introduce hydrology;
- begin population capability convergence.

## Validation

Validation is performed on:

- branch `work/live-economy-005-modern-catalog`;
- starting HEAD `beba3cf1603369ba064c32d4c008cc049d1f1b31`.

Required validation includes:

- Debug build;
- targeted `[convergence][geography][grouping]` tests;
- prior `[convergence][authority][target]` regression;
- full CTest;
- `git diff --check`;
- exact staged-file verification;
- local/remote HEAD verification after push.

The intended result is a general spatial-membership seam layered alongside existing OpenVic geographic authority rather than a replacement geography system.
