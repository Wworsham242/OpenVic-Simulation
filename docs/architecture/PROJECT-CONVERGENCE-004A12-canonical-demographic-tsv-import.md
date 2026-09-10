# PROJECT-CONVERGENCE-004A12 - Canonical Demographic TSV Import

Research-Gate-Version: 1

## Native Repository Basis

004A9 establishes province demographic age-sex authority.

004A10 defines provenance-bearing demographic input candidates and
deterministic authority resolution.

004A11 provides the province-indexed load-lock-resolve registry and
defines ProvinceDemographicAgeSexImportRecord as the importer boundary.

004A12 adds the first concrete format parser producing that existing
import record.

The parser does not write ProvinceInstance directly and does not alter
004A9 through 004A11 authority rules.

## External Reference Review

U.S. Census Population Estimates publishes age-sex datasets together
with explicit file layouts identifying variables and sort order.

Vintage releases distinguish demographic reference periods from the
release/vintage of the underlying estimate series.

Current Census age-sex products include both grouped and single-year age
data.

References:

https://www.census.gov/programs-surveys/popest/technical-documentation/file-layouts.html

https://www.census.gov/newsroom/press-kits/2026/vintage-2025-pop-estimates.html

WorldPop publishes age-sex structures with accompanying administrative
geography metadata and distinguishes the geographic level of source
data.

Reference:

https://hub.worldpop.org/geodata/summary?id=1276

Provider formats differ substantially.

Therefore 004A12 defines an engine-owned canonical interchange format
rather than making any provider schema part of simulation authority.

Defense and strategic modelling references were reviewed under the
mandatory research framework. No defense-specific file representation
is preferable for demographic source ingestion.

## Chosen Mechanism

004A12 defines a strict UTF-8 tab-separated row format.

TSV is chosen rather than general CSV so the core parser does not need
provider-specific quoting, comma escaping, locale handling or spreadsheet
dialects.

Each row contains exactly 43 fields.

Fields 1 through 7 are:

1. province_index
2. source_kind
3. geography_basis
4. reference_date
5. revision_date
6. authority_priority
7. source_identifier

The remaining 36 fields are age-sex relative weights.

They use the exact 004A6 ordering:

0-4 female
0-4 male
5-9 female
5-9 male
...
80-84 female
80-84 male
85+ female
85+ male

Weights are uint32 relative weights, not population counts.

### source_kind tokens

Accepted values are:

observed_enumeration
official_estimate
projection
scenario
model_derived

### geography_basis tokens

Accepted values are:

direct_province
explicit_geographic_disaggregation
explicit_parent_geography_proxy

### Dates

Dates use:

YYYY-MM-DD

The parser validates the date before constructing OpenVic Date.

Invalid dates are rejected rather than allowing Date's clamping behavior
to silently convert malformed source data.

### Province identity

province_index is parsed as native province_index_t.

004A12 checks representability.

004A11 remains responsible for checking whether that index exists in the
particular world registry.

### Provenance

source_identifier must be non-empty.

Tabs and newlines are structural delimiters and therefore are not valid
inside an identifier in this canonical format.

Provider adapters are responsible for producing a safe canonical source
identifier.

### Profile validity

All 36 weights must be valid uint32 values.

An all-zero profile is rejected during import because it carries no
demographic shape.

The weights remain relative composition only.

They do not create an independent population-total ledger.

## Calibration Status

004A12 introduces no demographic coefficients.

It does not rescale source data statistically.

It does not infer source quality.

It does not choose authority.

Parsing is representation conversion only.

004A10 remains authoritative for candidate selection.

004A9 remains authoritative for exact reconciliation to native province
population.

## Causal Integration

provider dataset
    |
    v
provider-specific adapter
    |
    v
canonical TSV row
    |
    v
004A12 parser
    |
    v
ProvinceDemographicAgeSexImportRecord
    |
    v
004A11 registry
    |
    v
lock
    |
    v
004A10 authority resolution
    |
    v
004A9 population reconciliation
    |
    v
province demographic age-sex authority

Malformed records fail before entering the registry.

No malformed date, enum token, integer overflow, empty provenance
identifier or empty age-sex shape is silently repaired.

## Scope Boundary

004A12 does not implement:

- filesystem traversal;
- whole-file streaming;
- header parsing;
- Census-specific parsing;
- UN-specific parsing;
- WorldPop-specific parsing;
- API downloading;
- spreadsheet parsing;
- geographic-name matching;
- GIS boundary reconciliation;
- automatic authority assignment;
- interpolation;
- extrapolation;
- IPF/raking;
- aging;
- fertility;
- births;
- mortality;
- deaths;
- migration.

The next increment may add whole-file ingestion and reporting around
this row parser or add a first provider adapter without changing the
canonical import contract.
