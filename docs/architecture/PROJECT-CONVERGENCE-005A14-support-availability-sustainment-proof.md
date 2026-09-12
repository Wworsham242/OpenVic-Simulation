# PROJECT-CONVERGENCE-005A14 — Support Availability to Sustainment Effect Proof

## Purpose

005A14 makes the support relationships introduced in 005A13 causally meaningful.

A formation may now declare zero or more required support types.

For formations that declare support requirements, linked support-node availability drives an authoritative sustainment state.

For formations with no declared support requirements, no mandatory support penalty is applied.

## Native Repository Basis

005A13 established optional typed support relationships:

formation

→ support type

→ support target identifier.

Those relationships intentionally did not yet affect military state.

005A14 adds the first causal consumer of that relationship graph.

The implementation remains inside the generic military formation runtime and does not introduce a separate military facility-state ledger.

## External Reference Review

Real military formations vary greatly in infrastructure dependence.

Some forces can operate with austere or informal support.

Other formations are highly dependent on maintenance, logistics, aviation, depot, medical, or other support systems.

Therefore absence of sophisticated support infrastructure cannot be treated as a universal invalid state.

Instead, dependency must be declared by formation content.

## Chosen Mechanism

`MilitaryFormationDefinition` gains optional required support types.

`MilitaryFormationInstance` gains an authoritative bounded sustainment state.

The runtime evaluates sustainment using an externally supplied support-availability provider.

The provider maps:

support target identifier

→ availability in [0,1].

## No Duplicate Support-State Ledger

The military runtime does not own authoritative operational state for support targets.

It consumes target availability from the authoritative domain that owns those targets.

This preserves one authoritative world state.

## Optional Dependency

If a formation declares no required support types:

sustainment = 1.

The availability provider is not queried.

This preserves valid austere formations.

## Missing Required Support

If a formation declares a support type but has no relationship of that type:

availability for that required type = 0.

Therefore the missing dependency constrains sustainment.

## Same-Type Redundancy

If several support relationships provide the same required support type, they are treated as substitutes.

The best currently available linked target is used.

Example:

logistics node A = 0.2

logistics node B = 0.8

effective logistics availability = 0.8.

This permits infrastructure redundancy without hard-coding major-power behavior.

## Cross-Type Bottleneck

Different required support types are complementary dependencies.

The weakest required support type constrains overall sustainment.

Example:

logistics = 0.8

maintenance = 0.5

sustainment = 0.5.

This is a bounded proof mechanism rather than a final military sustainment equation.

## Authoritative Sustainment

Sustainment is stored as authoritative runtime state on the formation instance.

It remains distinct from readiness.

Readiness may later depend on:

- sustainment;
- personnel;
- equipment condition;
- training;
- fatigue;
- command effectiveness;
- morale;
- operational tempo;
- other domain inputs.

005A14 deliberately does not collapse all military condition into one scalar.

## Causal Update

The path is:

formation support requirements

→ linked support relationships

→ authoritative support target availability

→ per-type availability

→ sustainment.

A support node may therefore degrade or fail without changing the formation's physical position while still degrading the formation's sustainment.

## Validation

The availability provider must return values in [0,1].

Out-of-range values are rejected.

A failed evaluation does not overwrite the previous authoritative sustainment state.

## Victoria-Specific Boundary

005A14 does not depend on:

- armies;
- navies;
- regiments;
- ships;
- `unit_branch_t`;
- Victoria supply mechanics.

## Modern-Specific Boundary

005A14 does not hard-code:

- airbase;
- naval station;
- fuel depot;
- maintenance depot;
- carrier logistics;
- modern military doctrine.

Support dependencies remain data-defined.

## Calibration Status

The proof introduces no empirical military coefficients.

Test values are deterministic fixtures used to prove causal behavior.

The current "best node per type, bottleneck across types" rule is provisional and bounded.

Future domain implementations may replace or extend this aggregation where evidence requires richer mechanics.

## Causal Integration

005A14 is the first causal bridge from military support infrastructure into military formation condition.

Later systems may consume sustainment to influence:

- readiness;
- maintenance backlog;
- fuel state;
- ammunition state;
- replacement flow;
- sortie or mission generation;
- operational tempo;
- recovery rate.

## Scope Boundary

005A14 does not implement:

- stockpile consumption;
- transport flow;
- route capacity;
- actual maintenance throughput;
- fuel burn;
- ammunition expenditure;
- personnel replacement;
- distance effects;
- access rights;
- diplomatic basing rights;
- temporal lag;
- recovery curves;
- combat effects.

Those remain later increments.

## Validation Performed

Focused tests verify:

- formations without declared support remain unpenalized;
- missing declared support reduces sustainment to zero;
- support target availability drives sustainment;
- same-type redundant nodes substitute for one another;
- different support types form a bottleneck;
- support loss changes sustainment without moving the formation;
- invalid provider values are rejected without mutation.

005A13 and 005A12 regressions remain passing.

The broader military suite remains passing.

Full CTest is required before closure.

## Result

005A14 establishes:

> Military infrastructure affects formations through explicit, optional, data-defined dependencies rather than through universal base requirements or country-quality modifiers.

The resulting causal path is:

required support type

→ support relationship

→ authoritative node availability

→ sustainment state.
