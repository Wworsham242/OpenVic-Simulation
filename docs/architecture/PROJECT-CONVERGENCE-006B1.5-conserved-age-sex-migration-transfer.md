# PROJECT-CONVERGENCE-006B1.5 — Conserved Age-Sex Migration Transfer

**Program:** PROJECT-CONVERGENCE-006B1 — Causal Population Evolution

## Purpose

Establish a deterministic, population-conserving origin-to-destination
migration transfer using the existing age/sex demographic authority.

This is a movement/accounting substrate. It is not yet a migration-choice
model.

## Repository Reconciliation

Repository reconnaissance found inherited OpenVic migration concepts including:

- `migration_chance`;
- `colonialmigration_chance`;
- `emigration_chance`;
- country migration-target weights;
- province migration-target weights;
- immigrant push/attract modifiers;
- `IMMIGRATION_SCALE`;
- POP result counters for internal, external and colonial migration.

These are inherited Victoria-specific scripts, modifiers, reporting surfaces,
or configuration concepts.

No general authoritative origin-to-destination demographic transfer owner was
demonstrated.

`BasicResourceStressResponse::mobility_push_pressure` is explicitly an
incentive/aspiration input, not migration itself.

B1.5 therefore establishes the missing conserved transfer seam rather than
making the inherited scripted migration system authoritative for the general
engine.

## Empirical Method

Cohort-component demographic methods treat migration as a distinct component of
population change.

The U.S. Census cohort-component formulation includes migration separately from
births and deaths.

State projection methodology explicitly subtracts domestic out-migrants from
their origin and adds them to their destination as in-migrants.

United Nations demographic projection methods support net migration by age and
sex as an explicit cohort-component input.

References:

- U.S. Census Bureau, Population Projections:
  https://www.census.gov/programs-surveys/popproj/about.html

- U.S. Census Bureau, state population projection methodology.

- United Nations cohort-component projection methodology.

## Chosen Mechanism

`DemographicMigrationTransfer` contains one explicit age/sex migrant structure.

For every cell:

    origin emigration
        =
    destination immigration

The implementation reuses the 006B1.1 component authority:

    origin:
        emigration = transfer

    destination:
        immigration = transfer

A transfer is applied only if both sides are valid.

## Conservation Invariant

For a valid transfer of N persons:

    origin_end =
        origin_start - N

    destination_end =
        destination_start + N

and:

    origin_start + destination_start
        =
    origin_end + destination_end

Age and sex composition are preserved exactly.

Migration cannot create or destroy population.

## Transactional Boundary

The transfer function is pure.

If the origin cannot supply the requested migrants, the transfer is rejected.

If the destination would overflow, the transfer is rejected.

On failure, returned ending stocks remain equal to their starting stocks.

No caller observes a half-applied migration in which an origin loses population
but the destination fails to receive it.

## Geography Boundary

B1.5 does not encode:

- country;
- colony;
- state;
- province;
- urban/rural;
- player versus AI.

The mechanism transfers between two demographic stocks.

A later geography/runtime owner can bind those stocks to appropriate world
locations.

This avoids making inherited Victoria political geography part of the general
engine ontology.

## Causal Boundary

B1.5 does not decide who migrates or where they go.

Future migration-flow generation may consume:

- mobility push;
- destination wages and employment;
- housing availability and cost;
- safety and conflict;
- environmental conditions;
- family/social networks;
- legal restrictions;
- transport accessibility and cost;
- wealth/mobility capacity;
- actor-perceived destination information.

The intended chain is:

    pressures / opportunities / constraints / perception
        -> migration decision or flow producer
        -> explicit age/sex transfer
        -> B1.5 conserved movement
        -> B1.1 demographic accounting

rather than:

    generic migration modifier
        -> population appears somewhere

## Scope Boundary

Not implemented here:

- migration propensity;
- destination utility;
- migration network choice;
- household migration decisions;
- refugee status;
- legal border policy;
- transportation capacity;
- internal/international classification;
- POP socioeconomic resizing;
- live province mutation;
- demographic scheduling.

## Acceptance

Tests prove:

1. zero transfer is identity;
2. age/sex cohort identity is preserved;
3. origin loss exactly equals destination gain;
4. combined population is conserved;
5. excessive origin transfer is rejected;
6. negative transfer is rejected;
7. destination overflow rejects atomically;
8. mechanism is geography-type agnostic;
9. mobility pressure does not itself move population;
10. identical inputs produce identical results.

Completion requires successful build, focused tests, full CTest, research gate,
clean diff, commit, push, and exact local/remote HEAD agreement.