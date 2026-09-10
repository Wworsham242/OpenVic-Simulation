# PROJECT-CONVERGENCE-005A6 — Production Composition Proof

## Purpose

005A6 proves that multiple real productive sites can execute distinct setting-general `PROCESS` definitions through the same native production, inventory, workforce, logistics, and market orchestration.

The increment removes one narrow remaining assumption from `LiveEconomyRuntime`:

additional upstream productive sites no longer have to reuse the primary upstream site's `ProductionType`.

## Native Repository Basis

Before 005A6, `LiveEconomyRuntime` already supported:

- a primary bound productive site;
- additional bound upstream productive sites;
- stable site identity;
- one `AggregateProducer` per site;
- one producer-specific market bridge per site;
- one shared province employment authority;
- native `Pop::hire()` employment mutation;
- shared material allocation;
- shared utility constraints;
- common `GoodMarket` settlement;
- deterministic additional-site ordering;
- provenance and status aggregation.

However, `bind_additional_upstream_site()` resolved every additional site against:

`upstream.get_production_type()`

and constructed each added producer with that same process.

Thus multiple physical sites were supported, but multiple distinct production recipes were not yet proven through the native live-economy orchestration.

## External Reference Review

No new economic model is introduced.

005A6 is an architectural composition proof over mechanisms already implemented in the repository:

- `ProductionType`;
- `AggregateProducer`;
- productive-site binding;
- shared workforce allocation;
- material-flow resolution;
- producer inventories;
- producer market bridges;
- `GoodMarket` clearing.

No new empirical coefficient or economic calibration was required.

## Chosen Mechanism

`bind_additional_upstream_site()` now has an overload accepting an explicit:

`ProductionType const& production_type`

The overload:

1. requires the supplied production type to be a setting-general process;
2. validates the physical binding against that process;
3. preserves existing employer-identity uniqueness;
4. constructs the site's existing `AggregateProducer` with the supplied process;
5. preserves deterministic site sorting.

The original two-argument overload remains and delegates to the primary upstream process.

Therefore existing callers retain their prior behavior.

## Composition Proof

The test fixture now defines two distinct setting-general processes.

### Primary process

1 unit feedstock

→ 1 unit intermediate output

### Alternate process

2 units feedstock

→ 1 unit intermediate output

Both use the same underlying production machinery.

The second physical building can be configured with the alternate process.

The 005A6 integration test then binds that site explicitly with the alternate process.

## Causal Integration

Both processes participate in the same causal path:

authoritative province POPs

→ one employment allocation phase

→ site-specific workforce assignments

→ existing `AggregateProducer` capacity

→ shared material-flow allocation

→ site-specific physical inventory consumption

→ site-specific production result

→ site-specific market bridge

→ one existing intermediate-good market clearing.

No parallel production scheduler, workforce store, or clearing engine is introduced.

## Workforce Authority

The proof uses one province-local POP pool containing 60 available workers.

The two real productive sites request more labor than can jointly be satisfied.

Observed allocation is:

- primary site: 40 workers;
- alternate site: 20 workers;
- total: 60 workers;
- unemployment after allocation: 0.

This is performed through the existing province employment phase and existing `Pop::hire()` authority.

There is no second hiring pass.

## Material Conservation

The source provides four feedstock units.

The two distinct recipes satisfy:

`primary_output * 1 + alternate_output * 2 = 4`

The test verifies this identity directly.

It also verifies that both producer feedstock inventories are zero after production, proving the delivered physical feedstock was consumed rather than duplicated into parallel inventories.

## Market Conservation

Both producers output the same intermediate good for this bounded proof.

Each producer submits through its own existing market bridge.

The test verifies:

- primary amount sold equals primary actual output;
- alternate amount sold equals alternate actual output;
- total sold equals total actual upstream output;
- runtime aggregate upstream output equals the sum of both producers;
- only one intermediate market clearing occurs.

Thus adding a distinct production recipe does not create a second clearing system.

## Determinism

Existing additional-site ordering remains based on stable employer identity.

The previous same-process multi-site test remains unchanged and continues to prove that opposite binding-call order produces identical authoritative outcomes.

005A6 does not weaken that invariant.

## Compatibility

The original:

`bind_additional_upstream_site(binding, map)`

API remains.

It delegates to:

`upstream.get_production_type()`

and therefore preserves all previous same-process behavior.

Existing multi-site regression tests remain green.

## Victoria-Specific Boundary

005A6 does not introduce a factory-only or Victoria-era production ontology.

The new seam accepts any existing setting-general `PROCESS`.

Building identity remains a physical-site binding concern.

Production semantics remain defined by data-backed `ProductionType`.

## Modern-Specific Boundary

005A6 also does not encode a modern industrial sector taxonomy.

The alternate 2:1 test recipe is only a proof fixture.

The engine does not learn concepts such as:

- refinery;
- semiconductor fab;
- steel mill;
- power plant;
- defense plant.

Those remain scenario/content semantics.

## Scope Boundary

005A6 does not:

- replace `ProductionType`;
- replace `AggregateProducer`;
- add a universal economy graph;
- add a second employment ledger;
- add a second material ledger;
- add a second market-clearing mechanism;
- generalize every economy runtime field;
- support arbitrary multi-output production;
- add producer-to-producer direct exchange;
- add sector ontology;
- change wage formation;
- change utilization decisions;
- change material allocation policy;
- change logistics routing;
- change market pricing.

The proof is deliberately limited to distinct process composition inside the existing authoritative orchestration.

## Validation

005A6 validation includes:

- focused distinct-process composition test;
- real province workforce authority;
- real `Pop::hire()` employment mutation;
- shared physical feedstock conservation;
- producer inventory consumption;
- producer-specific market bridge settlement;
- one common market clearing;
- existing same-process multi-site regression;
- native workforce regression suite;
- full CTest;
- `git diff --check`;
- exact staged-file verification;
- local/remote HEAD equality after push.

## Result

005A6 establishes the following engine rule:

> Multiple productive sites may compose different data-defined transformation processes while sharing the same authoritative labor, material, inventory, logistics, and market mechanisms.

This removes another hardwired orchestration assumption without introducing a new economy framework.
