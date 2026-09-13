# CURRENT ENGINE CAPABILITY REGISTER

**Canonical snapshot:** `Wworsham242/OpenVic-Simulation@c278d287636bf7baf461750cddc6c9051fb86948`  
**Program:** `PROJECT-CONVERGENCE-006`  
**Status vocabulary:** `EXISTS`, `PARTIAL`, `ABSENT`, `WRONG-ABSTRACTION`, `COMPUTE-RISK`, `DEFERRED`

This file is intended to become the first-stop architecture register before any new implementation increment.

| Capability | Layer | Current authority / mechanism | Status | Main gap / risk | MCV |
|---|---|---|---|---|---|
| Time / calendar / cadence | substrate | Foundation time/event cadence | EXISTS | global multirate scale proof | not certified |
| Scheduled events | substrate | deterministic event runtime | EXISTS | event-volume benchmark | not certified |
| Deterministic RNG | substrate | named deterministic RNG | EXISTS | whole-world replay coverage | not certified |
| Ordered commands / replay | substrate | ordered command runtime | EXISTS | authoritative world coverage incomplete | not certified |
| Identity / registries | substrate | native definitions/instances/registries | EXISTS | inherited object ontology | not certified |
| ECS integration | substrate | native ECS integration | EXISTS | scale/layout measurement | not certified |
| Authority / jurisdiction | substrate | authority registry + position/session machinery | PARTIAL | actors remain insufficiently general | not certified |
| Command admission | substrate | validated authority path | EXISTS | actor generalization | not certified |
| Fixed-point quantitative math | substrate | native fixed point | EXISTS | none structural | not certified |
| Bounded calculations | optional/substrate seam | restricted calculation proof | EXISTS | package-facing composition | not certified |
| Geography | substrate | provinces/regions/map + grouping seams | PARTIAL | compiled resolution/ontology assumptions | not certified |
| Graph topology | substrate | logistics graph | EXISTS | generalized multimodal infrastructure taxonomy | not certified |
| Stocks / inventories | substrate | resource/inventory mechanisms | EXISTS | world persistence/scale | not certified |
| Flows / transfers | substrate | production/market/logistics flows | EXISTS | generalized settlement across domains | not certified |
| Route capacity | substrate | shared logistics capacity | EXISTS | benchmark/cache strategy | not certified |
| Rerouting | substrate | allocation-aware deterministic rerouting | EXISTS | scale under congestion | not certified |
| Persistent shipments | substrate/optional logistics | SOURCE→IN_TRANSIT→ARRIVED→DELIVERED | EXISTS | broaden civilian usage + persistence | not certified |
| Transport execution | optional logistics | transport capacity/execution state | EXISTS | cross-domain sharing under scale | not certified |
| Production processes | optional economic | setting-general production process | EXISTS | package composition | not certified |
| Facility capacity | optional economic | general facility capacity | EXISTS | infrastructure/energy inputs | not certified |
| Productive-site binding | optional economic | site/producer binding | EXISTS | broader site types | not certified |
| Workforce allocation | optional economic | native POP employment authority | EXISTS | Victoria POP schema | not certified |
| Markets | optional economic | OpenVic native market + bridges | EXISTS | must remain optional | not certified |
| Fiscal collection | optional institution/economic | vertical fiscal work | PARTIAL | general government finance | not certified |
| Banking / credit | optional finance | none demonstrated | ABSENT | full domain missing | no |
| Financial claims/securities | optional finance | none demonstrated | ABSENT | ownership/valuation/default | no |
| Payments/settlement | optional finance | none general | ABSENT | conserved financial flows | no |
| Central banking/monetary policy | optional finance | none | ABSENT | actor + finance prerequisite | no |
| Population totals | optional population | `Pop::size` | EXISTS | Victoria-shaped schema | not certified |
| Employment | optional population/economic | `Pop::hire`, employed/unemployed | EXISTS | schema generalization | not certified |
| Optional population health | optional population/health | nutrition-health capability | EXISTS | only one bounded optionality proof | not certified |
| Age/sex cohorts | optional demography | A6–A12 cohort/profile work | PARTIAL | transitions | not certified |
| Fertility | optional demography | no complete transition owner demonstrated | ABSENT/PARTIAL | age-specific fertility | no |
| Mortality | optional demography | no complete transition owner demonstrated | ABSENT/PARTIAL | baseline + excess mortality | no |
| Cohort aging | optional demography | representation exists | PARTIAL | runtime transition cadence | no |
| Migration | optional demography | inherited accounting, no general modern causal owner | PARTIAL | push/pull/network/constraints | no |
| Environmental truth | optional environment | province water availability | PARTIAL | weather/hydrology/soil/temperature | no |
| Agriculture coupling | optional agriculture | water constrains farm output | PARTIAL | crop/water/weather/land detail | no |
| Food scarcity | cross-domain chain | market supply→POP needs | EXISTS | broader substitution/storage/trade | not certified |
| Survival stress | optional population | lagged resource stress | EXISTS | broader welfare dimensions | not certified |
| Nutrition health burden | optional health | history-dependent burden | EXISTS | disease/morbidity/treatment | not certified |
| Epidemiology | optional health | none | ABSENT | transmission + healthcare | no |
| Healthcare capacity | optional health/infrastructure | none general | ABSENT | facilities/staff/supplies | no |
| Energy supply/transformation | optional energy | pieces only | PARTIAL | coherent energy domain | no |
| Electricity grid/dispatch | optional energy | not demonstrated as mature general system | PARTIAL/ABSENT | network dispatch/storage/fuel | no |
| Water infrastructure | optional infrastructure | none general | ABSENT | reservoirs/distribution/sanitation | no |
| Communications infrastructure | optional infrastructure | none general | ABSENT | information/report substrate link | no |
| Causal provenance | substrate/analysis | bounded provenance + causal results | PARTIAL | common cross-domain contract | not certified |
| Authoritative/perceived split | substrate | authoritative truth exists | PARTIAL | perceived state absent | no |
| Observation/access | substrate | no general owner | ABSENT | 006A4 | no |
| Reports/delay/noise/deception | substrate | no general owner | ABSENT | 006A4 | no |
| Actor knowledge | substrate | no general owner | ABSENT | 006A4 | no |
| Intelligence | optional | none general | ABSENT | depends on observation substrate | no |
| Cyber | optional | none | ABSENT | modern-only capability | no |
| Information operations | optional | none general | ABSENT | beliefs/report/deception prerequisites | no |
| Country actor | setting/inherited | CountryInstance | EXISTS | overused as general actor | n/a |
| General actor | substrate | partial position/authority concepts | WRONG-ABSTRACTION/PARTIAL | 006A5 | no |
| General institution | optional composition | none general | ABSENT | 006A5 | no |
| Politics/governance | optional | inherited Victoria political structures | PARTIAL | cross-era institutional model | no |
| Diplomacy | optional | country-centric relationships | PARTIAL | non-state actors/alliances/knowledge | no |
| Research/technology | optional | inherited research/tech | PARTIAL | general capability diffusion/R&D | no |
| Military domains | optional military | extensible domain identity | EXISTS | domain-specific mechanics still needed | not certified |
| Formation definitions/instances | optional military | generic formations | EXISTS | scale + actor integration | not certified |
| Hosting/placement | optional military | placement/hosting proof | EXISTS | geography/infrastructure coupling | not certified |
| Military support relations | optional military | layered support | EXISTS | operational interpretation | not certified |
| Equipment requirements | optional military | explicit requirements | EXISTS | production/procurement lifecycle | not certified |
| Equipment assignment | optional military | persistent assignment state | EXISTS | loss/repair/replacement breadth | not certified |
| Equipment replenishment | optional military/logistics | shipment-backed replenishment | EXISTS | cross-domain demand certification | not certified |
| Sustainment stock | optional military | aggregate formation stock | EXISTS | replenishment A31 deferred | not certified |
| Activity consumption | optional military | physical activity-driven demand | EXISTS | domain activity breadth | not certified |
| Sustainment fulfillment | optional military | per-resource fulfillment + diagnostic minimum | EXISTS | downstream mechanics intentionally separate | not certified |
| Full operational combat | optional military | inherited/partial | PARTIAL | sensors, fires, air/maritime, EW, etc. | no |
| Package/module composition | substrate | bounded bootstrap/optional capability proofs | PARTIAL | no general package capability manifest | no |
| Bronze Age package proof | certification | none | ABSENT | 006A3 | no |
| Modern package proof | certification | current content fragments | PARTIAL | 006A3 | no |
| World-state save/load | substrate | campaign snapshot partial | PARTIAL | native world state missing | no |
| Whole-world replay | substrate | primitives | PARTIAL | state coverage + scale | no |
| Whole-world scale benchmark | certification | none current-engine | ABSENT | 006A2 | no |
| Bounded histories | substrate/performance | mixed | PARTIAL | audit all persistent history | no |
| Dirty/incremental recomputation | substrate/performance | some mechanisms | PARTIAL | global instrumentation | no |
| AI decision framework | optional | no mature general architecture demonstrated | ABSENT/PARTIAL | actor/perceived state first | no |
| Cross-domain scenario harness | certification | ad hoc vertical proofs | PARTIAL | permanent kill-test suite | no |

## Priority order

1. `006A2` world-scale synthetic harness.
2. `006A3` Bronze Age/modern composition proof.
3. `006A4` actor-perceived state/report delivery substrate.
4. `006A5` general actor/institution proof.
5. architecture checkpoint.
6. first breadth-domain MCV work: finance, demographic transitions, civilian environment/infrastructure/politics.
7. cross-domain vertical: environment→economy→population→migration→politics.
8. reintroduce deferred military/logistics increments only when demanded by certification scenarios.

## MCV certification rule

Do not change any `MCV` field to `yes` until the domain:

1. owns appropriate authoritative state;
2. executes a native causal mechanism;
3. has at least one cross-domain connection;
4. participates in deterministic replay;
5. passes the current whole-world performance budget.
