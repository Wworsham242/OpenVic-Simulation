# PROJECT-CONVERGENCE-005A21 — Shortfall-Only Equipment Replenishment

## Purpose

005A21 closes the recurring equipment allocation loop.

005A20 established persistent equipment assignment.

005A21 ensures a formation requests only its real outstanding requirement rather than re-requesting its full declared requirement every allocation pass.

## Native Repository Basis

Before 005A21:

declared requirement

→ allocation

→ persistent assignment.

But repeated allocation still had no native mechanism for consulting persistent assignment before creating the next request.

005A21 adds that connection.

## Chosen Mechanism

The equipment allocator now supports an externally supplied request-quantity provider.

The provider receives:

- formation identity;
- equipment item identity;
- declared requirement.

It returns the quantity that should actually enter the allocation pass.

The existing allocation APIs retain their previous behavior by requesting the full declared requirement.

## Persistent Shortfall

`MilitaryEquipmentAssignmentState::allocate_replenishment(...)`

uses persistent assignment to derive:

outstanding requirement =
declared requirement - persistent assigned quantity.

Only that quantity enters the next allocation pass.

Example:

declared requirement = 10

persistent assignment = 6

replenishment request = 4.

Existing assigned equipment is therefore not drawn from external stock again.

## Full Satisfaction

A fully equipped formation generates zero physical draw.

The result remains inspectable, but external stock authority is not called for a zero shortfall.

## Priority Preservation

005A19 priority continues to govern scarce replenishment.

If two formations have outstanding requirements and stock is insufficient, higher-priority requests are processed first.

Persistent assignment changes requested quantity.

It does not bypass allocation priority.

## Release Integration

Equipment released under 005A20 immediately creates new outstanding requirement.

Therefore:

release

→ lower persistent assignment

→ larger shortfall

→ future replenishment demand.

## Execution Boundary

005A21 intentionally introduces a generic request-quantity seam rather than hard-coding perfect organizational execution.

This is important for real-world simulation.

The engine should execute its mechanics correctly and deterministically.

The simulated organization does not have to perform perfectly.

External command, logistics, access, personnel, transport, authority, or institutional mechanisms may derive an executable request smaller than the nominal requirement.

For example:

declared need = 10

organizational / logistics executable fraction = 40%

request entering allocation = 4.

005A21 proves the boundary but does not define the causes of that reduction.

## Reality-Output Principle

The engine should not simulate poor execution by randomly malfunctioning internally.

Instead, poor outcomes should arise from explicit world state and causal constraints.

Examples include:

- inadequate logistics personnel;
- insufficient drivers;
- fuel shortages;
- route capacity;
- damaged infrastructure;
- fragmented territorial authority;
- hostile checkpoints;
- local warlords;
- corruption;
- command delays;
- overloaded staffs;
- poor information;
- interdiction;
- depot access;
- transport shortages.

The engine then resolves those conditions faithfully.

## Example

A formation may nominally require 100 units of supply.

The route may cross three territories controlled by separate actors.

One actor permits transit.

One delays transit.

One denies access.

Transport capacity may also be limited.

The resulting deliverable amount should emerge from those constraints rather than from an arbitrary generic penalty.

## Authority Boundary

005A21 preserves the distinction between:

- declared requirement;
- persistent assignment;
- executable request;
- available external stock;
- actual allocation.

Those are separate causal stages.

## Fixed-Point Arithmetic

The focused proof avoids floating-point decimal construction.

Ratios are expressed through deterministic integer fixed-point arithmetic such as:

`(quantity * numerator) / denominator`

This preserves deterministic fixed-point behavior and avoids introducing binary floating-point into authoritative simulation math.

## Victoria-Specific Boundary

005A21 does not depend on:

- Victoria reinforcement sliders;
- regiment pools;
- army/navy-only branches;
- national reinforcement rates.

## Modern-Specific Boundary

005A21 does not hard-code:

- modern brigade replenishment;
- NATO logistics;
- truck convoys;
- modern depots;
- specific command structures.

The execution seam is generic.

## Calibration Status

005A21 introduces no empirical coefficient.

The 40% value used in testing is only a proof of the external execution boundary.

Real execution modifiers must come from explicit domain mechanics or calibrated scenario data.

## Causal Integration

The equipment chain is now:

declared requirement

→ persistent assignment

→ real shortfall

→ externally derived executable request

→ priority

→ finite stock allocation

→ persistent assignment

→ equipment condition

→ readiness.

## Scope Boundary

005A21 does not yet implement:

- route traversal;
- territorial access;
- convoy state;
- fuel logistics;
- logistics personnel;
- staff capacity;
- command delay;
- corruption;
- interdiction;
- depot geography;
- transport queues;
- delivery time.

Those systems may later feed the request/delivery boundary created here.

## Validation Performed

Focused tests verify:

- replenishment requests only persistent shortfall;
- fully equipped formations perform no physical draw;
- priority still governs scarce replenishment;
- release creates replenishment demand;
- the request-quantity seam can represent externally constrained execution.

005A20 and 005A19 regressions remain passing.

The full military regression suite remains passing.

Full CTest is required before closure.

## Result

005A21 establishes:

> Replenishment is history-aware, scarcity-aware, and capable of accepting externally derived execution constraints.

The engine remains deterministic.

The simulated world is free to be inefficient, fragmented, delayed, corrupt, or operationally incompetent through explicit causal state.
