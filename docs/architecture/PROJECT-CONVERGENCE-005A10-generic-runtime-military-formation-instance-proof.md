# PROJECT-CONVERGENCE-005A10 — Generic Runtime Military Formation Instance Proof

## Purpose

005A10 proves that a military formation outside the inherited LAND/NAVAL hierarchy can exist as authoritative runtime state.

The proof builds on:

- 005A8 — extensible military-domain identity;
- 005A9 — generic domain/capability formation definitions.

005A10 adds the first generic runtime military formation instance without deriving from `UnitInstance`, `RegimentInstance`, `ShipInstance`, `ArmyInstance`, or `NavyInstance`.

## Native Repository Basis

The inherited OpenVic runtime military model centers on `UnitInstance`.

`UnitInstance` requires a `UnitType`.

`UnitType` requires `unit_branch_t`.

The inherited LAND specialization additionally introduces POP and mobilization state, while the NAVAL specialization uses a different type structure.

Therefore subclassing or extending `UnitInstance` for new domains would preserve the same closed branch dependency.

005A10 instead introduces a parallel generic runtime instance seam.

## External Reference Review

The Real-World Model Reference Framework requires authoritative state and domain-appropriate mechanisms.

A generic runtime military entity requires stable authoritative identity before movement, basing, combat, logistics, or mission models are attached.

The minimum useful runtime state is therefore:

- unique runtime identity;
- human-readable mutable name;
- immutable formation-definition reference;
- minimal bounded readiness state.

No additional operational assumptions are introduced.

## Chosen Mechanism

005A10 introduces:

`MilitaryFormationInstance`

and:

`MilitaryFormationInstanceManager`

Each instance contains:

- `unique_id_t`;
- mutable name;
- reference to `MilitaryFormationDefinition`;
- bounded readiness value.

Domain and capability information are inherited through the formation definition.

## Runtime Identity

The manager assigns monotonically increasing `unique_id_t` values.

Two instances of the same formation definition therefore remain distinct authoritative runtime objects.

Runtime identity is separate from formation-definition identity.

## Definition Relationship

A runtime instance references one immutable `MilitaryFormationDefinition`.

Through that definition it can access:

- generic military domain;
- generic capability membership.

The runtime instance itself contains no `unit_branch_t`.

## Readiness

005A10 introduces one minimal mutable state value:

`readiness`

The value is constrained to:

`0 <= readiness <= 1`

Invalid readiness changes are rejected without mutating the existing value.

Invalid creation requests are rejected before authoritative runtime state changes.

This readiness value is deliberately generic.

005A10 does not define what causes readiness to rise or fall.

Future systems may connect readiness to:

- maintenance;
- personnel;
- training;
- supply;
- damage;
- sortie or operational tempo;
- command state;
- basing;
- environmental conditions.

## Victoria-Specific Boundary

A generic runtime formation instance does not require:

- regiment;
- ship;
- army;
- navy;
- general;
- admiral;
- soldier POP;
- mobilisation flag;
- land province;
- sea province.

The inherited runtime hierarchy remains intact for compatibility.

## Modern-Specific Boundary

005A10 does not hard-code:

- aircraft;
- satellite;
- missile battery;
- cyber unit;
- drone formation;
- airbase;
- orbital position.

The tests use air and space domain identities only as proof fixtures.

A different setting may use entirely different domains.

## Causal Integration

005A10 establishes authoritative mutable military state but introduces no autonomous causal update.

Readiness changes only through the native setter.

No tick behavior, mission execution, movement, combat, supply consumption, or attrition is introduced.

This keeps causal ownership explicit for later increments.

## Validation

Focused tests verify:

- a nonlegacy domain can produce an authoritative runtime instance;
- runtime identity is unique and stable;
- two instances may share one definition while retaining distinct IDs;
- domain identity is inherited from the generic definition;
- capability membership is inherited from the generic definition;
- readiness can be initialized and mutated;
- readiness is bounded to `[0,1]`;
- invalid readiness mutation does not alter existing state;
- empty-name creation is rejected;
- invalid-readiness creation does not mutate runtime state.

## Scope Boundary

005A10 does not:

- replace `UnitInstance`;
- replace `ArmyInstance`;
- replace `NavyInstance`;
- add a new `unit_branch_t`;
- implement movement;
- implement location;
- implement basing;
- implement combat;
- implement missions;
- implement logistics;
- implement equipment inventories;
- implement personnel;
- implement command hierarchy;
- implement ownership or country binding;
- implement persistence.

Those are later runtime-mechanics problems.

## Calibration Status

No empirical coefficients are introduced.

The readiness values used in tests are deterministic proof values only.

## Result

005A10 establishes:

> A military formation outside the inherited LAND/NAVAL hierarchy can exist as authoritative runtime state with stable identity and bounded mutable readiness.

The resulting generic chain is:

`MilitaryDomainDefinition`

→ `MilitaryCapabilityDefinition`

→ `MilitaryFormationDefinition`

→ `MilitaryFormationInstance`

This is the first complete nonlegacy military path from definition identity to authoritative runtime state.
