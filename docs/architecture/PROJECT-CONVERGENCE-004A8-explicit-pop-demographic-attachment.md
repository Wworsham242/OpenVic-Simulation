# PROJECT-CONVERGENCE-004A8 - Explicit POP Demographic Attachment

Research-Gate-Version: 1

## Native Repository Basis

OpenVic historical POP data currently supplies POP type, culture,
religion, size, militancy, consciousness and rebel type.

It contains no authoritative age-sex cross-tab.

`Pop::size` remains the authoritative total.

004A6 introduced `PopulationAgeSexStructure`.

004A7 introduced explicit profile-to-total reconciliation.

## External Reference Review

Census and UN cohort-component methodology require a defensible base
age-sex population before demographic projection.

Census subnational estimation and ACS weighting methodology use
cross-classified controls and raking/IPF where appropriate.

A provincial age-sex pyramid alone does not identify age structure by
OpenVic occupation, culture and religion.

Therefore 004A8 does not manufacture those subgroup distributions.

References:

- U.S. Census cohort-component methodology
- U.S. Census subnational population estimation methodology
- ACS weighting methodology
- UN World Population Prospects methodology
- International Futures population model

Defense references including World Modelers, Causal Exploration and
JTLS-GO were reviewed but do not provide a relevant mechanism for this
specific demographic attachment problem.

## Chosen Mechanism

A live POP gains an optional:

    PopDemographicAgeSexState

The state is absent by default.

Initialization requires an explicit:

    DemographicAgeSexProfile

004A7 materializes that profile against authoritative:

    Pop::size

The resulting structure must sum exactly to the POP total.

Initialization is one-shot so existing authoritative demographic state
cannot be silently replaced.

The 36-cell structure is heap-backed and allocated only when activated.

## Calibration Status

004A8 introduces no behavioral or demographic coefficients.

Future profiles must be identified as empirical, scenario-defined,
derived from documented controls, or explicitly provisional.

No synthetic fallback is provided.

## Causal Integration

Input:

    Pop::size
    explicit DemographicAgeSexProfile

Transformation:

    004A7 deterministic exact-total reconciliation

Stored output:

    PopulationAgeSexStructure

Invariant:

    age-sex total == Pop::size

Legacy POPs:

    no profile
        -> no structure
        -> no behavioral change

Future consumers include fertility, mortality, health, labor,
military manpower, education, pensions and migration.

## Scope Boundary

004A8 does not implement:

- demographic data files;
- automatic age profiles;
- province-to-POP demographic inference;
- raking/IPF;
- population-wide activation;
- aging;
- fertility;
- births;
- mortality;
- deaths;
- migration;
- health mortality;
- military manpower.

It establishes only the explicit live-POP demographic attachment seam.
