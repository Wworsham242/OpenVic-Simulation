# PROJECT-CONVERGENCE-004A10 - Demographic Input Authority

Research-Gate-Version: 1

## Native Repository Basis

OpenVic ProvinceHistoryEntry already represents dated province history.

Its current province-history fields include ownership, control, colonial
status, slavery, cores, RGO, life rating, terrain, buildings, party
loyalty and POP records.

POP history supplies socioeconomic POP state but no authoritative
age-sex demographic source metadata.

004A9 established ProvinceDemographicAgeSexState as the preferred
province-level age-sex authority and reconciles an explicit profile
against native province population.

004A10 does not modify the existing Victoria history parser.

Instead it establishes the typed input semantics that a later parser,
data importer or scenario-definition layer must produce.

## External Reference Review

### U.S. Census Population Estimates

The Census Population Estimates Program distinguishes the population
reference date from the vintage/version of an estimate.

New vintages revise the time series using newer input data and
methodology, and current vintages supersede older vintages.

Census also distinguishes estimates from projections: estimates
generally describe past/current populations using observed component
data, whereas projections require assumptions about future demographic
processes.

References:

https://www.census.gov/programs-surveys/popest/about/faq.html

https://www.census.gov/programs-surveys/popest/data/data-sets.html

https://www.census.gov/newsroom/blogs/random-samplings/2026/01/updates-data-methodology-population-estimates.html

### United Nations World Population Prospects

UN WPP maintains archived input data, demographic databases and
structured metadata used for analysis, modelling and public
documentation.

This supports retaining source and revision identity rather than
discarding provenance once a demographic profile enters the simulation.

Reference:

https://www.un.org/development/desa/pd/sites/www.un.org.development.desa.pd/files/files/documents/2024/Jul/undesa_pd_2024_wpp2024_methodology-report_web.pdf

### WorldPop

WorldPop distinguishes administrative census/official population inputs
from modelled spatial disaggregation.

Its methods emphasize documenting input datasets, modelling methods and
accuracy/uncertainty.

That distinction is important because a direct province observation and
a profile derived from a parent geography are not epistemically the
same input even if their final age-sex vectors look identical.

References:

https://www.worldpop.org/methods/populations/

https://www.worldpop.org/focus_areas/population_mapping/

### Defense and strategic-model review

The mandatory defense/strategic reference set was reviewed.

No defense-specific model provides a preferable authority-selection
scheme for demographic source metadata.

No defense mechanism is imported.

## Chosen Mechanism

004A10 introduces ProvinceDemographicAgeSexInput.

Each candidate contains:

- an explicit DemographicAgeSexProfile;
- source kind;
- geographic basis;
- demographic reference date;
- dataset revision/vintage date;
- explicit authority priority;
- owned source identifier.

### Source kind

Source kinds are descriptive provenance:

- observed enumeration;
- official estimate;
- projection;
- scenario;
- model-derived.

Source kind does not silently determine precedence.

A scenario may intentionally declare a scenario-specific profile more
authoritative than an observational source for that simulation.

### Geographic basis

Geographic basis records whether the profile is:

- directly province-specific;
- explicitly geographically disaggregated;
- an explicit parent-geography proxy.

No parent-geography profile is inherited automatically.

If a parent or reconstructed profile is to be considered, the data
layer must create it as an explicit candidate.

### Selection rule

Eligible candidates must:

- have complete provenance metadata;
- have a non-empty source identifier;
- have reference_date less than or equal to the target initialization
  date.

Candidate precedence is:

1. greater explicit authority priority;
2. later reference date;
3. later revision/vintage date.

If candidates remain tied after all three dimensions, resolution is
AMBIGUOUS.

The resolver deliberately does not use:

- source-kind enum order;
- geographic-basis enum order;
- source identifier lexical order;
- insertion order;
- pointer address.

Those would be hidden policy decisions rather than demographic
methodology.

### Future-dated profiles

A profile whose demographic reference date is later than the target
initialization date is not silently back-cast.

A future projection can be used when the scenario target date reaches
that reference date.

Historical reconstruction from a later-produced vintage remains
possible because revision_date is metadata and is not treated as the
demographic reference date.

### Application

A selected input can initialize the existing 004A9
ProvinceDemographicAgeSexState.

Materialization remains subject to all 004A9 population accounting and
cell-width invariants.

Ambiguous or absent input leaves demographic authority uninitialized.

## Calibration Status

004A10 introduces no demographic coefficients and no statistical
confidence score.

Authority priority is explicit scenario/data policy, not an empirically
estimated probability.

Source kind and geographic basis are provenance classifications rather
than quality weights.

No arbitrary confidence percentage is synthesized.

## Causal Integration

Input sources:

    Census / statistical office / UN / WorldPop
    scenario data
    explicit model-derived data
        |
        v
ProvinceDemographicAgeSexInput candidates
        |
        v
metadata validation
        |
        v
explicit authority priority
reference date
revision date
        |
        v
deterministic resolution
        |
        +---- no candidate ----> remain uninitialized
        |
        +---- tie -----------> AMBIGUOUS / refuse to guess
        |
        v
selected profile + provenance
        |
        v
004A9 ProvinceDemographicAgeSexState
        |
        v
exact reconciliation to native province population

The selected input pointer is returned to the caller so the eventual
demographic data registry can retain and expose the provenance that
produced the initialized state.

## Scope Boundary

004A10 does not implement:

- Census API downloading;
- UN API downloading;
- WorldPop downloading;
- CSV parsing;
- JSON parsing;
- new Victoria history syntax;
- source registry persistence;
- automated geographic matching;
- automatic parent-geography inheritance;
- IPF/raking;
- demographic interpolation;
- demographic extrapolation;
- population aging;
- fertility;
- births;
- mortality;
- deaths;
- migration;
- data-confidence inference.

The next parser/import increment may consume external or scenario data
only after producing the typed inputs and explicit metadata required by
this contract.
