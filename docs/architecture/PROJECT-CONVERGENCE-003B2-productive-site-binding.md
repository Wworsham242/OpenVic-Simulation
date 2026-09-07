# Native productive-site binding

`InstanceManager::bind_live_upstream_site` accepts a `ProductiveSiteBinding`.
The province identifier plus the building identifier identifies the existing
facility. A process identifier and explicit `LaborPoolScope` complete the binding.
No world pointers or additional building/population ownership are retained.

Each A6 due cycle resolves the binding through the real `MapInstance`. The
province-owned `BuildingInstance` supplies its current level; its `BuildingType`
supplies capacity per level and the production process. Resolution checks both
the process identifier and the authoritative definition object. Installed capacity
is recomputed from the real level on every cycle. Invalid initial bindings are
rejected without replacing a valid binding; failed later resolution supplies zero
capacity and an empty workforce rather than stale world values.

The current labor scope is explicitly `ProvinceLocal`. It resolves only the
facility province's existing POP colony. A non-owning `WorkforcePool` adapter
allows A5 to iterate that non-contiguous storage using the same allocation
implementation as its original span interface. No POP copies, worker ledger or
population container are introduced. `Pop::hire()` remains authoritative.

The live resolver runs in A6's prepare callback after `map_tick`, preserving:

1. POP daily employment reset and other existing POP work;
2. building ticks and existing RGO hiring;
3. productive-site resolution and A5 allocation from residual unemployment;
4. upstream work, one market clearing, and downstream production.

RGO-first allocation is a temporary migration priority artifact, not a permanent
modern labor-market policy. Province colony iteration and the existing due-cycle
order determine allocation order. No employer competition or wages are added.

Alternative labor scopes belong in the world resolver and its non-owning range
adapters. State, commuting-region or custom-area resolution can reuse the same
authoritative POP objects and allocation accounting without changing
`AggregateProducer` or `BuildingInstance`. No unimplemented scope is silently
treated as province-local.

A7 snapshots include the stable binding identity and the observed capacity and
hire results. World tests construct actual map-owned provinces, their actual
building instances and POP colonies. Between tested cycles they replace the
world POPs to supply fresh daily availability; they do not simulate the legacy
POP-needs phase. Production uses the same resolver as the post-map InstanceManager
path. Tests cover real level changes, fewer real POPs, residual employment,
unrelated-province isolation and invalid identity/process/scope rejection.

The only new world accessor is identifier-based mutable building lookup, so
callers can use the existing validated `BuildingInstance::set_level` API. No
ownership, lifetime, scheduler, RGO or FactoryProducer redesign is involved.
