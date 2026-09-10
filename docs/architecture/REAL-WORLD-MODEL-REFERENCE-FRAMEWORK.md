# Real-World Model Reference Framework

## Status

Mandatory architecture reference for simulation design and review.

All contributors and AI coding agents should read this document before
designing or implementing substantive simulation mechanics.

## Core rule

Every new mechanic is evaluated as a reconciliation of:

    existing OpenVic
        +
    empirical / scientific model
        +
    defense / strategic model where relevant
        +
    useful game abstraction
        =
    narrow native implementation

No single reference controls the architecture.

## Evaluation questions

Before implementation answer:

1. What authoritative state already exists?
2. What is the real-world causal mechanism?
3. How is that mechanism measured or modeled professionally?
4. What established simulation systems model the same problem?
5. What do games abstract usefully?
6. What information can be discarded without destroying causality?
7. What state must persist because it contains history?
8. What can remain derived?
9. What are the correct units and timescales?
10. How will the result connect causally to other domains?
11. How will we explain why an outcome occurred?
12. Which coefficients are empirical and which are provisional?

## Reference classes

### Game references

Use primarily for:

- interface;
- abstraction;
- player legibility;
- pacing;
- useful aggregation.

Examples:

- Victoria-series socioeconomic simulation;
- EU / Project Caesar spatial and logistical mechanics;
- Hearts of Iron IV production and equipment systems;
- Command: Modern Operations operational military detail.

Do not assume game formulas represent reality.

### Integrated scientific and policy models

#### GCAM / GCIMS

Primary use:

- energy;
- water;
- agriculture;
- land use;
- emissions;
- climate-economy interaction;
- resource transitions;
- cross-domain model coupling.

Related systems worth examining include:

- Xanthos — hydrology;
- Hector — climate;
- Moirai — land harmonization;
- Demeter — land-use disaggregation;
- Tethys — water demand;
- Persephone — crop yield change;
- Ambrosia — food demand.

Use the subsystem decomposition and empirical methods.

Do not import GCAM's long timestep equilibrium assumptions into
short-timescale operational simulation.

#### International Futures (IFs)

Primary use:

- demographics;
- economics;
- agriculture;
- health;
- education;
- energy;
- governance;
- infrastructure;
- long-horizon structural interactions.

Particularly useful as a comparison for global integrated causal
architecture.

#### World3 / system dynamics

Primary use:

- feedback-loop reasoning;
- stocks and flows;
- delays;
- resource constraints;
- sensitivity to accumulated state.

Do not use World3's extreme global aggregation as the world simulation
runtime.

### Demography

Primary references:

- United Nations Population Division;
- World Population Prospects;
- U.S. Census Bureau cohort-component methodology;
- Human Mortality Database;
- Human Fertility Database where appropriate.

Preferred concepts:

- age-sex cohorts;
- age-specific fertility;
- age-specific mortality;
- survival schedules;
- cohort aging;
- migration components;
- life tables.

### Health and food security

Primary references:

- WHO;
- IPC;
- FAO;
- WFP;
- IHME / Global Burden of Disease where appropriate.

Maintain distinctions between:

- exposure;
- vulnerability;
- nutritional condition;
- disease;
- morbidity;
- mortality.

Do not directly translate food shortage into deaths without the
intermediate health/demographic mechanism.

### Climate, environment and hydrology

Reference:

- climate reanalysis and weather methodology;
- hydrological models;
- drought indices;
- watershed/reservoir models;
- crop-water response;
- disaster and hazard models.

Preserve distinction between:

- climate baseline/distribution;
- realized weather;
- hydrology;
- environmental state;
- economic/social consequences.

### Agent-based modeling

References:

- Mesa;
- NetLogo;
- Repast.

Use for:

- heterogeneous actor behavior;
- emergent interactions;
- localized decisions;
- experimental prototypes.

Do not default to one citizen = one software agent.

Use coarse autonomous actors where sovereign or systemic agency matters.

### System dynamics methodology

References:

- Vensim;
- Stella / iThink.

Adopt useful disciplines:

- explicit stocks;
- explicit flows;
- delays;
- units/dimensional consistency;
- causal tracing;
- sensitivity analysis;
- calibration;
- scenario comparison.

Do not make one stock-flow graph the entire authoritative simulation.

### DARPA World Modelers / Causemos

Primary use:

- cross-domain causal integration;
- causal model composition;
- provenance;
- scenario analysis;
- intervention analysis;
- sensitivity analysis;
- model/data linkage.

Project interpretation:

    native simulation mechanics
        -> typed causal interfaces
        -> authoritative world evolution
        -> derived causal/provenance view

The causal graph is an analysis and explanation layer, not universal
simulation physics.

### DARPA Causal Exploration

Primary use:

- instability;
- insurgency;
- hybrid conflict;
- population-security interactions;
- territorial and political grievances;
- conflict feedback.

Conflict must eventually depend on multiple interacting drivers rather
than one national unrest scalar.

### Expected-utility / political decision models

Examples include the model family associated with Policon and
Senturion-style political forecasting.

Primary use:

- bargaining;
- coalition formation;
- legislative politics;
- diplomacy;
- sanctions;
- elite alignment;
- coups;
- negotiations.

Useful actor dimensions include:

- preferred position;
- salience;
- influence;
- leverage;
- resolve;
- coalition possibilities.

### JTLS-GO and theater-level defense simulation

Primary use:

- operational/theater abstraction;
- air/land/naval interaction;
- logistics;
- readiness;
- intelligence;
- civil-military interaction;
- humanitarian and infrastructure effects.

Use as a military abstraction reference between highly aggregated
grand-strategy games and platform-level tactical simulators.

### RAND-style wargaming

Primary use:

- scenario validation;
- decision behavior;
- adversarial response;
- identifying assumptions that purely quantitative models miss.

Quantitative mechanics and qualitative actor choices should inform one
another without letting subjective adjudication replace the simulation.

## Domain reference matrix

### Economy and finance

Inspect:

- OpenVic native economy;
- national accounting;
- input-output methodology;
- stock-flow consistent accounting where useful;
- market microstructure where relevant;
- IFs;
- GCAM where resource/economy interaction matters.

### Population and demographics

Inspect:

- OpenVic POP model;
- UN;
- Census;
- HMD/HFD;
- IFs.

### Agriculture

Inspect:

- OpenVic RGO/market system;
- FAO;
- crop science;
- GCAM;
- IFs agriculture;
- hydrology/water models.

### Health

Inspect:

- OpenVic population state;
- WHO;
- IPC;
- IHME;
- epidemiological literature.

### Migration

Inspect:

- OpenVic migration state;
- UN/IOM;
- gravity/random-utility models;
- migration-network literature;
- IFs where useful.

### Politics and governance

Inspect:

- OpenVic politics;
- comparative-politics empirical literature;
- institutional/state-capacity research;
- expected-utility actor models;
- Causal Exploration;
- IFs governance.

### Conflict and instability

Inspect:

- OpenVic militancy/rebel state;
- conflict-onset research;
- DARPA Causal Exploration;
- World Modelers;
- defense and political-science literature.

### Military

Inspect:

- OpenVic military systems;
- JTLS-GO concepts;
- operational-research literature;
- Command where platform/sensor detail is relevant;
- HOI where industrial/equipment abstraction is useful.

### Logistics and infrastructure

Inspect:

- OpenVic infrastructure/logistics;
- network-flow models;
- queueing/capacity models;
- transport engineering;
- JTLS-GO where military logistics applies.

### Climate/environment/water

Inspect:

- current OpenVic environmental additions;
- GCAM/GCIMS;
- Xanthos-like hydrology;
- climate/weather literature;
- crop and disaster models.

### Intelligence/information

Inspect:

- intelligence-cycle literature;
- sensor/probability models;
- JTLS/operational concepts;
- causal provenance;
- Bayesian or probabilistic inference where justified.

## Architecture constraints

### No duplicate authoritative ledgers

Reuse native authority whenever possible.

A new state variable is justified when it represents genuinely missing
state, especially when that state contains history.

### Stored versus derived

Store state when future behavior depends on its past.

Derive state when it is completely determined by current authoritative
state and has no independent history.

### No universal framework

Leontief, SFC, ABM, system dynamics, causal graphs and network flow are
tools.

None is the universal engine.

Each domain uses the mechanism appropriate to that domain.

### Causal provenance

Important state transitions should expose enough causal information to
reconstruct:

    what changed
    how much
    because of what
    through which intermediate mechanisms

This serves both debugging and player explanation.

## Calibration policy

Every parameter must be labeled:

### Empirical

Directly supported by a cited empirical source or dataset.

### Derived

Calculated from authoritative scenario data.

### Scenario-defined

Intentionally supplied by scenario/configuration data.

### Provisional calibration

Chosen for simulation behavior pending empirical calibration.

Provisional values must never be described as empirical constants.

## Research requirement

Before substantive implementation, search current authoritative sources
when the mechanism depends on external knowledge.

Do not rely solely on remembered descriptions of outside models.

Prefer primary technical documentation, peer-reviewed literature,
government/institutional methodology, or source code.

## Required architecture-document sections

New substantive PROJECT-CONVERGENCE documents should include:

## Native Repository Basis

What exists before this increment.

## External Reference Review

What real-world/scientific/defense/game references were considered and
what was learned from them.

## Chosen Mechanism

The exact mechanism selected and why.

## Calibration Status

Which values are empirical, derived, scenario-defined, or provisional.

## Causal Integration

Inputs, outputs, timing, downstream consumers and provenance.

## Scope Boundary

What is explicitly not implemented.

## Completion standard

A mechanic is not complete merely because it compiles.

Certification requires as applicable:

- targeted tests;
- predecessor regression tests;
- full CTest;
- diff/static review;
- architecture review;
- commit;
- push;
- exact remote HEAD verification.
