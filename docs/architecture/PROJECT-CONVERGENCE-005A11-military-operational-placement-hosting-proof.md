# PROJECT-CONVERGENCE-005A11 — Military Operational Placement and Hosting Proof

## Purpose

005A11 proves that military runtime placement does not need to be represented by one mandatory location field.

A military formation may instead be:

- directly positioned at a canonical location;
- hosted by another military formation and derive effective position from that host;
- detached and later directly positioned;
- valid with no current spatial placement.

This is necessary for carrier aviation, embarked troops, transport relationships, and nonspatial military organizations.

## Native Repository Basis

The inherited OpenVic military runtime stores armies and navies directly on provinces.

`ProvinceInstance` contains separate army and navy collections and exposes branch-specific military accessors.

This means inherited spatial placement is tightly coupled to the LAND/NAVAL hierarchy.

That approach is insufficient for military entities whose current position may depend on another platform rather than direct map occupancy.

Examples include:

- aircraft aboard carriers;
- troops aboard transports;
- helicopters aboard ships;
- drones launched from mobile platforms.

005A11 therefore keeps the generic military runtime independent from province-owned army/navy containers.

## External Reference Review

The Real-World Model Reference Framework requires authoritative state while preserving domain-appropriate mechanics.

Hosted forces should not duplicate the host's spatial state.

If a carrier moves, embarked aircraft should derive the new effective position rather than require a second synchronized location update.

The same principle applies to transported land forces.

Therefore operational placement is modeled as a relationship rather than one universal location property.

## Chosen Mechanism

`MilitaryFormationInstance` now supports two alternative forms of current operational placement:

1. direct position;
2. hosted position.

Direct position stores a canonical location identifier.

Hosted position stores the stable `unique_id_t` of another runtime military formation.

A formation may also have neither.

## Direct Position

A directly placed formation stores:

`direct_position_id`

This is an identifier reference only.

005A11 does not create a second geographic object or map ledger.

The identifier is intended to reference existing canonical spatial identities.

## Hosting

A hosted formation stores:

`hosted_by_unique_id`

The host relationship uses stable runtime identity rather than a raw pointer.

This avoids invalidation when the runtime formation container reallocates.

A hosted formation clears its direct position.

Its effective position is resolved through the host chain.

## Derived Effective Position

`get_effective_operational_position_id(...)`

walks the hosting relationship until it finds a directly positioned host.

Therefore:

carrier position changes

→ hosted air group's effective position changes automatically

without duplicating or synchronizing a second location value.

## Embarkation and Detachment

The proof demonstrates a land formation hosted by a naval transport.

While hosted, the land formation derives the transport's sea position.

After detachment it has no effective position until it receives a direct location.

The same formation can then be directly placed at a land location.

This establishes the transition:

direct / unplaced

→ hosted

→ detached

→ direct

without changing formation type or domain.

## Hosting Validation

005A11 rejects:

- self-hosting;
- hosting cycles;
- hosting relationships involving unknown runtime identities.

Cycle prevention is required because effective-position resolution follows the hosting chain.

## Optional Placement

A runtime military formation remains valid with:

- no direct position;
- no host.

This is required for nonspatial organizations and for transitional states where location is not yet assigned.

Capability does not imply mandatory spatial resolution.

## Victoria-Specific Boundary

005A11 does not require:

- province army storage;
- province navy storage;
- `unit_branch_t`;
- army pathing;
- navy pathing;
- regiment transport flags.

The inherited systems remain unchanged for compatibility.

## Modern-Specific Boundary

005A11 does not hard-code:

- aircraft carrier;
- amphibious assault ship;
- helicopter;
- marine;
- transport ship;
- orbital carrier;
- drone mothership.

The test names demonstrate intended use cases only.

The mechanism is generic hosting plus optional direct placement.

## Important Distinction: Hosting Is Not Basing

005A11 models current operational placement.

It does not yet model persistent home/support relationships.

For example, a carrier air wing may later have:

- current host = carrier;
- home/support base = shore installation.

Those are independent relationships and should not be conflated.

A later increment may introduce support/basing contracts separately.

## Causal Integration

005A11 adds authoritative placement relationships but no movement scheduler.

Changing a host's direct position immediately changes the effective position returned for hosted formations.

No duplicate spatial state is mutated on the guest.

This establishes a narrow causal relationship:

host placement

→ guest effective placement

## Validation

Focused tests verify:

- generic direct placement;
- hosted air formation derives carrier position;
- hosted position changes automatically when the host moves;
- embarked land formation derives transport position;
- detachment removes inherited position;
- detached formation can receive a direct position;
- self-hosting is rejected;
- hosting cycles are rejected;
- unplaced formation remains valid.

005A10 regression tests remain passing.

The broader military test selection remains passing.

## Scope Boundary

005A11 does not implement:

- movement speed;
- paths;
- range;
- mission radius;
- basing capacity;
- deck capacity;
- transport capacity;
- embarkation time;
- disembarkation time;
- amphibious penalties;
- carrier launch/recovery;
- air missions;
- orbital mechanics;
- logistics;
- personnel;
- equipment;
- combat.

Those mechanics require later capability and domain-specific systems.

## Calibration Status

No empirical coefficients are introduced.

Location identifiers used in tests are proof fixtures.

## Result

005A11 establishes:

> Current military operational placement may be direct, inherited through a host, or absent entirely.

This supports carrier aviation and transported troops without forcing every military entity into direct province occupancy or duplicating host location state.

The generic runtime chain is now:

`MilitaryDomainDefinition`

→ `MilitaryCapabilityDefinition`

→ `MilitaryFormationDefinition`

→ `MilitaryFormationInstance`

→ optional operational placement / hosting relationship.
