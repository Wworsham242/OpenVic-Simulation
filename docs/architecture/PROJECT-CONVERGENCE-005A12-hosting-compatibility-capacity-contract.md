# PROJECT-CONVERGENCE-005A12 — Hosting Compatibility and Capacity Contract

## Purpose

005A12 constrains the generic hosting relationship introduced in 005A11.

A military formation may now declare:

- hosting profiles it provides;
- hosting profiles it requires;
- capacity provided per profile;
- demand consumed per profile.

Runtime hosting validates compatibility and remaining capacity before creating the relationship.

## Native Repository Basis

005A11 established generic runtime hosting by stable formation identity.

That proof deliberately allowed unrestricted hosting so that placement and host-derived position could be established independently.

However, unrestricted hosting cannot represent realistic distinctions such as:

- aviation-compatible hosting;
- troop transport;
- specialized vehicle transport;
- launch/support infrastructure;
- domain-specific support constraints.

The inherited OpenVic model represents transport largely through LAND/NAVAL-specific assumptions.

005A12 instead adds a generic typed contract.

## External Reference Review

The Real-World Model Reference Framework requires authoritative capacities and constraints to be expressed through native mechanisms where they materially affect outcomes.

Hosting is therefore modeled as a constrained relationship:

host provision

+ guest requirement

+ available capacity

→ admission or rejection.

No semantic category such as carrier, aircraft, marine, helicopter, or transport ship is required by the core mechanism.

## Chosen Mechanism

005A12 introduces:

`MilitaryHostingProfileDefinition`

`MilitaryHostingProvisionSpec`

`MilitaryHostingRequirementSpec`

`MilitaryHostingProvision`

`MilitaryHostingRequirement`

Formation definitions may contain zero or more provisions and requirements.

Each provision contains:

- hosting profile;
- capacity.

Each requirement contains:

- hosting profile;
- demand.

## Generic Profile Identity

Hosting-profile identifiers carry semantic meaning through data.

Examples used in tests include:

- `aviation_support`;
- `transport_capacity`;
- `personnel`.

These are proof fixtures and are not hard-coded engine semantics.

Other settings may define entirely different profiles.

## Optional Contract

Hosting contracts are optional.

A guest with no declared hosting requirements retains the generic 005A11 hosting behavior.

This preserves:

- simple settings;
- legacy compatibility;
- incomplete content during migration;
- nonrestrictive relationships where typed capacity is not required.

Once a guest declares one or more hosting requirements, every declared requirement becomes authoritative.

## Compatibility

For every guest requirement, the proposed host must provide the same hosting-profile identity.

If a required profile is absent, hosting is rejected before placement state changes.

## Capacity

Each provision defines a finite profile-local capacity.

Each hosted guest consumes its declared demand for the matching profile.

Runtime admission sums demand already assigned to the host.

A new guest is accepted only when:

existing demand

+ new guest demand

<= host capacity.

## Capacity Release

Capacity is not stored in a second ledger.

Current usage is derived from authoritative host relationships and guest formation definitions.

When a guest detaches, its demand automatically ceases to count toward the host's used capacity.

This avoids capacity-ledger synchronization problems.

## Multiple Independent Profiles

A host may provide multiple profiles simultaneously.

Different hosted formations may consume different profiles independently.

This supports compositions such as one platform providing both:

- aviation support;
- personnel transport;

without hard-coding a platform class.

## Definition Validation

005A12 rejects:

- empty hosting-profile identifiers;
- null provision profiles;
- null requirement profiles;
- non-positive provision capacities;
- non-positive requirement demands;
- duplicate provision profiles within one formation;
- duplicate requirement profiles within one formation.

## Victoria-Specific Boundary

005A12 does not require:

- regiment transport rules;
- naval transport slots;
- `unit_branch_t`;
- army/navy-specific hosting code.

The generic mechanism remains independent from the inherited LAND/NAVAL hierarchy.

## Modern-Specific Boundary

005A12 does not hard-code:

- carrier;
- flight deck;
- hangar;
- helicopter;
- marine;
- amphibious ship;
- transport aircraft;
- missile launcher.

Those meanings can be represented by data-defined hosting profiles.

## Causal Integration

005A12 extends the authoritative hosting mutation path.

`host_formation(...)` now validates:

1. runtime identity;
2. self-hosting;
3. cycle safety;
4. declared hosting-profile compatibility;
5. remaining capacity.

Only after all checks pass does the authoritative host relationship change.

## Calibration Status

No empirical coefficients are introduced.

Capacity and demand values used in tests are deterministic proof values.

Future content may calibrate profile units to setting-specific physical quantities.

## Scope Boundary

005A12 does not implement:

- actual aircraft counts;
- deck cycles;
- sortie generation;
- passenger counts;
- vehicle deck area;
- tonnage;
- embarkation duration;
- disembarkation duration;
- launch/recovery rules;
- basing;
- maintenance;
- logistics;
- combat.

Those mechanisms can later consume the generic hosting contract.

## Validation Performed

Focused tests verify:

- matching profile admits a guest;
- mismatched profile rejects a guest;
- finite capacity is shared across hosted guests;
- capacity becomes available again after detachment;
- one host may provide multiple independent profiles;
- invalid capacity definitions are rejected;
- invalid demand definitions are rejected.

005A11 regression remains passing.

The broader military selection remains passing.

Full CTest is required before closure.

## Result

005A12 establishes:

> Military hosting may be constrained by data-defined compatibility profiles and finite shared capacity without hard-coding military platform categories.

The generic path is now:

formation definition

→ hosting provisions / requirements

→ runtime hosting request

→ compatibility validation

→ capacity validation

→ authoritative host relationship.
