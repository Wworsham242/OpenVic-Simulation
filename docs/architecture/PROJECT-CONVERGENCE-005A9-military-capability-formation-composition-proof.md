# PROJECT-CONVERGENCE-005A9 — Military Capability / Formation Composition Proof

## Purpose

005A9 proves that a military formation can be represented through generic domain identity and generic capability composition without inheriting from the legacy LAND/NAVAL unit hierarchy.

The proof builds directly on 005A8.

005A8 established that military-domain identity can exist outside the closed `unit_branch_t` enum.

005A9 establishes that a formation definition can reference one of those domains and an arbitrary set of capability identities without becoming a `RegimentType`, `ShipType`, `ArmyInstance`, or `NavyInstance`.

## Native Repository Basis

The inherited OpenVic military model still centers on:

- `UnitType`;
- `RegimentType`;
- `ShipType`;
- `unit_branch_t`;
- `ArmyInstance`;
- `NavyInstance`.

`UnitType` itself contains a mandatory `unit_branch_t`.

Its LAND and NAVAL specializations contain substantially different domain-specific properties.

This confirms that adding AIR or SPACE to `unit_branch_t` would preserve the same closed inheritance model rather than generalize it.

005A9 therefore introduces a parallel generic formation-definition seam.

## External Reference Review

The Real-World Model Reference Framework requires domain-appropriate mechanisms rather than one universal abstraction.

Military forces across different settings may differ in:

- movement;
- basing;
- personnel;
- sustainment;
- command;
- sensing;
- strike;
- occupation;
- range;
- readiness;
- mission generation.

Therefore formation identity should not itself dictate all operational mechanics.

A formation should instead be able to compose:

- domain identity;
- capability identities;
- later optional operational models.

## Chosen Mechanism

005A9 introduces:

`MilitaryCapabilityDefinition`

`MilitaryFormationDefinition`

`MilitaryFormationManager`

A capability definition owns only a stable identifier.

A formation definition owns:

- stable identifier;
- one `MilitaryDomainDefinition`;
- zero or more `MilitaryCapabilityDefinition` references.

This is definition composition only.

## Capability Identity

Capabilities are generic named definitions.

The proof uses examples such as:

- `mobility`;
- `strike`;
- `sensing`.

These are proof identifiers, not hard-coded engine semantics.

The engine does not yet define what those capabilities do.

Future domain mechanics may attach behavior to capability identity through explicit native systems.

## Formation Identity

A generic formation does not contain:

- `unit_branch_t`;
- regiment type;
- ship type;
- army type;
- navy type;
- general/admiral classification.

The proof demonstrates a valid formation in a completely nonlegacy domain.

Therefore the formation layer is not constrained by the inherited LAND/NAVAL branch enum.

## Cross-Domain Capability Composition

A capability is not owned by one domain.

The same capability identity may be referenced by formations in different domains.

This supports cases such as:

- sensing from ground radar;
- sensing from aircraft;
- sensing from naval systems;
- sensing from orbital systems.

The capability identity does not determine the implementation.

## Validation

The manager validates:

- non-empty capability identifiers;
- non-empty formation identifiers;
- non-null capability references;
- no duplicate capability reference within one formation;
- normal registry uniqueness for capability and formation identifiers.

The proof includes rejection of a formation containing the same capability twice.

## MilitaryManager Integration

`MilitaryFormationManager` is owned by `MilitaryManager` beside:

- `MilitaryDomainManager`;
- `UnitTypeManager`;
- the legacy military managers.

This preserves compatibility while establishing a new general composition path.

## Victoria-Specific Boundary

005A9 does not require:

- regiment;
- ship;
- army;
- navy;
- general;
- admiral;
- soldier POP;
- port;
- land province;
- sea province.

Those concepts remain in the compatibility implementation only.

## Modern-Specific Boundary

005A9 does not hard-code:

- fighter;
- bomber;
- satellite;
- missile;
- drone;
- cyber unit;
- airbase;
- orbital regime.

The generic capability examples are setting-neutral proof fixtures.

## Formation Is Not Yet a Runtime Unit

`MilitaryFormationDefinition` is a definition object.

It is not yet:

- a deployed runtime formation;
- an authoritative military state holder;
- a movement participant;
- a combat participant;
- a basing participant;
- a personnel ledger;
- an inventory owner.

Those are later increments.

This distinction avoids prematurely forcing all military domains into one operational model.

## Causal Integration

005A9 introduces no new combat or mission causality.

It introduces a compositional definition seam that later native military systems can consume.

The intended future pattern is:

domain identity

+ capability identities

+ domain-appropriate runtime mechanics

→ authoritative military behavior.

## Calibration Status

No empirical coefficients or scenario parameters are introduced.

All capabilities used in tests are proof fixtures.

## Scope Boundary

005A9 does not:

- replace `UnitType`;
- replace `RegimentType`;
- replace `ShipType`;
- modify `unit_branch_t`;
- implement air movement;
- implement air combat;
- implement orbital movement;
- implement missile forces;
- implement cyber operations;
- implement basing;
- implement manpower;
- implement supply;
- implement readiness;
- implement missions;
- implement runtime military instances.

## Validation Performed

Focused tests verify:

- air-domain formation composition;
- space-domain formation composition;
- multiple capabilities on one formation;
- capability reuse across domains;
- capability presence checks;
- duplicate capability rejection;
- a nonlegacy formation with no `unit_branch_t`.

The broader `[military]` test selection also passes.

Full CTest is required before closure.

## Result

005A9 establishes:

> Military formation identity can be composed from generic domain and capability definitions without inheriting the legacy LAND/NAVAL unit hierarchy.

This is the second structural step required to support air, space, and other military systems as first-class engine concepts rather than enum extensions.
