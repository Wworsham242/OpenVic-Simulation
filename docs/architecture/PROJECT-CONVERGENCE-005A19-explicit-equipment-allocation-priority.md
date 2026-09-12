# PROJECT-CONVERGENCE-005A19 — Explicit Equipment Allocation Priority

## Purpose

005A19 replaces the temporary formation-ID-only equipment allocation rule from 005A18 with an explicit generic priority input.

Scarce equipment can now be directed toward formations according to an externally determined priority while preserving:

- deterministic execution;
- finite authoritative stock;
- stable tie-breaking;
- no player privilege;
- no hard-coded operational doctrine.

## Native Repository Basis

005A18 introduced:

- finite external equipment stock;
- deterministic equipment allocation;
- assigned quantities;
- unmet demand;
- equipment-condition integration.

The remaining limitation was that allocation order was determined only by ascending formation runtime identity.

005A19 adds priority without changing stock authority or creating new state ownership.

## External Reference Review

Real equipment allocation is rarely neutral.

Military organizations may prioritize formations according to:

- operational mission;
- theater importance;
- mobilization status;
- readiness objective;
- reserve policy;
- replacement urgency;
- command decisions;
- political decisions.

Those reasons belong to policy, command, AI, or scenario systems.

The allocator should consume a priority signal without owning the reason behind it.

## Chosen Mechanism

005A19 introduces:

`MilitaryEquipmentAllocationRequest`

with:

- `formation_unique_id`;
- `priority`.

Higher numeric priority allocates first.

Priority is intentionally semantic-free.

The allocator does not interpret a priority value as:

- frontline;
- elite;
- guard;
- reserve;
- player-controlled;
- modern;
- rich-country;
- mobilized.

Those meanings remain external.

## Deterministic Ordering

Priority-aware allocation sorts by:

1. higher priority first;
2. lower stable formation unique ID for equal priority.

Caller iteration order therefore has no effect.

This preserves deterministic replay.

## Compatibility

The existing 005A18 overload remains available.

That overload converts each formation identity into a request with priority zero.

Therefore existing callers preserve the previous behavior:

equal priority

→ stable formation-ID ordering.

## Scarcity Conservation

Priority changes distribution, not available stock.

Example:

available stock = 15

formation A requirement = 10

formation B requirement = 10

B has higher priority.

Result:

B receives 10;

A receives 5;

total allocation remains 15.

Priority cannot create equipment.

## Negative Priority

Priority is an ordering value rather than a bounded score.

Negative values remain valid.

This permits an external authority to rank:

high priority;

normal priority;

deprioritized formations

without requiring core semantic categories.

## Validation

Duplicate formation requests remain invalid regardless of differing priorities.

A formation cannot enter the same allocation pass twice under separate priority values.

Validation still occurs before external stock draws begin.

## No Player Privilege

The allocator has no concept of a player-controlled formation.

Human-controlled and AI-controlled actors must supply priority through the same request mechanism.

This preserves the engine rule:

> player input may differ, simulation mechanics do not.

## Victoria-Specific Boundary

005A19 does not depend on:

- reinforcement sliders;
- regiment priority;
- army/navy-only branches;
- national focus;
- Victoria mobilization rules.

## Modern-Specific Boundary

005A19 does not hard-code:

- NATO readiness tiers;
- frontline brigades;
- air wings;
- carrier groups;
- missile formations;
- modern reserve doctrine.

Priority semantics remain external.

## Calibration Status

005A19 introduces no empirical coefficient.

Priority values are ordinal control inputs.

Their production by command, policy, AI, or mobilization systems remains future work.

## Causal Integration

The chain is now:

policy / command / AI / scenario

→ allocation priority

→ scarcity-aware equipment allocation

→ assigned quantity

→ equipment condition

→ readiness.

## Scope Boundary

005A19 does not implement:

- a command hierarchy;
- automatic theater prioritization;
- mobilization priority;
- mission assignment;
- procurement priority;
- replacement priority;
- player UI priority controls;
- AI priority logic;
- dynamic priority recalculation.

Those remain later increments.

## Validation Performed

Focused tests verify:

- explicit priority overrides formation identity;
- equal priority falls back to stable formation identity;
- the 005A18 compatibility overload preserves previous ordering;
- negative priority values remain valid and ordered;
- duplicate prioritized formation requests fail before stock mutation;
- priority changes scarcity distribution without changing total stock.

005A18 and 005A17 regressions remain passing.

The full military regression suite remains passing.

Full CTest is required before closure.

## Result

005A19 establishes:

> Scarce equipment can now be allocated according to externally supplied strategic priority without embedding strategy semantics inside the allocation primitive.

The allocator decides only ordering.

The authority that creates the priority remains outside it.
