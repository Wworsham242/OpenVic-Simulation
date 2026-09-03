# VERTICAL-004 â€” Fiscal capacity from country history

Status: fiscal collection capacity can now originate in scenario/mod data.

## Data path

VERTICAL-004 deliberately reuses the existing OpenVic country-history pipeline:

```text
country history file
      â†“
CountryHistoryEntry
      â†“
bookmark/date filtering
      â†“
CountryInstanceManager::apply_history_to_countries
      â†“
FiscalCollectionCapacity
      â†“
effective tax realization
```

No separate modern scenario loader is introduced.

## New optional history keys

```text
fiscal_tax_base_coverage
fiscal_compliance_rate
fiscal_collection_execution
```

All are optional fixed-point values.

Example:

```text
fiscal_tax_base_coverage = 0.82
fiscal_compliance_rate = 0.91
fiscal_collection_execution = 0.88
```

The capacity object clamps applied values to [0, 1].

## Dated inheritance

Country history already applies entries in date order up to the loaded bookmark date.

Fiscal history follows those same semantics:

- missing key = preserve the previous/default value;
- present key = replace that one dimension;
- later dated entries may override any subset.

This allows historical/scenario transitions without resetting unrelated dimensions.

## Compatibility

The base `FiscalCollectionCapacity` defaults all dimensions to 1.0.

Therefore countries without the new history keys retain VERTICAL-003's legacy-neutral behavior.

## Why country history instead of CountryDefinition

Fiscal capacity is mutable scenario state, not permanent country identity.

Putting it in `CountryDefinition` would incorrectly imply that a country's collection capacity is
timeless metadata. Country history is the more appropriate existing OpenVic mechanism.

## Polity identity note

VERTICAL-004 does not yet generalize the inherited country TAG itself into the final sovereign-polity
definition. That identity migration should be done only when a concrete ruleset/bootstrap need requires
it, rather than adding another parallel registry prematurely.

## Next

With one state-capacity mechanism now live and data-driven, the nation/state-capacity skeleton has
enough substance to pause.

The recommended next step is the economy/production/market vertical audit:

- preserve useful OpenVic market and production mechanisms;
- identify Victoria-specific assumptions;
- define modern industry-level resolution;
- connect fiscal revenue to real economic activity without creating per-firm simulation by default.