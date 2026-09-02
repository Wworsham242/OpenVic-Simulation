# FOUNDATION-005 â€” Campaign state composition

Status: first generalized campaign-state envelope; composition only.

## Purpose

Compose proven durable primitives into one authoritative campaign snapshot without prematurely
migrating gameplay domains or choosing a physical save-file format.

```text
CampaignStateSnapshot
â”œâ”€â”€ SimulationTimelineSnapshot
â”œâ”€â”€ named deterministic RNG stream records
â”œâ”€â”€ command/replay position
â”œâ”€â”€ ECS WorldIdentitySnapshot
â””â”€â”€ schema/checksum metadata
```

## Existing OpenVic behavior reused

`WorldIdentitySnapshot` is reused directly. It preserves stable entity identity, generations,
immutability, and free-list order while deliberately excluding archetype/chunk packing. This is
exactly the right separation for a generalized campaign package.

## New records are storage contracts, not runtime implementations

`CampaignRngStreamState` defines where named deterministic RNG stream state belongs. It does not yet
choose or alter the runtime PRNG algorithm.

`CampaignReplayState` defines an authoritative accepted-command count and replay cursor. It does not
yet replace `GameAction`, define authority validation, or persist a full command log.

Those behaviors will be wired in later foundation slices.

## Canonicality

A campaign envelope is canonical only when:

- schema versions are supported;
- RNG stream identifiers are non-empty and strictly sorted/unique;
- replay cursor does not exceed accepted command count;
- ECS free-list indices are unique and in range;
- ECS slot generations are valid.

Full ECS restore validation remains owned by `World::restore_identity`.

## Checksum

The campaign checksum deterministically composes timeline, RNG stream, replay, and ECS identity state.
Future domain checksums will be appended as authoritative stores enter the campaign package.

## Deliberately not included yet

- physical file serialization;
- runtime RNG manager;
- full ordered command log;
- command authority/jurisdiction mechanics;
- component payload serialization;
- economy/population/finance/logistics/military state;
- Godot save UI;
- bookmark replacement.

FOUNDATION-005 establishes the stable container those systems will plug into.