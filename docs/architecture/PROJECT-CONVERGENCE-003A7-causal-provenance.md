# Live economy causal provenance

`LiveEconomyRuntime::get_latest_provenance()` exposes the latest completed
cycle; `InstanceManager::get_live_economy_provenance()` forwards a value copy.
An unfinished cycle does not replace the previous completed snapshot. Storage
is bounded to one completed snapshot and one in-progress assembly. The optional
constructor recording flag permits outcome comparisons without snapshot assembly.

Native subsystems remain the authorities:

- `ResourceFlowResult` retains requested, nominal, physical-before-routing,
  accessible-after-routing, delivered, buffer and unmet quantities. Source
  availability and source access flags describe reductions of requested direct
  flow; a buffer can mask these reductions without erasing their causes.
- `WorkforceAllocationResult` records requested and actually hired workers from
  the existing allocation pass. Its absence means no native allocation happened;
  an externally supplied workforce figure is not represented as POP hiring.
- `AggregateProductionResult` retains installed, utilization, labor and input
  ceilings alongside the existing desired/actual output and input-limited flag.
  Potential output means installed capacity at current utilization before labor
  and inputs. The installed-ceiling flag describes the capacity minimum, not an
  invented demand for expansion. Independent input and labor ceilings coexist;
  `input_limited` retains its original meaning of reducing desired output.
- The published `DeliverableSupply` envelope preserves physical versus
  deliverable supply. Its domain calculation identifies delivery restriction.
- The two live `AggregateProducerMarketBridge` instances retain their own order
  quantities and actual `BuyResult`/`SellResult` callback quantities. Unfilled
  ordered demand identifies a transaction shortfall without guessing whether
  competition, affordability or another market rule caused it. Market-wide
  totals are not attributed to these producers.

The runtime copies these results in the fixed vertical's stage fields after the
single existing market clearing and downstream production. No recorded value
feeds back into a decision. The A6 due boundary supplies `due_time`; direct
compatibility pre/post calls leave it absent. Identifiers are existing definition
strings, quantities are fixed point, and stage layout is fixed. No pointer
identities, wall time, random IDs or unordered trace iteration are used.

This is a snapshot of one vertical, with multiple simultaneous constraint facts.
It does not create parent nodes, propagate causes through a general graph, retain
history, or persist provenance. It does not infer why external availability
changed. A5 POP-pool routing and A6's legacy compatibility driver remain unchanged.
