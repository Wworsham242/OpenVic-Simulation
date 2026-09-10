# PROJECT-CONVERGENCE-004A9 - Province Demographic Authority

Research-Gate-Version: 1

## Native Repository Basis

OpenVic already represents socioeconomic population as individual POPs.

ProvinceInstance inherits PopsAggregate.

PopsAggregate contains authoritative province-level `total_population`.

ProvinceInstance::_update_pops() clears the aggregate and rebuilds it
from the actual live POPs.

Therefore province population total is already derived from native POP
authority and no second population-total ledger is required.

004A6 introduced PopulationAgeSexStructure.

004A7 introduced deterministic profile reconciliation for one POP-sized
population.

004A8 attached optional age-sex structure to an individual POP only when
an explicit profile is supplied.

## External Reference Review

U.S. Census national, state and county population estimates use the
cohort-component method.

The method begins with a geographic base population and applies births,
deaths and migration while preserving age-sex composition.

Current Census documentation states that national, state and county
estimates are produced by age and sex and that cohort-component
estimation starts from a base population.

UN World Population Prospects likewise uses evaluated age-sex base
populations for demographic estimation and projection.

International Futures represents demographic stocks by age and sex at
country/region level and advances them through fertility, mortality and
migration.

Census subnational methods also demonstrate that disaggregating a known
geographic age-sex distribution into finer cross-classifications
requires additional data or estimation methodology such as IPF.

Therefore occupation/culture/religion age profiles should not be
invented merely because a province age profile exists.

Defense and strategic simulation references were reviewed under the
mandatory framework. They do not provide a superior base-population
accounting model for this problem.

## Chosen Mechanism

004A9 establishes province-level age-sex composition as the preferred
primary demographic authority.

ProvinceInstance receives an optional:

    ProvinceDemographicAgeSexState

Initialization requires an explicit:

    DemographicAgeSexProfile

and reconciles it against native:

    ProvinceInstance::get_total_population()

The demographic state is therefore constrained by the population
already represented by actual OpenVic POPs.

No separate province population total is created.

### Hybrid relationship with POP demographics

Province demographic age-sex structure is the preferred base stock for
future:

- fertility;
- mortality;
- births;
- age-specific health;
- age dependency;
- education demand;
- pension demand;
- aggregate working-age population.

004A8 per-POP demographic state remains available when a downstream
mechanic genuinely requires joint detail such as:

    age x occupation
    age x culture
    age x religion

and defensible data or reconstruction exists.

Province age-sex structure is not automatically copied into every POP.

### Aggregate population width

Individual OpenVic POP size uses 32-bit pop_size_t.

Province total population uses 64-bit pop_sum_t.

004A9 therefore adds a province reconciliation path using the 64-bit
aggregate total.

Existing PopulationAgeSexStructure cells remain 32-bit.

Every materialized cell is explicitly checked against the cell limit.

A profile that would require any single age-sex cell to exceed the
existing representation is rejected rather than silently truncated.

## Calibration Status

004A9 introduces no behavioral coefficients.

Profile weights are external/scenario input.

Population total is derived from authoritative native POP state.

Largest-remainder allocation is an accounting rounding mechanism, not a
demographic empirical law.

No fertility, mortality or migration coefficients are introduced.

## Causal Integration

Native socioeconomic authority:

    live OpenVic POPs

Aggregation:

    ProvinceInstance::_update_pops()
        -> PopsAggregate::total_population

Explicit demographic input:

    DemographicAgeSexProfile

Reconciliation:

    64-bit province total
        + profile weights
        -> exact integer age-sex structure

Stored demographic output:

    ProvinceDemographicAgeSexState

Required invariant:

    sum(age-sex cohorts)
        == ProvinceInstance::get_total_population()

Future demographic chain:

    province age-sex stock
        + fertility schedules
        + mortality schedules
        + migration
        -> next province age-sex stock

The POP system remains the authoritative socioeconomic partition of that
same population.

Future integration must keep demographic and socioeconomic accounting
consistent whenever population actually changes.

## Scope Boundary

004A9 does not implement:

- demographic source files;
- Census/UN data import;
- scenario profile assignment;
- automatic initialization;
- IPF/raking;
- age x occupation reconstruction;
- age x culture reconstruction;
- age x religion reconstruction;
- aging;
- fertility;
- births;
- mortality;
- deaths;
- migration;
- labor-force participation;
- military manpower;
- education demand;
- pension demand.

It establishes only the geographic demographic authority seam and its
exact accounting relationship to native province population.
