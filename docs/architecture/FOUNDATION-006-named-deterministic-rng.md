# FOUNDATION-006 â€” Named deterministic RNG runtime

Status: generalized deterministic RNG runtime established; no gameplay-domain consumption yet.

## Purpose

Turn the FOUNDATION-005 RNG persistence records into an actual deterministic runtime primitive.

## Design

Each stable stream id owns an independent xoroshiro128+ state.

```text
master seed
   |
stable hash(stream id)
   |
SplitMix64 expansion
   |
named xoroshiro128+ stream
```

Examples of future stream names:

- `combat.operational`
- `combat.air`
- `economy.market`
- `politics.domestic`
- `ai.strategic`

The exact names are ruleset/runtime contracts, not hard-coded domain behavior in FOUNDATION-006.

## Why named streams

A draw in one subsystem must not perturb unrelated future outcomes elsewhere. If AI evaluation consumes
500 extra random numbers, the market or combat stream remains bit-for-bit unchanged.

This is essential for:

- deterministic replay;
- debugging;
- save/restore parity;
- parallel execution discipline;
- moddable systems whose internal draw counts may evolve.

## Persistence

The manager captures directly into the canonical `CampaignRngStreamState` records introduced by
FOUNDATION-005. Because the runtime uses `std::map`, capture order is strictly sorted by stream id.

Restore is transactional and rejects:

- empty stream ids;
- duplicate/out-of-order stream ids;
- xoroshiro's invalid all-zero state.

## Explicit non-goals

FOUNDATION-006 does not:

- replace existing gameplay randomness yet;
- permit wall-clock or thread-derived entropy;
- define probability distributions beyond raw 64-bit draws;
- wire RNG into economy, politics, population, military, AI, or events;
- define multiplayer seed negotiation;
- make the generator cryptographically secure.

Those integrations come only after the deterministic primitive is proven.