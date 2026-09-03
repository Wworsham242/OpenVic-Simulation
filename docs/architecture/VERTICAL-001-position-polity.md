# VERTICAL-001 â€” Position, occupant, and polity identity

Status: first post-foundation vertical slice.

## Audit result

The inherited nation layer is not one thing. It contains several different responsibilities that
must be treated differently.

### KEEP

- data-driven identifier/registry patterns in `CountryDefinition`;
- scenario/date bootstrap patterns in `CountryHistory`;
- indexed runtime collection patterns in `CountryInstanceManager`;
- territorial owner/controller relationships;
- geographic `State` as a province/population aggregation concept.

### GENERALIZE

- `CountryDefinition` from Victoria country/TAG metadata toward a sovereign-polity definition;
- `CountryInstanceManager` from Victoria rank/status management toward generalized polity runtime management;
- `CountryHistory` toward modern scenario bootstrap;
- diplomacy relationship storage away from Victoria-specific influence/sphere assumptions.

### DECOMPOSE / REWRITE OVER TIME

`CountryInstance` currently owns too many domain responsibilities in one object: political state,
budget/tax/spending, research, trade, military, population aggregates, diplomacy, rankings, territory,
and modifiers.

Do not split it first. Build explicit destination owners in verticals and migrate responsibilities
incrementally.

### REWRITE / COMPATIBILITY MIGRATE

Legacy `PlayerManager` models player control as a direct `CountryInstance*`.

The target model is:

```text
controller (human / AI / script / external)
              |
              v
        occupies position
              |
              v
position_id = authority-bearing actor
              |
              v
        jurisdiction_id
```

The country pointer remains only as a compatibility seam while UI/application code migrates.

## Core invariant: authority belongs to position, not controller

Human and AI occupants of the same office must produce the same:

- `position_id`;
- authority actor identity;
- jurisdiction.

Changing the controller must not silently change legal authority.

This directly supports the no-player-privilege rule.

## Identity layers

VERTICAL-001 deliberately separates:

1. `controller_id` â€” who/what is making decisions;
2. `position_id` â€” the stable authority-bearing actor;
3. `jurisdiction_id` â€” the scope over which that position can exercise authority;
4. legacy `country_index_t` / `CountryInstance*` â€” runtime compatibility identity only.

Do not turn a numeric country index into the generalized geopolitical identity.

## Geographic State is not sovereign state

OpenVic `map::State` is a geographic/province/population aggregation object. Keep it for that purpose.

Do not overload it with sovereign-polity or institutional meaning merely because the English word
"state" is ambiguous.

## State-capacity rule

Do **not** introduce a universal `state_capacity` scalar.

Where verticals require capacity, use named domain mechanisms such as:

- administrative capacity;
- fiscal extraction/collection capacity;
- coercive/security capacity;
- information/reporting capacity;
- logistics/mobilization capacity;
- regulatory/implementation capacity.

Those may share reusable `Capacity` mechanics where useful, but their causal meaning belongs to
domain/ruleset data.

A UI may later derive summary indicators from them; derived summaries must not become hidden causal
master variables.

## Government and institutions

Inherited `GovernmentType` is useful as a data-driven registry skeleton, but elections, ruling-party
appointment, term duration, and flag type are not enough to represent modern institutional authority.

Do not expand `GovernmentType` into a giant hard-coded constitution object. Later politics verticals
should define institutions, offices, appointment/election rules, procedures, delegation, and authority
through moddable definitions layered on generic mechanisms.

## Diplomacy

Current country relations are keyed directly by `CountryInstance*` pairs and contain Victoria-specific
influence/sphere semantics.

Do not replace them in this slice. Later diplomacy work should preserve useful bilateral relation
storage while moving stable identity and modern relationship semantics above raw object pointers.

## Test strategy

`A_005_nation_tests.cpp` is currently an empty inherited TestScript shell.

Keep it as the future broad nation/polity scenario acceptance surface. VERTICAL-001 adds focused
compiled unit tests for position and occupancy invariants instead of stuffing low-level identity tests
into that broad script.

## Next migration step

After VERTICAL-001 passes, inspect application/UI usage of legacy player-country selection and connect
`PositionOccupancy` to session bootstrap without removing the legacy country pointer.

Then begin the polity/state-capacity vertical proper by adding the first concrete domain capacity only
when a real subsystem needs it.