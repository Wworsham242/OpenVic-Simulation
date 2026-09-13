# REFERENCE-MODEL-RECONCILIATION

**Status:** Mandatory architecture reference  
**Purpose:** define exactly what external models may influence and how their equations, coefficients and abstractions enter the engine.

## Core rule

External models are evidence and mechanism references. They are not alternate authorities over world state.

A reference may contribute:

- ontology suggestions;
- causal structure;
- equations;
- elasticities;
- estimated coefficients;
- lag structures;
- probability functions;
- convergence functions;
- calibration priors;
- validation targets;
- aggregation strategies;
- scenario ranges.

It may **not** bypass the authoritative state owners of this engine.

## International Futures (IFs)

### Why IFs matters

IFs is the principal reference for integrated civilian structural dynamics.

Its documented methodology is structure-based and agent-class driven. It combines:

- cohort-component demography;
- goods/services markets;
- stocks and flows;
- social-accounting and financial-flow structures;
- energy and agriculture partial-equilibrium systems;
- socio-political relationships;
- cross-domain feedback.

IFs is particularly useful because it contains both structural state and empirically estimated relationships rather than treating forecasting as trend extrapolation.

### What "use IFs as a weight" means

The engine will **not** create one `ifs_weight` scalar.

Instead, IFs-derived information enters as **domain-specific structural influences**.

Examples:

- fertility: nonlinear expected TFR relationships, contraception effects, income-distribution effects, convergence and exogenous trend terms;
- mortality: expected life-expectancy relationships, income-distribution effects, health-spending effects, conflict/starvation/disease modifiers;
- economy: production-function relationships, multifactor-productivity contributions, investment allocation and stock/price adjustment;
- energy: GDP-linked demand, energy-intensity transitions, price elasticities, reserve/capital constraints;
- agriculture: yield response to capital/labor/technology, saturation, price/stock response and demand relationships;
- governance/state failure: estimated probability relationships using development, institutions, education, openness and other supported predictors;
- interstate behavior: probabilistic threat/challenge formulations where documentation and data support them.

### Structural Influence Record

Every imported/adapted IFs relationship should be representable by metadata equivalent to:

```text
StructuralInfluence {
    id
    source_model
    source_equation_or_document
    domain
    target_quantity
    driver_quantities[]
    functional_form
    coefficients[]
    lags[]
    valid_geography
    valid_era
    calibration_class
    uncertainty
    bounds
    cadence
    version
}
```

This metadata does not itself mutate the world. The owning domain consumes it.

### Permitted roles

An IFs-derived relationship may act as:

1. **rate function** — e.g. expected fertility or mortality tendency;
2. **elasticity** — e.g. demand response to price/income;
3. **probability prior** — e.g. instability likelihood before current-game evidence;
4. **convergence target** — structural long-run tendency;
5. **behavioral response coefficient** — e.g. investment or consumption response;
6. **calibration prior** — initial coefficient before setting-specific calibration;
7. **validation envelope** — expected broad behavior against which scenarios are tested.

### Prohibited roles

An IFs-derived result may not:

- directly overwrite conserved inventories;
- directly force migration, coup, war, default or death because an aggregate score crossed a threshold;
- replace actor perception and decision-making;
- force an equilibrium inside a short operational tick;
- turn long-horizon regression relationships into universal laws;
- silently apply modern relationships to the Bronze Age or another unsupported setting;
- be described as empirical if it is only a scenario parameter or historical tuning value.

### Prior versus outcome

Where uncertainty exists:

```text
structural prior
+ current authoritative conditions
+ actor-perceived information
+ actor/institution decision
+ physical/economic constraints
+ stochastic process where justified
= realized outcome
```

The prior bends likelihood or expected rate. It does not dictate the realized event.

### IFs adjustment functions

IFs frequently uses stock discrepancy and change-in-stock terms to move demand, price, trade or investment toward desired states over time.

We may reuse this **control-system idea** where appropriate, but not the same coefficient universally.

For this engine:

- actual and desired stock must be explicit;
- first-order discrepancy and trend may be inputs;
- coefficients are domain-specific;
- adjustment must respect capacity and conservation;
- operational domains may use shorter native cadences than IFs.

### Historical initialization and convergence

IFs often begins from country-specific empirical conditions and allows structural relationships to influence change or convergence over time.

This is valuable for game initialization:

- preserve observed initial conditions;
- use structural expected-value relationships as tendency/pressure;
- allow scenario-specific persistence;
- gradually reduce unexplained initial residuals only where the domain evidence supports convergence.

Do not instantly snap a country/region to a regression line.

### IFs calibration and auto-tuning

The Pardee Center's public BIGPOPA/IFs auto-tuning tooling demonstrates that IFs parameters and coefficients can be searched over user-defined ranges against historical validation windows.

Our implication:

- coefficient sets should be data-driven and versioned;
- historical backtesting should be able to score candidate calibrations;
- tuning should optimize explicitly defined validation targets;
- tuned values remain labeled as calibrated, not physical constants;
- cross-validation/out-of-sample checks are required before promoting a coefficient set.

The public Pardee GitHub organization exposes helper/data/tuning repositories, but the full production IFs source is not presented there as a normal open-source repository. Documentation notes that adding/changing core variables requires IFs source code and a license. Therefore our work should use documented equations/methodology and openly available tuning/data tooling, not assume we can vendor the IFs engine.

## GCAM / GCIMS

Primary influence:

- energy transformation;
- agriculture/land use;
- water;
- emissions;
- climate-economy coupling;
- resource substitution and technology transition.

Allowed:
- subsystem decomposition;
- physical accounting;
- empirical functions;
- technology/resource constraints;
- long-run transition relationships.

Not allowed:
- importing coarse equilibrium timestep assumptions into operational gameplay;
- replacing our physical networks/inventories with annual aggregate balances where the game needs spatial causality.

## Demographic references

Primary:
- UN Population Division / WPP;
- U.S. Census cohort-component methods;
- HMD/HFD;
- IFs demographic implementation.

Use for:
- cohort aging;
- age-specific fertility;
- age/sex mortality;
- migration accounting;
- life tables;
- calibration and validation.

Demography should be one of the first domains to consume the structural-influence framework.

## Health references

Primary:
- WHO;
- IHME/GBD;
- IPC;
- FAO/WFP;
- epidemiological literature.

Use for:
- disease burden;
- nutrition;
- mortality hazards;
- treatment effects;
- healthcare capacity;
- exposure/vulnerability separation.

Do not convert deprivation directly into death without health/demographic intermediates.

## CMO / JTLS-GO / professional military simulation

Primary influence:
- operational causality;
- readiness;
- sensors;
- weapons employment;
- theater logistics;
- air/land/naval interaction;
- infrastructure and basing;
- command/mission constraints.

CMO is a detail reference, not a required object granularity. The engine should generally operate at the least granular level that preserves the strategic consequence.

## Historical/game references

Paradox-family games are references for:

- legibility;
- useful aggregation;
- pacing;
- data-driven content;
- mod flexibility.

They are not empirical authorities.

## DARPA World Modelers / causal-model work

Use for:

- typed causal composition;
- intervention analysis;
- provenance;
- sensitivity;
- model/data linkage.

The causal graph remains an explanation/analysis layer. Domain-native mechanics remain simulation authority.

## Calibration classes

Every coefficient must be tagged as one of:

- `EMPIRICAL_ESTIMATE`
- `DERIVED_FROM_AUTHORITATIVE_STATE`
- `SCENARIO_DEFINED`
- `REFERENCE_MODEL_CALIBRATION`
- `PROVISIONAL`
- `TUNED_HISTORICAL`

A coefficient may also carry uncertainty/range metadata.

## Implementation order

1. 006A2 establishes scale.
2. Define a small general `StructuralInfluence`/coefficient metadata contract using the existing bounded calculation seam where possible.
3. First consumers should be breadth domains with strong external models: demographic transition and finance/economy, not another military branch.
4. Add historical-validation tooling before promoting large sets of weights.
5. Use the same metadata system for IFs-, UN-, GCAM-, WHO- and literature-derived coefficients without giving any one source special mutation authority.

## Reference URLs

Official IFs documentation:
- https://www.ifs.du.edu/assets/help/understanding_the_model____opening_the_black_box_/understanding_the_modeling_approach/ifs_structure_elements_and_philosophy.htm
- https://ifs.du.edu/assets/help/WebHelp/understanding_the_model____opening_the_black_box_/understanding_the_equations/specialized_functions/adjustment_mechanism.htm
- https://ifs.du.edu/assets/help/understanding_the_model____opening_the_black_box_/demography_population/demographic_equations/demographic_fertility_distribution.htm
- https://ifs.du.edu/assets/help/understanding_the_model____opening_the_black_box_/demography_population/demographic_equations/demographic_mortality_distribution.htm
- https://www.ifs.du.edu/assets/help/understanding_the_model____opening_the_black_box_/economy/economic_equations/the_goods_and_services_market/economic_stocks_and_prices.htm
- https://www.ifs.du.edu/assets/help/understanding_the_model____opening_the_black_box_/energy/energy_equations/energy_equations_overview.htm
- https://www.ifs.du.edu/assets/help/understanding_the_model____opening_the_black_box_/agriculture/agricultural_equations/agricultural_equations_overview.htm
- https://www.ifs.du.edu/assets/help/WebHelp/understanding_the_model____opening_the_black_box_/socio_political/socio_political_equations/social_organization_stability.htm

Pardee public GitHub:
- https://github.com/PardeeCenterDU
- https://github.com/PardeeCenterDU/BIGPOPA
