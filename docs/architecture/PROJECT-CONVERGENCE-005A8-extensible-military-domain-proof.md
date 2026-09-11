# PROJECT-CONVERGENCE-005A8 — Extensible Military Domain Proof

## Purpose

005A8 proves that the universe of military domains no longer needs to be exhaustively represented by the inherited compile-time `unit_branch_t` enumeration.

The existing OpenVic military architecture remains LAND/NAVAL-oriented and is preserved for compatibility.

A new generic military-domain identity seam is introduced beside that legacy architecture.

The proof demonstrates that additional domains such as `air` and `space` can exist natively without adding `AIR` or `SPACE` to `unit_branch_t`.

## Native Repository Basis

Before 005A8, the military model was structurally closed around:

- `unit_branch_t::LAND`;
- `unit_branch_t::NAVAL`;
- `RegimentType`;
- `ShipType`;
- `ArmyInstance`;
- `NavyInstance`;
- army/navy deployment parsing;
- province army/navy collections;
- land/sea pathfinding;
- general/admiral leader semantics;
- army/navy spending;
- regiment-oriented recruitment;
- land/naval modifier caches.

The inherited enum was:

`INVALID_BRANCH, LAND, NAVAL`

This was not merely terminology.

LAND/NAVAL assumptions are embedded through multiple runtime and definition layers.

## External Reference Review

The Real-World Model Reference Framework requires the engine to preserve domain-appropriate native mechanisms while avoiding unnecessary ontology in general core.

A general strategic engine must not assume that all military power fits a fixed army/navy dichotomy.

Modern military and strategic systems may require domains or capability families including:

- land;
- maritime;
- air;
- space;
- cyber;
- strategic missile forces;
- unmanned systems;
- intelligence and surveillance assets.

These do not necessarily share one movement, basing, personnel, location, or formation model.

Therefore the correct first generalisation is not to add more values to `unit_branch_t`.

The correct first step is to separate generic military-domain identity from the inherited branch implementation.

## Chosen Mechanism

005A8 introduces:

`MilitaryDomainDefinition`

and:

`MilitaryDomainManager`

`MilitaryDomainDefinition` owns only:

- stable identifier;
- optional compatibility marker for legacy LAND;
- optional compatibility marker for legacy NAVAL.

It does not define:

- movement;
- combat;
- recruitment;
- basing;
- leadership;
- province occupancy;
- formation structure;
- pathfinding;
- logistics;
- mission behavior.

This keeps domain identity separate from domain mechanics.

## Registry

`MilitaryDomainManager` uses the existing native identifier-registry machinery.

The proof registers:

- `land`;
- `naval`;
- `air`;
- `space`.

No `unit_branch_t::AIR` or `unit_branch_t::SPACE` values are introduced.

Duplicate identifiers are rejected through normal registry behavior.

## Legacy Compatibility

005A8 provides a narrow compatibility mapping:

`unit_branch_t::LAND` → generic legacy-land-compatible domain

`unit_branch_t::NAVAL` → generic legacy-naval-compatible domain

Only one registered domain may own each legacy compatibility role.

A domain cannot simultaneously claim both LAND and NAVAL compatibility.

This prevents ambiguous bridging while the old military stack remains active.

## MilitaryManager Integration

`MilitaryDomainManager` is owned by `MilitaryManager` as a peer to the existing military managers.

It does not replace `UnitTypeManager`, `DeploymentManager`, or existing army/navy state.

This establishes the future migration seam without forcing a military rewrite.

## Victoria-Specific Boundary

The following inherited concepts remain deliberately untouched in 005A8:

- regiment;
- ship;
- army;
- navy;
- general;
- admiral;
- land recruitment;
- naval deployment;
- province army/navy storage;
- land/sea pathfinding.

005A8 does not claim these concepts are general.

It establishes a boundary from which they can later be migrated or retained as compatibility implementations.

## Modern-Specific Boundary

005A8 does not hard-code:

- AIR;
- SPACE;
- CYBER;
- MISSILE;
- DRONE;
- orbital force;
- air force.

`air` and `space` are test/example identifiers only.

A different setting may define different domain identities.

Thus the core mechanism does not assume a modern military taxonomy.

## Important Distinction: Domain Is Not Unit Type

`MilitaryDomainDefinition` is not:

- a unit;
- a formation;
- a branch implementation;
- a movement model;
- a basing model;
- a personnel model;
- a command model.

Future military entities may compose domain identity with other capabilities rather than inherit all behavior from one branch enum.

This is necessary because land, naval, air, space, cyber, and strategic assets may require fundamentally different operational models.

## Data/Definition Boundary

005A8 proves an extensible native definition registry.

It does **not yet prove external file-driven loading** of military domains.

The current proof registers domains through the manager API.

Future work may expose domain definitions to data loading once the desired schema is known.

This distinction is intentional: the increment proves the engine boundary before adding a content format.

## Validation

The focused 005A8 tests verify:

- legacy `land` and `naval` domains can be registered;
- `air` and `space` can register without enum extension;
- all four retain stable identity;
- nonlegacy domains do not masquerade as LAND/NAVAL;
- legacy LAND/NAVAL map onto generic domains;
- duplicate domain identity is rejected;
- only one domain can own LAND compatibility;
- only one domain can own NAVAL compatibility;
- one domain cannot claim both legacy branches.

## Scope Boundary

005A8 does not:

- rewrite `unit_branch_t`;
- add AIR or SPACE enum values;
- rewrite `UnitType`;
- rewrite `UnitInstance`;
- rewrite `UnitInstanceGroup`;
- change army/navy runtime state;
- change deployment/OOB parsing;
- change recruitment;
- change leaders;
- change pathfinding;
- change combat;
- change military spending;
- change military modifiers;
- implement air warfare;
- implement orbital mechanics;
- implement cyber warfare;
- implement missile forces.

Those are later domain-mechanics problems.

## Causal Integration

005A8 introduces no new causal military behavior.

It creates a definition seam that future causal military systems can reference without requiring the domain universe to be compile-time closed.

## Calibration Status

No empirical or scenario coefficients are introduced.

There is no behavioral calibration in this increment.

## Result

005A8 establishes:

> Military domain identity is no longer conceptually limited to the inherited LAND/NAVAL enum.

The old army/navy architecture remains available as a compatibility implementation.

New military domains can now be represented without changing that enum.

This is the first bounded step toward an engine in which military power is composed from domain-appropriate capabilities rather than forced into an exhaustive army-or-navy hierarchy.
