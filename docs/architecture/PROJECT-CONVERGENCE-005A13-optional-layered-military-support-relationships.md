# PROJECT-CONVERGENCE-005A13 — Optional Layered Military Support Relationships

## Purpose

005A13 separates persistent military support relationships from current operational placement and hosting.

A formation may have:

- no support relationships;
- one support relationship;
- several simultaneous support relationships;
- several targets for the same support type.

Support is therefore a capability of the military system, not a mandatory property of every formation.

## Native Repository Basis

The inherited OpenVic military model ties much military state directly to army/navy runtime structures and geographic placement.

That is insufficient for representing forces whose sustained operation depends on infrastructure that is separate from their current location.

Examples include a formation that is:

- deployed away from its home station;
- embarked on another formation while retaining shore support;
- supported by several logistics nodes;
- operating with little or no permanent support infrastructure.

Existing `BuildingInstance` objects already provide stable identifiers for authoritative facilities.

005A13 therefore uses identifier references rather than introducing a second military-only base object.

## External Reference Review

The Real-World Model Reference Framework requires causal infrastructure effects to be represented through explicit relationships rather than broad country-quality modifiers.

Military support depth varies substantially between forces.

A formation in a highly developed military may have multiple redundant support relationships.

A formation in a low-infrastructure military may legitimately have none.

The engine must represent both without privileging a particular force structure.

## Chosen Mechanism

005A13 introduces:

`MilitarySupportTypeDefinition`

`MilitarySupportManager`

`MilitarySupportRelationship`

Each runtime relationship contains:

- a data-defined support type;
- a target identifier.

`MilitaryFormationInstance` owns zero or more support relationships.

## Typed Support

Support semantics are carried by data-defined identifiers.

Possible examples include:

- home station;
- maintenance;
- logistics support;
- forward support;
- medical support;
- depot relationship.

These are not hard-coded engine ontology.

## Optional Support

A military formation requires no support relationship to exist.

Zero support links is a valid authoritative state.

This permits settings or actors with weak, informal, austere, or nonexistent support infrastructure.

## Layered Support

Several support relationships may coexist on one formation.

For example:

formation

→ home station

→ forward support node

→ maintenance hub.

Adding support relationships does not alter operational placement.

## Redundant Support

The same support type may reference several target nodes.

For example:

logistics support → node A

logistics support → node B.

This permits redundancy without requiring a special major-power flag.

## Support and Hosting Are Independent

Current hosting and persistent support may coexist.

A formation may therefore be:

- currently hosted by a mobile platform;
- deriving operational position from that host;
- still linked to a permanent support site.

005A13 does not force one relationship to replace the other.

## Relationship Validation

The runtime rejects:

- empty target identifiers;
- unknown formation identities;
- exact duplicate `(support type, target)` relationships;
- removal of relationships that do not exist.

One relationship may be removed without disturbing others.

## Victoria-Specific Boundary

005A13 does not depend on:

- armies;
- navies;
- regiments;
- ships;
- `unit_branch_t`;
- province army/navy storage.

## Modern-Specific Boundary

005A13 does not hard-code:

- airbase;
- naval station;
- depot;
- FOB;
- maintenance center;
- logistics hub.

These are content meanings expressed through support-type identifiers and target identities.

## Causal Integration

005A13 establishes authoritative support relationships only.

It does not yet calculate effects from those relationships.

Later mechanics may use them to affect:

- readiness;
- maintenance;
- replacement flow;
- fuel and ammunition delivery;
- sortie generation;
- recovery;
- repair;
- sustainment resilience.

This preserves causal ownership for later domain-specific systems.

## Calibration Status

No empirical coefficients are introduced.

Support relationship types and target IDs in tests are proof fixtures.

## Scope Boundary

005A13 does not implement:

- supply flow;
- maintenance throughput;
- facility capacity;
- support radius;
- travel time;
- transport delay;
- stockpiles;
- repair rates;
- sortie generation;
- replacement rates;
- readiness effects;
- ownership or diplomatic access validation.

Those are later mechanics.

## Validation

Focused tests verify:

- a formation may have zero support relationships;
- multiple support types may coexist;
- support overlaps operational placement;
- one support type may reference several nodes;
- support links coexist with a current mobile host;
- exact duplicate support links are rejected;
- one relationship can be removed independently.

005A12 and 005A11 regression suites remain passing.

The broader military test selection remains passing.

## Result

005A13 establishes:

> Military support is an optional, typed, many-to-many relationship independent from current placement and hosting.

This allows military infrastructure depth to emerge from actual support relationships rather than a hard-coded assumption that every formation possesses a sophisticated permanent base structure.
