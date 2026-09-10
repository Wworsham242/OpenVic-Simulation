# PROJECT-CONVERGENCE-004A6 - Age-Sex Cohort Foundation

## Purpose

004A6 establishes the demographic population structure required before
the simulation can credibly implement:

- fertility;
- mortality;
- life expectancy;
- maternal risk;
- age-specific health effects;
- migration propensity;
- military-age manpower;
- school-age population;
- retirement-age population.

It does not yet change live POP population.

## Empirical method

Modern official population projections commonly use the
cohort-component method.

The basic population identity is:

    P_(t+1)
        =
        P_t
        + births
        - deaths
        + immigration
        - emigration

The important feature is that these components are evaluated separately
for population cohorts.

The U.S. Census Bureau describes cohort-component projection as
advancing cohorts through time using age-specific survival, adding new
birth cohorts from fertility rates, and applying migration separately.

Five-year age groups are valid for cohort-component implementation,
although more detailed systems can use single-year ages.

References:

- U.S. Census Bureau, Population Projections:
  https://www.census.gov/programs-surveys/popproj/about.html

- U.S. Census Bureau International Database Methodology:
  https://www2.census.gov/programs-surveys/international-programs/technical-documentation/methodology/idb-methodology.pdf

## Representation

004A6 uses 18 demographic age groups:

    0-4
    5-9
    10-14
    15-19
    20-24
    25-29
    30-34
    35-39
    40-44
    45-49
    50-54
    55-59
    60-64
    65-69
    70-74
    75-79
    80-84
    85+

Each group contains:

    female population
    male population

This yields 36 demographic cells.

The 85+ cohort is open-ended.

## Why five-year cohorts

Five-year cohorts provide enough resolution for:

- age-specific fertility schedules;
- mortality schedules;
- child health;
- working-age population;
- military-age manpower;
- education demand;
- retirement;
- age-sensitive migration.

They are substantially smaller than storing every single year of age for
every OpenVic POP.

The design can later be refined if single-year ages are needed in
specific high-fidelity subsystems.

## Demographic sex axis

The female/male distinction in this structure is a demographic
accounting axis for fertility and mortality calculations.

It is not intended to encode the full social concept of gender identity.

Any future social or political gender mechanics should remain a
separate representation layer.

## Accounting invariants

Every demographic structure must satisfy:

    female_count >= 0
    male_count >= 0

for every cohort.

For a demographic structure attached to an authoritative POP:

    sum(all cohort counts) = Pop::size

004A6 provides explicit validation for that equality.

This prevents a future demographic subsystem from becoming a second,
divergent population ledger.

## Why the structure is not attached to Pop yet

OpenVic currently contains authoritative POP size but no authoritative
age-sex distribution.

Automatically generating an age structure would invent demographic
facts.

That would contaminate future:

- birth estimates;
- death estimates;
- military manpower;
- migration behavior;
- school population;
- pension population.

004A6 therefore establishes the type and invariants without assigning
synthetic age distributions to existing POPs.

A later initialization increment must load or derive base-year age-sex
data from an explicitly documented demographic source or scenario
definition.

Only then should the structure become live authoritative state.

## Why aging is not implemented yet

A five-year cohort does not by itself specify the exact within-band age
distribution.

Advancing one-fifth of every cohort each year is a possible
approximation, but it is still an assumption.

Before implementing aging, the engine should explicitly choose:

- demographic update cadence;
- within-band aging treatment;
- integer rounding/remainder handling;
- terminal 85+ behavior;
- ordering relative to births, deaths, and migration.

004A6 therefore avoids silently embedding those assumptions.

## Future cohort-component sequence

A later demographic tick should have an explicit order such as:

    starting age-sex structure
        -> mortality/survival
        -> aging
        -> births
        -> migration
        -> ending age-sex structure

The exact ordering and exposure convention must be documented when that
increment is implemented.

## Relationship to 004A5

004A5 established:

    nutritional health burden

but did not implement mortality because mortality risk depends strongly
on age.

004A6 provides the structural age-sex dimension needed for a later
health/mortality bridge:

    nutrition burden
        + disease burden
        + age/sex cohort
        + WASH
        + health-system access
        -> age-specific excess mortality risk

This keeps exposure, health state, and demographic outcome separate.

## Performance

004A6 deliberately does not add 36 counters to every live POP yet.

The final storage strategy should be chosen when real demographic
initialization is implemented.

Possible future strategies include:

- direct per-POP cohort storage;
- pooled demographic profiles;
- sparse per-POP deviations from a province profile;
- province-level demographic structures with subgroup overlays.

The choice should be benchmarked against real scenario POP counts before
being made authoritative.

## Scope boundary

004A6 does not implement:

- synthetic age distributions;
- annual aging;
- births;
- fertility rates;
- deaths;
- mortality rates;
- life tables;
- life expectancy;
- migration;
- disease;
- military manpower effects;
- school enrollment;
- pensions.

It supplies the typed demographic foundation those later mechanics
require.
