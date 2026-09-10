# PROJECT-CONVERGENCE-004A11 - Demographic Source Registry

Research-Gate-Version: 1

## Native Repository Basis

004A9 establishes province-level age-sex demographic authority.

004A10 defines provenance-tagged demographic candidates and deterministic
candidate resolution.

The existing OpenVic codebase uses explicit loading and locking phases
for registries and history data.

province_index_t is the native typed province index.

004A11 therefore adds a province-indexed load-time demographic registry
rather than embedding source-file parsing directly into ProvinceInstance
or ProvinceHistoryEntry.

## External Reference Review

### United Nations World Population Prospects

UN World Population Prospects maintains source archives, demographic
databases and structured metadata used to produce estimates and
projections.

This supports preserving multiple source candidates and their provenance
until an explicit authority decision is made.

Reference:

https://www.un.org/development/desa/pd/sites/www.un.org.development.desa.pd/files/files/documents/2024/Jul/undesa_pd_2024_wpp2024_methodology-report_web.pdf

### WorldPop

WorldPop distinguishes census and official population control totals
from model-derived spatial disaggregation.

WorldPop also documents source datasets, modelling methods and
uncertainty.

This supports storing direct and model-derived demographic candidates as
separate source records rather than collapsing them during import.

References:

https://www.worldpop.org/methods/populations/

https://www.worldpop.org/focus_areas/population_mapping/

### Defense and strategic-model review

The mandatory defense and strategic modelling references were reviewed.

No defense-specific data registry mechanism is more appropriate for
demographic source authority.

No defense mechanism is imported.

## Chosen Mechanism

004A11 introduces:

    ProvinceDemographicAgeSexRegistry

The registry owns candidate inputs grouped by native province_index_t.

An importer supplies:

    ProvinceDemographicAgeSexImportRecord

containing:

    province_index
    ProvinceDemographicAgeSexInput

The registry does not know whether the record originated from:

- CSV;
- JSON;
- an API;
- a scenario file;
- a generated dataset;
- another importer.

File format is therefore not demographic authority.

### Load and lock lifecycle

The registry has two phases.

Import phase:

    unlocked
        -> add input candidates
        -> add import records

Resolution phase:

    lock()
        -> expose immutable spans
        -> resolve candidate
        -> initialize province state

Once locked, the registry refuses additional records.

This is necessary because 004A10 returns a pointer to the selected input
for provenance inspection.

Allowing candidate vectors to grow after resolution could invalidate
that pointer.

### Province addressing

The registry uses native province_index_t.

It allocates one candidate vector per known province index.

Invalid province indexes are rejected rather than silently creating a
new geographic identity.

### Candidate preservation

Multiple candidates for one province are retained.

The registry does not merge or deduplicate them.

This allows 004A10 to:

- prefer explicit authority;
- select newer reference dates;
- select newer revisions;
- detect exact precedence ambiguity.

An exact tie remains visible rather than being lost during ingestion.

## Calibration Status

004A11 introduces no demographic coefficients.

It introduces no source-quality scores.

It changes no authority-priority values.

It performs no interpolation or statistical reconstruction.

Registry ordering is storage organization only.

004A10 remains authoritative for demographic candidate selection.

## Causal Integration

External or scenario source
        |
        v
format-specific parser/importer
        |
        v
ProvinceDemographicAgeSexImportRecord
        |
        v
province_index_t
        |
        v
ProvinceDemographicAgeSexRegistry
        |
        +---- multiple candidates preserved
        |
        v
lock()
        |
        v
004A10 deterministic authority resolver
        |
        +---- NONE
        |
        +---- AMBIGUOUS
        |
        v
selected input + provenance
        |
        v
004A9 ProvinceDemographicAgeSexState
        |
        v
exact reconciliation to native province population

The registry creates no second province population ledger.

## Scope Boundary

004A11 does not implement:

- CSV parsing;
- JSON parsing;
- Census API access;
- UN API access;
- WorldPop downloads;
- file discovery;
- geographic-name matching;
- GIS boundary matching;
- parent-geography inheritance;
- IPF/raking;
- interpolation;
- extrapolation;
- population aging;
- fertility;
- births;
- mortality;
- deaths;
- migration.

The next increment may implement a concrete parser/import format that
produces ProvinceDemographicAgeSexImportRecord values without changing
the authority rules established by 004A9 through 004A11.
