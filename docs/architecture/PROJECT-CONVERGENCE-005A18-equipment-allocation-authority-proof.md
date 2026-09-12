# PROJECT-CONVERGENCE-005A18 — Equipment Allocation Authority Proof

## Purpose

005A18 introduces deterministic allocation of scarce externally authoritative equipment across multiple military formations.

005A17 established:

external equipment quantity

→ formation requirement fulfillment

→ equipment condition.

005A18 adds the missing scarcity step:

authoritative stock

→ allocation

→ formation-assigned quantity

→ equipment condition.

## Native Repository Basis

005A17 deliberately avoided a military-specific stock ledger.

Existing engine mechanisms already provide physical inventory and stock behavior outside the military runtime.

Military formations therefore continue to declare requirements without owning the stock from which those requirements are satisfied.

005A18 preserves that boundary.

## External Reference Review

Real military equipment allocation is a scarcity problem.

A national stock of ten serviceable systems cannot simultaneously equip two formations with ten systems each.

Equipment assignment therefore requires an authoritative allocation operation that:

- consumes or reserves finite stock;
- assigns quantities to specific recipients;
- records unmet requirements;
- prevents double counting.

Allocation priority is a separate policy problem.

## Chosen Mechanism

005A18 introduces:

- `MilitaryEquipmentAllocation`;
- `MilitaryEquipmentAllocationResult`;
- `MilitaryEquipmentAllocator`.

The allocator consumes:

- a set of formation identities;
- their already-declared equipment requirements;
- an externally authoritative draw callback.

The callback contract is:

`draw(item_id, requested_quantity) -> quantity actually committed`.

The allocator records only the result of that authoritative draw.

It does not own the underlying stock.

## Authoritative Stock Boundary

The draw provider represents the stock-owning authority.

That authority may later be:

- a national stockpile;
- an equipment depot;
- a logistics node;
- a procurement pool;
- an economy-good inventory;
- another generic stock owner.

The military allocator does not duplicate that state.

## No Double Allocation

If the external stock authority contains 15 units and two formations each require 10:

formation 1 may receive 10;

formation 2 may receive 5;

total assigned = 15;

unmet requirement = 5.

The same 15 units cannot be counted twice because the draw provider mutates or reserves the authoritative external stock.

## Deterministic Ordering

005A18 processes requested formations in ascending stable runtime unique-ID order.

Caller order does not change allocation outcome.

This is a neutral deterministic proof rule.

It is not intended to represent realistic military priority.

Future command, mobilization, procurement, and policy systems may supply explicit priority ordering.

## Assignment Facts

`MilitaryEquipmentAllocationResult` records:

- formation identity;
- equipment item identity;
- requested quantity;
- assigned quantity;
- unmet quantity.

These are allocation facts.

They are not an independent equipment stock ledger.

## Multiple Equipment Types

Equipment types remain independent content-defined stock identities.

Scarcity in one item does not automatically reduce another item's authoritative stock.

## Optional Equipment Capability

Formations without declared equipment requirements:

- remain valid;
- perform no stock draw;
- produce no allocation records.

The military core therefore does not require equipment semantics for every formation.

## Equipment Condition Integration

005A18 allocation results can feed the existing 005A17 equipment-condition evaluator.

The causal chain becomes:

authoritative stock

→ allocation

→ assigned equipment quantity

→ requirement fulfillment

→ equipment condition

→ readiness ceiling

→ bounded readiness transition.

## Validation

The allocator validates the entire formation request set before external draws begin.

It rejects:

- duplicate formation identities;
- unknown formation identities.

This prevents malformed request sets from partially consuming stock.

The authoritative draw provider must return a value within:

`[0, requested_quantity]`.

Invalid provider results are rejected and no partial allocation result is published.

## Atomicity Boundary

The allocator cannot roll back state already mutated by an external draw provider.

Therefore the external stock authority is responsible for validating its own draw operation atomically before committing its mutation.

This is intentional.

The military allocator must not become the transaction authority for every possible stock-owning subsystem.

A later concrete stock integration may provide stronger transactional composition where required.

## Victoria-Specific Boundary

005A18 does not depend on:

- regiment reinforcement pools;
- Victoria military stockpiles;
- army supply sliders;
- legacy unit branches;
- land/naval-only semantics.

## Modern-Specific Boundary

005A18 does not hard-code:

- tanks;
- aircraft;
- missiles;
- trucks;
- naval vessels;
- modern procurement systems.

Equipment identities remain content-defined.

## Calibration Status

005A18 introduces no empirical coefficients.

Ascending formation-ID allocation is a deterministic proof fixture, not a calibrated allocation doctrine.

Future allocation policy may depend on:

- operational priority;
- mobilization status;
- command authority;
- theater priority;
- mission requirement;
- readiness goals;
- reserve policy;
- replacement urgency;
- political decisions.

## Causal Integration

005A18 connects finite physical stock to military readiness through an explicit allocation boundary.

The engine can now represent:

production scarcity

→ insufficient stock

→ incomplete formation allocation

→ reduced equipment condition

→ reduced attainable readiness.

No scripted penalty is required.

## Scope Boundary

005A18 does not implement:

- allocation priority policy;
- procurement decisions;
- depot location;
- transport time;
- equipment transfer;
- equipment return;
- reserve pools;
- mobilization;
- damaged equipment;
- maintenance;
- repair;
- spare parts;
- attrition;
- replacement;
- transactional rollback across arbitrary external authorities.

Those remain later increments.

## Validation Performed

Focused tests verify:

- scarce stock cannot be double allocated;
- allocation order is deterministic by stable formation identity;
- independent equipment-item pools remain independent;
- allocation results feed equipment condition;
- formations without equipment requirements draw nothing;
- duplicate formation requests fail before stock mutation;
- unknown formations fail before stock mutation;
- invalid authoritative draws are rejected.

005A17 and 005A16 regressions remain passing.

The full military regression suite remains passing.

Full CTest is required before closure.

## Result

005A18 establishes:

> Equipment scarcity is now authoritative and allocative rather than merely observational.

The military system declares demand and records assignment.

The physical stock-owning authority remains responsible for the finite stock itself.
