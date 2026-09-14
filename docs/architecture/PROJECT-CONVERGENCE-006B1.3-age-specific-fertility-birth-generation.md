# PROJECT-CONVERGENCE-006B1.3 — Age-Specific Fertility Birth Generation

**Program:** PROJECT-CONVERGENCE-006B1 — Causal Population Evolution

## Purpose

Generate explicit female and male birth counts from an authoritative
age-sex population structure and an explicit age-specific fertility-rate
schedule.

This is the first native causal producer for the birth component accepted by
006B1.1.

## Repository Reconciliation

Repository reconnaissance found no existing general fertility mechanism.

Inherited OpenVic/Victoria surfaces include:

- `BASE_POPGROWTH`;
- `pop_growth`;
- `global_population_growth`;
- `population_growth`;
- province life rating;
- population-change reporting fields.

Those are aggregate Victoria-style growth/modifier surfaces. They are not an
age-specific fertility authority and are not used by this mechanism.

The existing convergence architecture already requires age-specific fertility
rather than direct aggregate population growth.

## Empirical Method

U.S. Census and United Nations demographic methods define age-specific
fertility rates using births to women of a given age or age group divided by
the female population or person-years of exposure in that age group.

A conventional five-year ASFR schedule is often expressed as annual live births
per 1,000 females.

References:

- U.S. Census Bureau, 2023 National Population Projections methodology.
- U.S. Census Bureau International Database.
- United Nations World Fertility Data / demographic projection methodology.

The engine does not hard-code a 15-49 fertility window. Published systems vary
in age resolution and modeled childbearing range. A ruleset or dataset selects
which age bands receive nonzero rates.

## Chosen Mechanism

`DemographicAgeSpecificFertilitySchedule` stores:

    annual live births per 1,000 female person-years

for every represented demographic age band.

For each age band:

    expected_births_numerator =
        female_population
        * ASFR

The denominator is 1,000.

Each product is divided before aggregate accumulation, while fractional
remainders are preserved.

The returned fractional remainder may be supplied to the next demographic
interval so small populations do not permanently lose fractional expected
births through repeated truncation.

## Birth Sex Allocation

Birth sex composition is supplied explicitly as:

    male births per 1,000 female births

No default biological ratio is embedded in engine authority.

For total births B and supplied sex ratio R:

    male share =
        R / (R + 1000)

Male births are deterministically rounded to nearest integer.

Female births receive the remainder so:

    female_births + male_births = total_births

exactly.

## Causal Boundary

This increment consumes an already selected effective fertility schedule.

It does not yet determine how fertility changes due to:

- nutrition;
- survival stress;
- income;
- education;
- contraception;
- reproductive healthcare;
- policy;
- cultural norms;
- war;
- disease;
- maternal health.

Those belong in later calibrated fertility-rate producers.

The causal path is therefore:

    female age structure
        + explicit ASFR schedule
        + explicit sex ratio at birth
        -> generated births
        -> 006B1.1 birth component

rather than:

    generic population-growth modifier
        -> population appears

## Time Boundary

ASFR is annual demographic exposure.

B1.3 does not define how one demographic year maps to `SimTime` ticks.

Scheduling/calendar mapping remains a separate runtime concern.

## Scope Boundary

Not implemented here:

- fertility behavior;
- stress-to-fertility calibration;
- socioeconomic fertility modifiers;
- contraception models;
- partnership/household formation;
- pregnancy state;
- maternal mortality;
- infant mortality;
- socioeconomic POP resizing;
- demographic scheduler integration.

## Acceptance

Tests prove:

1. zero rates produce zero births;
2. one age-specific rate produces expected births;
3. male population is not fertility exposure;
4. multiple age-band contributions sum correctly;
5. fractional expected births carry deterministically;
6. explicit birth sex ratio preserves the total;
7. core does not hard-code a reproductive age window;
8. invalid starting population is rejected;
9. invalid fractional carry is rejected;
10. impossible birth totals are rejected;
11. generated births feed 006B1.1 directly;
12. identical inputs produce identical results.

Completion requires successful build, focused tests, full CTest, research gate,
clean diff, commit, push, and exact local/remote HEAD agreement.