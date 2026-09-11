# PROJECT-CONVERGENCE-005A7 — Restricted Calculation Proof

## Purpose

005A7 proves that scenario-defined arithmetic can alter one bounded native calculation without gaining authority over arbitrary simulation state.

The proof introduces a small deterministic fixed-point evaluator and uses it only at the productive-site utilization profitability-pressure seam.

The authoritative simulation owner remains native code.

## Native Repository Basis

Before 005A7, the repository already contained:

- fixed-point deterministic arithmetic;
- `ProductiveSiteUtilizationDecision`;
- explicit utilization decision inputs and policy;
- validation of utilization policy;
- runtime ownership of productive-site utilization mutation;
- Victoria-compatible `ConditionScript`;
- `ConditionalWeight`;
- modifier parsing;
- an unimplemented `EffectScript` execution path.

The existing utilization path was already structurally suitable for a restricted calculation proof:

authoritative operating economics

→ explicit `ProductiveSiteUtilizationDecisionInput`

→ pure calculation

→ `ProductiveSiteUtilizationDecisionResult`

→ runtime applies `next_utilization`.

## External Reference Review

The mandatory Real-World Model Reference Framework requires explicit causal interfaces, deterministic behavior, bounded extension points, preserved authoritative ownership, and a distinction between derived calculation and state mutation.

005A7 does not introduce a general scripting VM.

The existing Victoria-style script and modifier systems were inspected and rejected as the primary mechanism for this proof because:

- `ConditionScript` is built around inherited scope semantics;
- `ConditionalWeight` is condition/modifier composition rather than a generic typed evaluator;
- modifier machinery maps compiled modifier effects to numeric values;
- `EffectScript` does not yet provide authoritative execution semantics.

Reusing those mechanisms as a general formula system would therefore import inherited ontology and mutation ambiguity into the new engine boundary.

## Chosen Mechanism

005A7 introduces:

`RestrictedCalculationDefinition`

A calculation definition contains:

- an explicit input count;
- a bounded list of instructions;
- fixed-point constants;
- no world references;
- no callbacks;
- no loops;
- no dynamic state access.

Supported operations are:

- push explicit input;
- push constant;
- add;
- subtract;
- multiply;
- minimum;
- maximum;
- negate.

Hard limits constrain:

- maximum inputs;
- maximum instruction count;
- maximum stack depth.

The evaluator returns:

`std::optional<fixed_point_t>`

Invalid definitions or invalid input shape produce no result.

## Validation

`RestrictedCalculationDefinition::is_valid()` verifies:

- at least one declared input;
- input count is below the hard maximum;
- at least one instruction exists;
- instruction count is below the hard maximum;
- all referenced input indexes exist;
- stack depth cannot overflow;
- operators cannot underflow the stack;
- unknown operation values are rejected;
- the program terminates with exactly one result.

The evaluator revalidates before execution.

## Explicit Input Boundary

For the 005A7 productive-site proof, the formula receives exactly two values:

1. normalized operating-surplus signal;
2. profitability weight.

It receives no references to:

- `LiveEconomyRuntime`;
- POPs;
- provinces;
- countries;
- goods;
- inventories;
- markets;
- productive-site objects;
- employment state;
- logistics state.

The formula therefore cannot discover or mutate arbitrary simulation state.

## Causal Integration

The native calculation still performs:

operating surplus

→ normalized surplus signal

→ profitability pressure

→ unconstrained utilization target

→ policy clamp

→ adjustment-rate response

→ next utilization.

Only the profitability-pressure scalar may be replaced by a restricted calculation.

The native domain still owns all later constraints.

## Authority Boundary

`RestrictedCalculationDefinition` cannot mutate authoritative state.

It only returns a candidate fixed-point value.

`ProductiveSiteUtilizationDecision` remains responsible for:

- utilization target construction;
- minimum/maximum policy constraints;
- adjustment rate;
- returned decision structure.

`LiveEconomyRuntime` remains responsible for applying:

`next_utilization`

to the authoritative producer.

Invalid policy configuration is rejected before it can enable a runtime utilization decision.

## Legacy Parity

When no restricted formula is configured, the original native formula remains:

`normalized_surplus_signal * profitability_weight`

A test also defines an equivalent restricted formula and verifies that it produces exactly the same `ProductiveSiteUtilizationDecisionResult`.

Thus 005A7 does not alter legacy behavior by default.

## Invalid Formula Behavior

An invalid calculation definition is rejected by policy validation.

The runtime configuration API therefore returns false.

A test verifies that the site's authoritative utilization remains unchanged after the rejected configuration.

This establishes the required rule:

> Invalid extension definitions fail without authoritative mutation.

## Determinism

The evaluator uses:

- deterministic fixed-point arithmetic;
- fixed instruction ordering;
- a fixed local stack;
- no external state reads;
- no random source;
- no time source;
- no callbacks.

Identical calculation definition and inputs therefore produce identical output.

The focused test evaluates the same program twice and requires exact equality.

## Victoria-Specific Boundary

005A7 does not reuse Victoria country/province script scopes as the formula execution environment.

It does not require:

- `THIS`;
- `FROM`;
- country scope;
- province scope;
- legacy condition identifiers;
- modifier-effect registries.

This prevents inherited Victoria scripting semantics from becoming the general calculation architecture.

## Modern-Specific Boundary

The restricted evaluator also contains no modern-world concepts.

It does not know:

- profitability;
- firms;
- electricity;
- finance;
- missiles;
- banking;
- governments.

Those meanings are supplied by the native caller through explicit numeric inputs.

The evaluator is mechanism-only.

## Calibration Status

No empirical coefficient or behavioral calibration is introduced by the evaluator.

The test formulas are proof fixtures.

Existing productive-site utilization policy values retain their previous status.

## Scope Boundary

005A7 does not:

- create a general-purpose programming language;
- execute arbitrary effects;
- mutate arbitrary native state;
- introduce dynamic world lookup;
- add branches or loops;
- add recursion;
- expose pointers;
- add filesystem or network access;
- replace `ConditionScript`;
- replace modifiers;
- replace `EffectScript`;
- replace native domain calculations;
- make every calculation data-defined.

The increment proves only that a bounded deterministic calculation surface can safely modify one explicit numeric relationship.

## Validation Performed

005A7 validation includes:

- deterministic same-input/same-output evaluation;
- invalid-operation rejection;
- hard evaluator validation;
- bounded productive-site utilization output;
- parity with the existing native formula;
- rejected invalid configuration without authoritative mutation;
- existing utilization regression;
- full CTest;
- `git diff --check`;
- exact staged-file verification;
- local/remote HEAD equality after push.

## Result

005A7 establishes the following engine rule:

> Data-defined calculations may transform explicitly supplied values, but only native domain owners may validate and apply resulting authoritative state changes.

This supplies a safe general calculation boundary without turning the legacy script system into universal simulation physics.
