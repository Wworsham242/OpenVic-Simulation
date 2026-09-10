# PROJECT-CONVERGENCE-004A7 - Demographic Profile Reconciliation

Research-Gate-Version: 1

## Purpose

004A7 establishes the deterministic reconciliation seam between an
externally supplied age-sex distribution and OpenVic's authoritative
population total.

It does not yet attach demographic structure to live POPs.

## Native Repository Basis

OpenVic currently owns authoritative population through:

    Pop::size

004A6 added:

    PopulationAgeSexStructure

with:

- 18 five-year age groups from 0-4 through 85+;
- female and male demographic accounting cells;
- exact total-population accounting;
- nonnegative validation.

004A6 deliberately did not attach this structure to `Pop`, because no
authoritative age-sex distribution exists in the native repository.

004A7 preserves that boundary.

The new demographic profile is not population state. It contains only
relative weights used to construct an age-sex structure for a known
authoritative population total.

## External Reference Review

### U.S. Census Bureau

The Census Bureau's cohort-component methodology starts from an
estimated base population classified by age and sex.

The age distribution of that base population is essential information
carried through future projections.

Census separates the population stock from the future demographic
components:

- fertility;
- mortality;
- migration.

Reference:

https://www2.census.gov/programs-surveys/popproj/technical-documentation/methodology/methodstatement23.pdf

Census also documents that cohort-component projection can be performed
using either single-year or five-year age groups.

Reference:

https://www.census.gov/data/software/rup/overview.html

### United Nations World Population Prospects

UN World Population Prospects methodology distinguishes the base
population and benchmark population.

A benchmark population is a reliable population count or age-sex
estimate used to validate or adjust demographic estimates.

This reinforces the separation between:

    authoritative total / benchmark
        and
    age-sex distribution estimate

Reference:

https://www.un.org/development/desa/pd/content/world-population-prospects-2024

### International Futures

International Futures represents population using age-sex cohorts.

Its demographic documentation specifies that the sum across cohorts and
both sexes equals total population.

Fertility adds births to the lower cohort and mortality subtracts deaths
from the affected cohorts.

Reference:

https://korbel.du.edu/pardee-resources/ifs-population-model-documentation/

### Defense / strategic models

The mandatory framework's defense references were reviewed for this
increment.

JTLS-GO, DARPA World Modelers, Causal Exploration, and political
expected-utility models do not provide a superior native mechanism for
base demographic age-sex apportionment.

They are therefore not imported into 004A7.

This is an intentional finding, not an omitted research category.

### Game abstractions

Victoria-style POP aggregation remains useful for representing
socioeconomic groups without individual-person simulation.

No game-specific age-distribution formula is adopted.

The authoritative total remains the existing OpenVic POP size.

## Chosen Mechanism

004A7 introduces:

    DemographicAgeSexProfile

containing 36 nonnegative integer weights:

    18 age groups
        x
    female / male

The weights are relative proportions.

They do not need to sum to a fixed denominator.

For authoritative population:

    P

and profile cell weight:

    w_i

with total profile weight:

    W = sum(w_i)

the ideal count for each age-sex cell is:

    q_i = P * w_i / W

Because population counts must be integers, first assign:

    floor(q_i)

This can leave a small number of people unassigned because of rounding.

The remaining population is distributed to cells with the largest
integer division remainders.

Tie breaking is deterministic:

    lower flattened cell index first

where the flattened ordering is:

    age 0-4 female
    age 0-4 male
    age 5-9 female
    age 5-9 male
    ...

This produces:

    sum(final cohort counts) = P

exactly.

The apportionment procedure is an engineering/accounting mechanism for
integer reconciliation.

It is not presented as an empirical demographic law.

## Calibration Status

004A7 introduces no demographic calibration coefficients.

The age-sex profile itself must eventually be one of:

- empirically sourced;
- scenario-defined;
- explicitly derived from authoritative demographic data.

004A7 does not provide a default population shape.

It therefore does not invent age or sex composition.

The largest-remainder procedure is deterministic integer accounting,
not a calibrated behavioral parameter.

## Causal Integration

Authoritative input:

    Pop::size or another authoritative population count

External/scenario input:

    DemographicAgeSexProfile

Transformation:

    proportional allocation
        -> integer floor
        -> deterministic largest-remainder reconciliation

Output:

    PopulationAgeSexStructure

Invariant:

    sum(age-sex structure) = authoritative population

Timing:

004A7 is an initialization/reconciliation operation.

It does not run as a daily demographic tick.

Downstream future consumers can include:

- fertility;
- age-specific mortality;
- nutritional-health mortality;
- disease;
- migration;
- labor-force availability;
- military manpower;
- education demand;
- pension demand.

Provenance fields expose:

- profile weight sum;
- population assigned by floor allocation;
- population assigned through remainder reconciliation;
- validity.

This makes demographic initialization inspectable.

## Why this does not create a second population ledger

`DemographicAgeSexProfile` contains weights, not people.

`PopulationAgeSexStructure` is materialized against an explicit
authoritative total.

The output is accepted as valid only when:

    cohort total = authoritative population

Therefore age-sex detail cannot silently create or destroy population.

When demographic state is eventually attached to live POPs, this
invariant must remain mandatory.

## Determinism

No floating-point arithmetic is required.

All proportional allocation uses integer arithmetic.

Identical:

- profile;
- authoritative population;

produce bit-identical cohort allocations.

Tied remainders use a fixed cell ordering.

This supports deterministic replay.

## Performance

004A7 performs initialization over exactly 36 demographic cells.

The algorithm is bounded and tiny compared with simulation runtime.

It does not add recurring cost to current POP ticks.

No new state is attached to every live POP.

## Scope Boundary

004A7 does not implement:

- real demographic datasets;
- synthetic default age distributions;
- population-data downloading;
- POP attachment;
- annual aging;
- fertility;
- births;
- mortality;
- deaths;
- life tables;
- migration;
- health-specific mortality;
- military manpower effects;
- education;
- pensions.

It establishes only the exact-total demographic initialization seam.
