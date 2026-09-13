# PROJECT-CONVERGENCE-006A1 — Canonical Engine Capability & IFs Reconciliation Audit

**Status:** Architecture gate / implementation sequencing authority  
**Canonical repository:** `Wworsham242/OpenVic-Simulation`  
**Canonical branch:** `work/live-economy-005-modern-catalog`  
**Audited commit:** `c278d287636bf7baf461750cddc6c9051fb86948`  
**Commit:** `PROJECT-CONVERGENCE-005A30 derive sustainment availability`  
**Date:** 2026-09-12

## 1. Mandate

This audit implements the World Simulation Engine Development Constitution requirement to return to engine-wide architecture review after a subsystem becomes disproportionately deep.

The project is a **general causal world-simulation engine**. The modern world is the maximum-complexity reference case; Bronze Age and other simpler eras are architectural counter-tests. The engine must not require modern mechanisms when a game package omits them.

The audit therefore asks:

1. What authoritative mechanisms already exist?
2. Which mechanisms are general substrate, optional modules, or setting content?
3. Which domains have real causal breadth and which only have stubs or inherited ontology?
4. Which hard-coded OpenVic/Victoria assumptions still prevent cross-era composition?
5. Which states participate in persistence/replay and which do not?
6. Which domains possess actor-perceived state versus only authoritative truth?
7. Which mechanisms have whole-world scale evidence?
8. What work should happen next under the Constitution rather than simply following the deepest current feature chain?

## 2. Evidence hierarchy

1. **World Simulation Engine Development Constitution** — supreme development policy.
2. **Pinned OpenVic implementation at `c278d287...`** — canonical implementation evidence.
3. **Architecture documents and tests at the pinned commit** — design intent plus validation evidence.
4. **`REAL-WORLD-MODEL-REFERENCE-FRAMEWORK.md`** — mandatory mechanism research/reconciliation method.
5. **Older WargameEngine material** — secondary historical evidence only; not canonical implementation.
6. **External scientific, policy, defense, and game models** — references, never ontology authorities.

## 3. Executive finding

The project has a credible general simulation chassis. It is not merely a military logistics engine.

Strong or useful foundations already exist for:

- deterministic time, cadence, ordered commands, and RNG;
- authority and command admission;
- fixed-point quantitative state;
- ECS/runtime composition;
- production and productive-site capacity;
- markets and physical acquisition;
- inventories, resource flows, logistics graphs, pathfinding, capacity allocation, rerouting;
- bounded causal provenance;
- environmental agricultural constraint;
- food-scarcity → POP-needs → survival-stress → nutrition-health burden chain;
- age/sex demographic representation and reconciliation/import seams;
- optional capability proofs;
- bounded data-defined calculations;
- generic military domains/formations;
- equipment allocation, persistent shipment state, transport execution, sustainment stocks and physical consumption.

The current failure is **not lack of mechanisms**. It is **architectural imbalance**.

The largest present blockers are:

1. no proven general package/module composition across radically different settings;
2. country/Victoria-shaped actor and institution ontology;
3. no general actor-perceived-state / report pipeline;
4. incomplete authoritative-world persistence/replay;
5. no current-engine whole-world scale proof;
6. no general finance/credit/claims/settlement domain;
7. incomplete demographic transition mechanics, especially births/deaths/migration;
8. politics/governance/diplomacy remain insufficiently general;
9. civilian infrastructure/energy/environment remain much shallower than logistics;
10. military/logistics has exceeded the depth appropriate before the above gaps are attacked.

Therefore `PROJECT-CONVERGENCE-005A31` is **deferred**, not rejected.

## 4. Constitution compliance finding

### 4.1 Strong compliance

The current engine generally follows these rules well:

- authoritative state is favored over duplicate ledgers;
- new state is usually added only when history must persist;
- derived values are kept derived where possible;
- resource flows and military replenishment reuse common logistics machinery;
- recent military mechanics preserve causal distinctions rather than collapsing them into arbitrary modifiers;
- environmental stress enters physical farm output before markets and POP consumption;
- health work does not directly convert food shortage into deaths;
- optional capability experiments have begun;
- restricted calculations avoid a general unrestricted scripting VM;
- A30 explicitly records physical sustainment fulfillment rather than inventing a universal combat penalty.

### 4.2 Partial compliance

- Optionality exists as bounded proofs, but there is no complete package/capability system.
- Deterministic replay primitives exist, but not all authoritative world state is serialized/replayed.
- Environmental and health chains are causally correct in shape but narrow in physical scope.
- Population has cohort structure, but the inherited POP schema remains strongly Victoria-shaped.
- Geography/logistics are reusable, but actor, institution, and information models remain less general.
- Causal provenance exists but is not yet a whole-engine explanation contract.

### 4.3 Current constitutional failures / gates

No major domain can yet be formally certified at **Minimum Causal Viability** because the Constitution requires operation within a whole-world performance budget and the current C++/OpenVic engine has not yet passed that gate.

The military/logistics chain has also gone well beyond five consecutive subsystem increments. Further deepening without an architecture/scale checkpoint would violate the development rule unless required to close a named cross-domain proof.

## 5. General substrate review

### Time, cadence, event scheduling — EXISTS / STRONG

The Foundation series provides deterministic time/event/cadence machinery and later ordered command/replay and named RNG work.

**Decision:** retain as universal substrate.

**Next concern:** prove multi-rate operation under representative global workload. No requirement should force every domain to scan every record every hour.

### Identity and registries — EXISTS / STRONG

Native identity, registries, definitions/instances and deterministic identifiers are reusable.

**Decision:** retain.

**Risk:** inherited entity classes still carry setting-specific ontology. Identity is general; the objects identified are not always general.

### Authority and command admission — EXISTS / PARTIAL

Authority grants, position occupancy, ordered commands and command admission exist.

**Decision:** retain as substrate.

**Gap:** authority-bearing actors are not yet generalized sufficiently beyond country/player-position assumptions.

### Fixed-point deterministic math — EXISTS / STRONG

The project already uses deterministic fixed-point calculations and bounded calculation surfaces.

**Decision:** retain. This is suitable for authoritative quantitative simulation where determinism matters.

### Causal provenance — EXISTS / PARTIAL

The project has bounded provenance mechanisms and many newer domain results expose causal facts.

**Decision:** expand toward a common causal-record interface. Do not turn provenance into universal simulation physics.

### Persistence/replay — PARTIAL / BLOCKER

Timeline, commands, RNG streams and ECS identity have persistence primitives. At least some later authoritative native state is explicitly not covered. The 004A1 environmental document states that `CampaignStateSnapshot` does not persist the authoritative province environmental field.

**Decision:** world-state persistence coverage must become an explicit program before claiming campaign-grade determinism.

### Observation / perceived state — ABSENT / BLOCKER

Authoritative truth exists. A generic mechanism for actor observation, delay, uncertainty, deception, access control, report delivery and actor knowledge does not.

**Decision:** build after composition/scale proof as `006A4`.

## 6. Economy, production and logistics review

### Production — EXISTS / STRONG FOUNDATION

The engine has general production-process, facility-capacity, productive-site and live-economy work. This is one of the stronger domains.

**Classification:** optional mechanism(s) over universal stocks/flows.

### Markets — EXISTS / STRONG FOUNDATION

OpenVic's market machinery has been integrated into physical causal chains and deliverable supply/access work.

**Classification:** optional economic module, not universal substrate.

**Generality risk:** market assumptions must not be required for Bronze Age redistribution/tribute systems or command economies.

### Inventory and resource flows — EXISTS / STRONG

This is close to universal substrate when expressed as ownership, stock, transfer, capacity and conservation without assuming a specific economic institution.

### Logistics graph, route capacity, rerouting — EXISTS / STRONG

Reusable deterministic graph and capacity mechanisms are among the best pieces of the engine.

**Decision:** preserve as common substrate.

### Persistent shipments / transport execution — EXISTS / STRONG

A23–A27 establish persistent material-in-transit and separate transport capacity from cargo.

**Decision:** reuse civilian and military. Do not create separate military transport physics.

### Finance — ABSENT / HIGH-PRIORITY DOMAIN GAP

There is no demonstrated general architecture for:

- bank balance sheets;
- deposits and loans;
- securities/claims;
- collateral;
- credit creation;
- payment/settlement;
- liquidity;
- insolvency/default propagation;
- central-bank operations;
- exchange-rate/monetary mechanisms.

OpenVic fiscal/cash mechanisms are not a substitute for this.

**Decision:** finance must reach MCV before military logistics grows much further.

## 7. Population, health and demography review

### Population authority — EXISTS / INHERITED BUT RIGID

`Pop` remains authoritative for population size and employment, and `Pop::hire()` remains employment mutation authority.

This is valuable conservation authority.

However, inherited mandatory dimensions still include Victoria-specific `PopType`, culture, religion, militancy, consciousness, rebel type, strata behavior, ideology/political support, reform support, military support and other compatibility fields.

**Decision:** do not replace population authority wholesale. Introduce composition boundaries around optional population dimensions.

### Nutrition health — EXISTS / NARROW OPTIONAL CAPABILITY

The project correctly distinguishes:

food access → survival stress → health vulnerability → stored nutritional burden.

It explicitly does not create disease, mortality, migration or rebellion.

**Decision:** retain. Good example of optional history-dependent state.

### Age/sex cohorts — EXISTS / PARTIAL

The 004A6–A12 sequence adds age/sex demographic representation, reconciliation, authority/import work.

**Gap:** full transition ownership is incomplete.

### Fertility, mortality, cohort aging, migration — PARTIAL / MISSING

These are required for a real demographic engine and for the planned environment→economy→population→migration→politics vertical.

**Decision:** after the 006 architecture gate, demographic transition should be one of the first breadth domains completed to MCV.

## 8. Environment, agriculture, energy and infrastructure

### Environmental authoritative state — EXISTS / VERY NARROW

Current physical environmental authority demonstrated by A1 is essentially province `water_availability` in `[0,1]`.

This is explicitly not:

- weather generation;
- soil moisture;
- hydrology;
- irrigation;
- temperature stress;
- crop-specific sensitivity;
- disease/pests;
- calibrated agronomy.

**Decision:** correct seam, insufficient domain.

### Agriculture coupling — EXISTS / PARTIAL

Environmental state physically constrains farm output before normal market submission.

This is architecturally sound and should be retained.

### Energy — PARTIAL

Energy/resource/electricity concepts exist in pieces, but there is no demonstrated general energy system with supply, transformation, networks, dispatch, storage, fuel constraints and cross-sector demand comparable to the maturity of logistics.

### Civil infrastructure — PARTIAL

Transport capacity is relatively mature. Water, sanitation, health facilities, electricity, communications and other infrastructure are not yet represented as a general composable family.

**Decision:** IFs and GCAM-style decomposition should be used to build a coverage map, not to copy their equations wholesale.

## 9. Politics, governance, actors and diplomacy

### General actors/organizations — WRONG ABSTRACTION / BLOCKER

`CountryInstance` and Victoria-era political objects still dominate major inherited concepts.

The target engine needs authority-bearing entities such as:

- polities;
- ministries;
- legislatures;
- central banks;
- courts;
- corporations;
- parties;
- unions;
- alliances;
- insurgencies;
- international organizations;
- temples/palaces/dynasties/merchant houses in historical packages.

These must not all masquerade as countries.

**Decision:** `006A5` must prove a general actor/institution capability without constructing one universal institution simulator.

### Politics/governance — PARTIAL / MAJOR GAP

Inherited political state provides material to reuse, but it does not yet establish a general institutional decision architecture applicable across eras.

### Diplomacy — PARTIAL / COUNTRY-CENTRIC

Relationships between countries are useful inherited content but insufficient for non-state actors, alliances, institutions and perceived-state decision making.

## 10. Intelligence, information and cyber

### Perceived state — ABSENT

This is a universal architectural gap.

Required generic pipeline:

`authoritative fact`
→ `observation/access`
→ `noise / delay / deception`
→ `report`
→ `actor knowledge`
→ `decision`.

A Bronze Age messenger and a modern satellite report should use the same broad substrate but different optional mechanisms/content.

### Intelligence — ABSENT / OPTIONAL DOMAIN

Full intelligence-cycle mechanics should consume the general observation/report substrate rather than define it.

### Cyber/information warfare — ABSENT / OPTIONAL DOMAIN

Should remain optional and modern-setting dependent.

## 11. Military review

Military/logistics is presently the deepest newly built domain.

Useful general contributions include:

- extensible military-domain identifiers;
- generic formation definitions/instances;
- placement/hosting;
- layered support relationships;
- equipment requirements/assignment/condition;
- shortfall-only replenishment;
- network-resolved shipment;
- transport execution;
- aggregate sustainment stock;
- activity-driven physical consumption;
- per-resource fulfillment.

A30 correctly keeps `limiting_fraction` diagnostic and does not automatically convert it into combat power/readiness/movement/effectiveness.

**Decision:** keep all of it.

**Sequencing decision:** stop deepening it now.

`005A31` remains a sound candidate when a cross-domain certification scenario requires shipment-backed consumable replenishment. It should not proceed merely because it is the obvious next military task.

## 12. IFs reconciliation

International Futures is useful as a **civilian-system coverage map**, not as an engine template.

Official IFs documentation describes integrated structural systems for:

- demography/population;
- goods and services/economics;
- financial flows;
- energy;
- agriculture;
- environment;
- socio-political/governance;
- education;
- health/human development;
- infrastructure/transportation;
- interstate interaction.

IFs uses structural/agent-class representations rather than one-micro-agent-per-person simulation. That is compatible with this project's preference for aggregate/cohort/systemic actors at world scale.

### Reconciliation rules

For each IFs domain, future audits must record:

1. authoritative IFs-like state concept;
2. main causal inputs;
3. main causal outputs;
4. characteristic cadence/timescale;
5. cross-domain links;
6. corresponding OpenVic/current-engine capability;
7. whether ours is `EXISTS`, `PARTIAL`, `ABSENT`, `WRONG-ABSTRACTION`, or `COMPUTE-RISK`;
8. what belongs in universal substrate;
9. what belongs in optional module;
10. what belongs in scenario content;
11. which IFs aggregate assumptions should **not** be imported into a playable simulation.

### Current IFs-domain coverage

| IFs structural area | Current engine | Audit judgment |
|---|---|---|
| Demography | POP + age/sex cohort work | PARTIAL |
| Economy | production + markets + workforce | STRONG PARTIAL |
| Financial flows | fiscal/cash pieces only | ABSENT as general finance |
| Energy | resource/electric pieces | PARTIAL / shallow |
| Agriculture | RGO + environmental water constraint | PARTIAL |
| Environment | one water-availability seam | VERY PARTIAL |
| Health | nutritional burden only | VERY PARTIAL |
| Infrastructure | transport/logistics strong; other infrastructure shallow | UNEVEN |
| Governance/sociopolitical | inherited Victoria structures | WRONG-ABSTRACTION / PARTIAL |
| Interstate relations | country diplomacy | PARTIAL / country-centric |
| Education | no demonstrated general domain | ABSENT |
| Transportation | logistics network/capacity strong | STRONG |
| Long-horizon cross-domain integration | isolated causal chains | PARTIAL |

## 13. Formal MCV status

Under the Constitution, a domain reaches Minimum Causal Viability only if it has:

- appropriate authoritative state;
- at least one native causal mechanism;
- at least one cross-domain interaction;
- deterministic replay participation;
- whole-world performance within budget.

Because the current C++ engine does not yet have a representative whole-world benchmark and world-state persistence coverage is incomplete, **no major domain is certified MCV-complete in this audit**.

This is deliberate. MCV certification must be an evidence gate, not an impression.

## 14. Scale risk

Historical WargameEngine evidence suggests that a world with roughly:

- 200 sovereign-scale actors;
- 100,000 geographic/network points;
- hundreds of thousands to ~1,000,000 aggregate socioeconomic records;
- 50,000 formations

can fit within consumer-memory ranges if representation is compact.

That evidence is **not transferrable as proof** to the current OpenVic/C++ architecture.

Primary scale hypothesis:

- RAM is manageable with aggregate records and compact state.
- CPU work is the larger risk if systems scan inactive records at high cadence.
- event-driven updates, dirty sets, multirate scheduling, cached aggregates and bounded AI are mandatory design assumptions.

## 15. 006 program

### 006A1 — Canonical Engine Capability & IFs Reconciliation Audit

**This document.**

Acceptance:
- canonical pinned snapshot;
- capability register;
- IFs reconciliation;
- blockers ranked;
- A31 disposition recorded;
- next-work order established.

### 006A2 — Current-Engine World-Scale Synthetic Harness

Build a **measurement harness**, not more gameplay.

Staged workload targets:

- 200 sovereign-scale actors;
- 100,000 geographic/network points;
- up to 1,000,000 aggregate economic/population records;
- 50,000 formations;
- representative active/quiet fractions;
- realistic domain cadences;
- events, route work, dirty recomputation, provenance and persistence activity.

Required measurements:

- retained memory;
- temporary peak memory;
- per-domain phase time;
- p50/p95/p99 tick or scheduled-work latency;
- records scanned vs records actually changed;
- events processed;
- route searches/cache hits;
- dirty-set sizes;
- provenance records;
- save size/load time where supported.

No invented performance pass/fail threshold becomes constitutional without evidence. The older WargameEngine 4/6 GiB and timing budgets are provisional historical starting points only.

### 006A3 — Bronze Age / Modern Capability Composition Proof

One executable, two tiny packages.

**Bronze package must be coherent while omitting:**
- banking;
- monetary policy;
- aviation;
- cyber;
- electrical grid;
- modern legislature assumptions;
- modern military domains.

**Modern package may enable additional capabilities.**

The test should identify every Victoria-specific mandatory ontology that prevents this composition.

### 006A4 — Actor-Perceived State & Report Delivery Proof

Build only the general substrate:

`truth → observation → delay/noise/access/deception → report → actor knowledge`.

Proof cases:

- Bronze messenger: delayed report, limited access.
- Modern sensor/intelligence report: shorter delay, uncertainty or deception.

Do not yet build a full intelligence game.

### 006A5 — General Actor / Institution Capability Proof

Prove two radically different authority-bearing actors can exist without pretending to be countries.

Suggested test pair:

- Bronze Age palace/temple administration;
- modern central bank or ministry.

Required shared properties may include identity, authority scope, resources/claims, observation, decision cadence and command emission.

Do not create one universal institution behavior model.

### Checkpoint after A5

The Constitution requires another architecture review before a new subsystem is allowed to run deep.

## 16. Work after the 006A gate

Preferred first broad causal vertical:

`environment`
→ `physical production/resources`
→ `market/redistribution`
→ `household/population welfare`
→ `health`
→ `demographic movement/migration`
→ `labor/services/fiscal effects`
→ `political/institutional perception`
→ `response`
→ feedback.

Run it in two settings:

### Bronze Age proof
Drought/resource stress affects agricultural production, stored food/redistribution, health, migration, tribute/labor availability, military capacity and political authority.

### Modern proof
Drought/heat/resource stress affects production, food/energy/water prices, health, internal/international migration, infrastructure, public finance and political response.

The two scenarios must use the same general substrate but different optional modules/content.

## 17. Permanent cross-domain certification scenarios

Maintain scenario families as architecture kill tests:

1. Bronze Age environmental/political collapse.
2. Iran/Hormuz regional crisis.
3. Taiwan blockade/invasion.
4. Russia–Ukraine high-intensity war.
5. Russia–NATO escalation.
6. Global banking/financial crisis.
7. Pandemic/public-health emergency.
8. Fiscal austerity/public-health consequences.
9. Climate/environment-driven migration.
10. Global trade fragmentation / chokepoint disruption.
11. Domestic institutional crisis.

Tests certify mechanisms and interactions, **not predetermined outcomes**.

A scenario-specific hard-coded consequence chain is an architecture failure when ordinary domain mechanics should generate the result.

## 18. Kill criteria

Stop or redesign if any of the following occurs:

- a sixth consecutive same-subsystem increment begins without a constitutional checkpoint;
- a domain is called MCV-complete without scale evidence;
- camera/viewport/zoom changes authoritative physics;
- a new feature creates a duplicate authoritative ledger;
- a country-only object becomes mandatory substrate for all authority-bearing actors;
- modern finance, air warfare, cyber, legislature, etc. become mandatory for historical packages;
- a summary scalar replaces conserved physical/financial state needed for downstream causality;
- a crisis is solved primarily by a bespoke scripted outcome chain;
- per-record high-frequency scans are introduced without measured need;
- unbounded histories/provenance accumulate;
- new micro-resolution is added without demonstrating strategically relevant causal benefit;
- empirical and provisional coefficients are mixed without labeling.

## 19. Decision

`PROJECT-CONVERGENCE-005A` closes at A30.

`PROJECT-CONVERGENCE-005A31` is parked.

The next implementation increment is:

# `PROJECT-CONVERGENCE-006A2 — Current-Engine World-Scale Synthetic Harness`

No new domain depth should be added before A2 establishes the current engine's actual scale envelope.

## 20. External reference notes

Current official IFs material confirms:

- IFs is an integrated global model covering environment, infrastructure, health, governance, demographics, agriculture, energy, economics and interstate relations.
- Its structural methodology combines cohort-component population, markets for goods/services and social-accounting/financial-flow structures.
- It is explicitly agent-class/structure based rather than a one-person-one-agent world model.

This makes IFs useful for **coverage and coupling reconnaissance**. It does not justify copying IFs wholesale or importing long-horizon aggregate equations into short-cadence operational simulation.
