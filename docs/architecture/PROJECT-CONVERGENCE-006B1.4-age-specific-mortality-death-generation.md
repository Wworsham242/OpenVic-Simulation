# PROJECT-CONVERGENCE-006B1.4 — Age-Specific Mortality Death Generation

**Program:** PROJECT-CONVERGENCE-006B1 — Causal Population Evolution

## Purpose

Generate explicit age/sex-specific deaths from an authoritative demographic
structure and an explicit annual mortality schedule.

This supplies the first native causal producer for the death component accepted
by 006B1.1.

## Repository Reconciliation

Repository reconnaissance found no existing general demographic mortality
producer.

The repository does contain a causal health chain:

    basic-needs failure
        -> survival-needs stress
        -> health-vulnerability pressure
        -> persistent nutrition-health burden

Those quantities explicitly are not mortality probabilities or death counts.

Existing architecture also states that credible mortality eventually requires:

- baseline age-specific mortality;
- nutritional condition;
- disease prevalence;
- sanitation;
- health-system access;
- conflict and other hazards where applicable.

Therefore B1.4 does not convert existing nutrition-health burden directly into
deaths.

## Empirical Method

Cohort-component population projection methods apply age- and sex-specific
mortality or survival probabilities to the population.

The U.S. Census International Database methodology states that population by
age and sex is exposed to estimated age-sex-specific chances of dying before
survivors advance to the next age.

United Nations World Population Prospects likewise uses survival probabilities
by sex and age in cohort-component projection.

References:

- U.S. Census Bureau International Database Methodology:
  https://www2.census.gov/programs-surveys/international-programs/technical-documentation/methodology/idb-methodology.pdf

- U.S. Census Bureau Population Projections:
  https://www.census.gov/programs-surveys/popproj/about.html

- United Nations World Population Prospects 2024 Methodology.

## Chosen Mechanism

`DemographicAgeSexMortalitySchedule` stores an annual probability of death for
each female/male age-band cell.

Probability is represented as:

    deaths per 1,000,000 exposed persons

with:

    0 <= probability <= 1,000,000

This provides substantially more precision than per-thousand rates while
remaining deterministic integer arithmetic.

For each demographic cell:

    expected_death_numerator =
        population
        * mortality_probability
        + prior_fractional_remainder

    deaths =
        floor(expected_death_numerator / 1,000,000)

    remainder =
        expected_death_numerator % 1,000,000

## Per-Cell Remainder Authority

Fractional mortality remainder is stored separately for every age/sex cell.

It must not be pooled across a province.

Pooling would permit fractional expected mortality generated in one cohort to
later materialize as a death in another cohort, destroying age/sex causal
identity.

## Causal Boundary

B1.4 consumes an already selected effective mortality schedule.

It does not yet calculate mortality effects from:

- nutritional-health burden;
- disease;
- sanitation;
- healthcare access;
- extreme temperature;
- pollution;
- conflict;
- disaster exposure.

Those mechanisms should later alter or compose into an effective age/sex
mortality schedule or excess-hazard layer.

The intended chain is:

    baseline mortality
        + calibrated excess mortality mechanisms
        -> effective age/sex mortality probability
        -> B1.4 deaths
        -> B1.1 accounting

rather than:

    food shortage
        -> direct deaths

## Time Boundary

Mortality probabilities in this increment represent annual demographic
exposure.

B1.4 does not define how a demographic year maps to `SimTime`.

## Scope Boundary

Not implemented here:

- health-to-mortality conversion;
- disease mortality;
- malnutrition mortality;
- conflict mortality;
- infant-specific mortality mechanics;
- life-table estimation;
- life-expectancy estimation;
- socioeconomic POP resizing;
- demographic scheduling.

## Acceptance

Tests prove:

1. zero mortality produces zero deaths;
2. mortality varies independently by age and sex;
3. fractional carry remains attached to its exact age/sex cell;
4. probability one cannot kill more than the cohort;
5. probability greater than one is rejected;
6. invalid fractional remainder is rejected;
7. generated deaths feed 006B1.1 directly;
8. nutrition-health burden is not implicitly consumed;
9. identical inputs are deterministic.

Completion requires successful build, focused tests, full CTest, research gate,
clean diff, commit, push, and exact local/remote HEAD agreement.