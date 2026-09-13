# CROSS-DOMAIN-CERTIFICATION-SCENARIOS

**Status:** Permanent architecture kill-test catalog

## Rule

These scenarios certify that ordinary engine mechanisms compose.

They do **not** specify a correct historical outcome.

A test fails architecture if the desired causal chain can only be produced by a scenario-specific script that directly sets the downstream result.

Each scenario should record:
- initial authoritative state;
- enabled optional mechanisms;
- injected intervention/shock;
- observations available to each actor;
- domain mechanisms exercised;
- conserved quantities checked;
- causal provenance expected;
- performance envelope;
- deterministic replay result;
- broad invariants, not predetermined winner/outcome.

## 1. Bronze Age environmental stress / migration / political breakdown

Shock examples:
- multi-year precipitation shortfall;
- regional harvest losses;
- maritime trade insecurity.

Mechanisms required:
- environment/weather/hydrology input;
- agricultural physical output;
- food storage/redistribution/trade;
- nutrition/health;
- demographic movement;
- labor/tribute;
- authority/institutional response;
- military manpower/sustainment;
- trade-network disruption.

Pass condition:
The engine can produce resilience, migration, political adaptation or collapse depending on state and choices without requiring modern banking, electricity, aviation, cyber or modern legislature mechanics.

## 2. Iran / Hormuz regional crisis

Shock examples:
- attacks on shipping;
- strikes on military/infrastructure targets;
- threats/closure pressure in the Strait of Hormuz.

Mechanisms required:
- operational military actions;
- imperfect information;
- shipping route risk/capacity;
- energy inventories/production/trade;
- insurance/commercial-routing behavior when implemented;
- alliance/basing decisions;
- sanctions/financial mechanisms when implemented;
- public finance/political response;
- escalation as derived state, not authoritative scalar.

## 3. Taiwan blockade / invasion

Mechanisms required:
- maritime/air/missile operations at operational abstraction;
- basing and logistics;
- merchant shipping/rerouting;
- Taiwan energy reserves and electricity;
- semiconductor/systemic production dependence;
- alliance decisions;
- economic/trade propagation;
- perceived state/intelligence;
- escalation management.

No requirement to simulate every aircraft as an independent authoritative strategic object.

## 4. Russia–Ukraine high-intensity war

Mechanisms required:
- manpower/mobilization;
- equipment production and replacement;
- artillery/ammunition/sustainment;
- drones/EW/air defense abstractions;
- infrastructure damage/repair;
- foreign assistance;
- industrial adaptation;
- casualties/demography;
- fiscal/economic burden;
- operational adaptation.

## 5. Russia–NATO escalation

Mechanisms required:
- alliance institutions;
- force generation/readiness;
- reinforcement and basing;
- air/missile defense;
- maritime/undersea infrastructure;
- cyber/space effects when enabled;
- strategic warning;
- nuclear escalation logic as optional strategic mechanism;
- political cohesion and imperfect information.

## 6. Global financial crisis

Initial conditions:
- leveraged banking/financial system;
- interconnected claims;
- collateral-sensitive lending;
- maturity/liquidity mismatch.

Shock examples:
- asset repricing;
- major counterparty default;
- funding freeze.

Mechanisms required:
- balance sheets;
- deposits/loans/securities/claims;
- collateral;
- settlement/liquidity;
- default;
- inter-institution exposure;
- central-bank response;
- credit contraction into real investment/employment/consumption;
- fiscal response.

A direct “financial crisis = GDP -X%” modifier fails.

## 7. Pandemic

Mechanisms required:
- transmission;
- age/health vulnerability;
- healthcare capacity;
- morbidity/mortality;
- labor availability;
- voluntary behavior;
- government interventions;
- fiscal/education effects;
- trade/logistics effects;
- information/perception.

## 8. Austerity / public health

Intervention:
- large fiscal consolidation via configurable policy mix.

Mechanisms required:
- government budget;
- transfers/services;
- household disposable resources;
- healthcare/social-service capacity;
- employment/demand;
- health pathways;
- mortality/morbidity where implemented;
- distributional effects;
- political response.

No direct “austerity causes N deaths” rule.

## 9. Climate/environment-driven migration

Mechanisms required:
- climate baseline vs realized weather;
- water/hydrology;
- agriculture/livelihood;
- disaster/infrastructure damage;
- household/population welfare;
- destination attractiveness/capacity;
- migration network/constraints;
- labor/housing/services;
- fiscal/political response.

## 10. Global trade fragmentation / chokepoint disruption

Shock examples:
- Suez/Red Sea disruption;
- sanctions/trade bloc split;
- port closure.

Mechanisms required:
- transport networks;
- routing/capacity;
- transit time;
- inventories;
- freight cost;
- substitution;
- production inputs;
- prices;
- investment;
- strategic stockpiles;
- political response.

## 11. Domestic institutional crisis

Mechanisms required:
- multiple authority-bearing institutions;
- legal/constitutional authority scopes;
- actor-perceived state;
- competing commands;
- coalition/legislative or setting-specific decision processes;
- security-force authority where applicable;
- public legitimacy/support;
- economic/financial reaction.

This is a direct test that not every political actor is implemented as `CountryInstance`.

## Certification levels

### L0 — state
All required authoritative state can be represented.

### L1 — native mechanism
Each participating domain performs its own causal update.

### L2 — cross-domain
Outputs flow through explicit typed seams into other domains.

### L3 — perception/agency
Relevant actors observe imperfectly and choose legal responses.

### L4 — replay/explanation
Outcome is deterministic for fixed seed/inputs and causal provenance can explain major transitions.

### L5 — world-scale
Scenario runs within current WORLD-SCALE-CONTRACT budgets.

A scenario is fully certified only at L5. Earlier levels are useful progress markers but not completion.
