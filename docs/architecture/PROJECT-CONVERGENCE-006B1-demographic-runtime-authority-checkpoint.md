# PROJECT-CONVERGENCE-006B1 — Demographic Runtime Authority Checkpoint

**Checkpoint after:** PROJECT-CONVERGENCE-006B1.1 through 006B1.5

## Purpose

Evaluate whether the completed demographic mechanisms should now mutate live
OpenVic socioeconomic POP authority.

This checkpoint introduces no runtime mutation.

## Completed Demographic Substrate

006B1 now contains:

- B1.1 — explicit demographic transition accounting;
- B1.2 — explicit cohort aging;
- B1.3 — age-specific fertility birth generation;
- B1.4 — age/sex-specific mortality death generation;
- B1.5 — conserved age/sex migration transfer.

Together these provide deterministic demographic mechanics for:

    aging
    births
    deaths
    migration

with explicit age/sex state and conservation/accounting boundaries.

## Native Runtime Authority

Repository reconciliation confirms that:

    Pop::size

remains the authoritative socioeconomic people count.

`PopBase::size` is protected and has no general public runtime setter.

This is appropriate because POP size participates directly in multiple live
mechanisms.

Observed size-dependent state and calculations include:

- unemployment through `size - employed`;
- native workforce availability;
- RGO and productive-site employment;
- artisan production scaling;
- ideological supporter equivalents;
- party-policy and reform supporter equivalents;
- vote equivalents;
- recruitment and supported-regiment calculations;
- POP needs and economic scaling;
- province population aggregation;
- state population aggregation;
- country population aggregation.

A demographic mechanism must therefore not mutate POP size independently of
these dependent states.

## Existing Aggregate Authority

`ProvinceInstance` owns the actual POP collection.

`ProvinceInstance::_update_pops()`:

1. clears the province POP aggregate;
2. iterates the live POP collection;
3. updates POP-derived game state;
4. rebuilds population totals and distributions from those POPs;
5. normalizes the resulting aggregate.

Therefore province population total is derived from native POP authority.

No second demographic population-total ledger should be introduced.

State and country population aggregates likewise consume lower-level POP
aggregates rather than establishing an independent demographic population
authority.

## Age/Sex Authority Relationship

`ProvinceDemographicAgeSexState` stores optional demographic composition.

Initialization reconciles the structure against:

    ProvinceInstance::get_total_population()

and consistency requires:

    sum(age/sex cells)
        ==
    authoritative province population

The stored age/sex structure is therefore demographic detail describing the
same people represented by native POPs.

It is not permission to evolve an independent population total.

The current state object exposes initialization and consistency checking but no
sanctioned live transition mutation.

`PopDemographicAgeSexState` has the same initialization-only characteristic.

## Why a Pop Size Setter Is Not the Solution

Adding a public:

    Pop::set_size(...)

would create a mutation path capable of changing population while leaving
dependent state inconsistent.

For example, shrinking a POP could leave:

    employed > size

during an employment cycle.

Political-support equivalents and other size-scaled quantities could also
retain totals based on the previous population.

Province, state and country aggregates would remain stale until a later rebuild.

A bare setter therefore violates the repository's existing ownership and
derived-state boundaries.

## Migration Exposes the Deeper Reconciliation Problem

B1.5 deliberately transfers age/sex demographic population.

A native socioeconomic POP also requires identity including at least:

- POP type;
- culture;
- religion.

The current migration transfer does not invent those dimensions.

For an origin-to-destination transfer the demographic system can prove:

    origin age/sex loss
        ==
    destination age/sex gain

but it cannot infer how those migrants should be represented among destination
socioeconomic POPs.

Possible shortcuts are invalid without an explicit model:

- distributing migrants proportionally across existing destination POPs would
  fabricate socioeconomic composition;
- assigning migrants to arbitrary destination POPs would fabricate identity;
- reconstructing occupation/culture/religion from province age/sex data is not
  possible without a defensible joint-distribution model;
- blindly cloning origin POP records would impose a socioeconomic migration
  policy that B1.5 does not currently define.

This confirms the earlier 004A9 rule that province demographic composition must
not be used to invent finer socioeconomic cross-classifications.

## Future Runtime Mutation Boundary

When a concrete vertical requires live demographic population change, the
correct boundary should be a sanctioned population transaction rather than a
general size setter.

Such a transaction will need to coordinate, as applicable:

    socioeconomic POP changes
        +
    employment-cycle safety
        +
    size-scaled POP distributions
        +
    POP creation/removal/merge/split rules
        +
    province demographic detail
        +
    province aggregate rebuild
        +
    higher-level aggregate rebuild

The transaction must preserve one authoritative people count.

It must not retain a parallel demographic total.

## Scheduling Constraint

Population mutation must occur at a simulation phase in which employment and
other size-dependent allocations cannot observe a half-transitioned state.

The current employment system resets and reallocates POP employment on its
defined cycle.

A future mutation transaction must explicitly define its ordering relative to:

- POP tick;
- employment reset;
- workforce allocation;
- production;
- market clearing;
- aggregate refresh.

This checkpoint does not select that phase.

## Migration Payload Boundary

A future socioeconomic migration producer may need a richer payload than B1.5.

That producer may identify the socioeconomic composition of migrants using
actual causal inputs or source POP identity.

B1.5 should remain the demographic conservation kernel:

    explicit migrant age/sex transfer
        ->
    origin loss equals destination gain

A richer upstream migration mechanism can later bind that transfer to native
POP identity.

## Decision

006B1 is accepted as a completed demographic mechanism substrate.

It is deliberately not wired directly into live POP mutation.

The absence of a runtime bridge is an explicit integration boundary, not a
reason to create a second population ledger or expose unrestricted POP resizing.

The project now leaves the population subsystem in accordance with the breadth
rule.

## Deferred Population Integration Requirements

Future work must return to this boundary when demanded by a concrete vertical
and resolve:

1. sanctioned POP resize/split/merge/create/remove authority;
2. preservation or recomputation of size-scaled socioeconomic state;
3. employment-cycle ordering;
4. province demographic state mutation;
5. demographic-to-socioeconomic reconciliation;
6. socioeconomic identity of migrants;
7. aggregate invalidation/rebuild;
8. deterministic replay and persistence of population transactions.

None is implemented in this checkpoint.

## Next Breadth Step

Proceed to the next major domain rather than extending the demographic series.

The next planned domain is:

    PROJECT-CONVERGENCE-006B2
    Environment / Civil Resources

This restores cross-domain breadth while retaining the completed demographic
substrate for later causal integration.