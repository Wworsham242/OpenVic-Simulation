# PROJECT-CONVERGENCE-005A20 — Persistent Equipment Assignment State

## Purpose

005A20 makes military equipment assignment history-bearing.

005A18 and 005A19 established deterministic scarcity-aware allocation.

Those mechanisms answer:

which formation receives newly available equipment?

005A20 answers:

what equipment remains committed to a formation afterward?

## Native Repository Basis

The existing equipment chain before 005A20 was:

external stock

→ allocation

→ transient allocation result

→ equipment condition

→ readiness.

The allocation result itself was not persistent.

Repeated allocation could therefore conceptually begin from zero unless another system remembered prior assignment.

005A20 adds that missing state without duplicating unassigned stock.

## External Reference Review

Equipment assigned to a formation is materially different from equipment sitting in a depot, reserve pool, factory inventory, or national stockpile.

Once assigned, it normally remains with that formation until:

- returned;
- transferred;
- lost;
- destroyed;
- consumed;
- sent for repair;
- otherwise removed.

Therefore assignment must be history-bearing rather than recomputed as a disposable per-tick value.

## Chosen Mechanism

005A20 introduces:

- `MilitaryEquipmentAssignment`;
- `MilitaryEquipmentAssignmentState`.

Assignment records contain:

- formation identity;
- content-defined equipment item identity;
- assigned quantity.

## Authority Boundary

Persistent assignment is authoritative state for equipment committed to formations.

It is not a duplicate representation of unassigned stock.

The boundary is:

external unassigned stock

→ allocation/draw

→ persistent assignment.

Equipment belongs on one side of that boundary at a time.

## Allocation Commit

`apply_allocation_result(...)` commits successful allocation facts into persistent assignment state.

Existing assigned quantity is retained.

New allocation adds to existing assignment only if the final quantity remains within the formation's declared requirement.

A repeated commit of the same full allocation is therefore rejected rather than duplicating equipment.

## Outstanding Requirement

The assignment state derives:

outstanding requirement =
declared requirement - persistent assigned quantity.

This creates the basis for later replenishment behavior.

Example:

declared requirement = 10

persistent assignment = 6

outstanding requirement = 4.

A future replenishment allocator should request only 4, not another 10.

## Release

Assigned equipment may be returned to an external stock authority.

Release follows:

persistent assignment

→ external return callback

→ assignment reduction.

The local assignment is reduced only after the external authority accepts the return.

A rejected return leaves persistent assignment unchanged.

## Transfer

Assigned equipment may move directly from one formation to another.

Transfer:

- reduces source assignment;
- increases target assignment;
- conserves total assigned quantity;
- does not draw new external stock;
- does not return stock to the unassigned pool.

The target cannot receive more than its declared requirement.

The source cannot transfer more than it currently has.

## Persistence

Assignment survives destruction or clearing of the transient allocation result.

The state therefore represents historical commitment rather than an allocation calculation artifact.

## Equipment Condition Integration

Persistent assignment can directly provide quantities to the existing 005A17 equipment-condition evaluator.

The chain becomes:

external stock

→ scarcity-aware allocation

→ persistent assignment

→ requirement fulfillment

→ equipment condition

→ readiness.

## Victoria-Specific Boundary

005A20 does not depend on:

- regiment reinforcement pools;
- Victoria mobilization mechanics;
- land/naval branch assumptions;
- army supply sliders;
- reinforcement rates.

## Modern-Specific Boundary

005A20 does not assume:

- tanks;
- aircraft;
- missiles;
- trucks;
- ships;
- modern depot doctrine;
- modern force structures.

Equipment item identities remain content-defined.

## Calibration Status

005A20 introduces no empirical coefficients.

Assignment quantities are physical state.

Transfer and release are direct state transitions rather than calibrated formulas.

## Causal Integration

005A20 changes equipment from a transient observation into history-bearing military state.

The resulting causal path is:

production / procurement / stock

→ allocation priority

→ finite-stock allocation

→ persistent formation assignment

→ equipment condition

→ readiness.

Later attrition, maintenance, repair, and replacement mechanics can now operate on a persistent assigned quantity instead of an ephemeral allocation result.

## Scope Boundary

005A20 does not implement:

- automatic replenishment;
- attrition;
- combat loss;
- damaged-equipment state;
- maintenance;
- repair;
- spare parts;
- depot location;
- transport time;
- return transit;
- capture;
- abandonment;
- equipment age;
- individual equipment entities.

Those remain later increments.

## Validation Performed

Focused tests verify:

- committed assignment survives the transient allocation result;
- repeated commit cannot duplicate assignment;
- release returns equipment before reducing local assignment;
- rejected external return preserves assignment;
- transfer conserves total assigned quantity;
- transfer cannot exceed source quantity;
- persistent assignment directly feeds equipment condition.

005A19 and 005A18 regressions remain passing.

The full military regression suite remains passing.

Full CTest is required before closure.

## Result

005A20 establishes:

> Equipment assignment is now history-bearing state rather than a transient allocation calculation.

Unassigned stock remains external.

Assigned equipment remains with the formation until an explicit causal transition moves it elsewhere.
