# PROJECT-CONVERGENCE-006B1.1 — Province Demographic Transition Accounting

**Program:** PROJECT-CONVERGENCE-006B1 — Causal Population Evolution

## Purpose

Establish the first deterministic demographic transition mechanism over the
existing authoritative province age-sex representation.

The increment accounts explicitly for:

- births;
- deaths;
- immigration;
- emigration;
- exact ending population;
- exact net population change.

It does not yet mutate native OpenVic socioeconomic POP state.

## Native Repository Basis

The repository already contains:

- `PopulationAgeSexStructure`, with 18 five-year age groups by female/male;
- exact demographic profile materialization and reconciliation;
- optional POP demographic attachment;
- province-level demographic age-sex state;
- deterministic demographic source selection/import;
- native socioeconomic population authority through `Pop::size`;
- province aggregate population rebuilt from live POPs.

Repository reconnaissance for 006B1.1 found no established runtime mechanism
for demographic resizing, splitting, merging, removal, birth creation, or
general population transfer among live POP records.

Changing `Pop::size` directly would also affect inherited state whose totals
are population-scaled, including employment and political/support
distributions.

Therefore this increment does not invent a public POP-size setter or a hidden
socioeconomic allocation rule.

## External Reference Review

The chosen accounting shape follows the cohort-component demographic identity:

    P(t+1) = P(t) + births - deaths + immigration - emigration

This increment implements only the accounting contract. It does not implement
empirical fertility, mortality, survival, or migration models.

## Chosen Mechanism

`DemographicPopulationComponents` carries explicit component flows.

Transition order is defined as:

    starting age-sex stock
        -> deaths
        -> births into age 0-4
        -> immigration
        -> emigration
        -> ending age-sex stock

The order is part of the contract and prevents hidden ambiguity when multiple
components affect the same cohort during one transition.

The transition is pure and transactional:

- the starting structure is never mutated;
- all counts must be nonnegative;
- deaths cannot exceed the starting population of a cell;
- births are admitted only into age 0-4;
- cell overflow is rejected;
- emigration cannot exceed the population available after prior components;
- any failed transition returns the unchanged starting structure;
- successful results must satisfy the exact population identity.

## Calibration Status

No empirical coefficients are introduced.

All transition component counts are explicit caller-provided authoritative
inputs for this increment.

## Causal Integration

Future demographic mechanisms may produce component counts:

- fertility -> births;
- mortality/health -> deaths;
- migration mechanisms -> immigration/emigration.

Those mechanisms will feed this accounting contract rather than directly
changing downstream population totals.

A later reconciliation increment must map valid demographic change into the
native socioeconomic POP partition without inventing unsupported occupation,
culture, religion, political-support, or employment characteristics.

## Scope Boundary

Not implemented here:

- cohort aging;
- fertility-rate calculation;
- mortality-rate calculation;
- disease;
- health-to-mortality conversion;
- migration decisions;
- migration destination selection;
- socioeconomic POP resizing;
- POP creation/removal/split/merge;
- political effects;
- labor-force effects;
- demographic persistence integration.

## Acceptance

Targeted tests must prove:

1. identity under zero components;
2. births enter only age 0-4;
3. explicit deaths reduce only their cells;
4. immigration/emigration retain cohort identity;
5. mixed transitions satisfy exact conservation;
6. impossible deaths are rejected atomically;
7. impossible emigration is rejected atomically;
8. negative components are rejected;
9. cell overflow is rejected;
10. identical inputs produce identical outputs.

Completion still requires successful build, targeted tests, full CTest,
architecture review, clean diff, commit, push, and exact remote HEAD
verification.