# FOUNDATION-010 â€” Authorized mobilise legacy bridge

Status: first existing gameplay action is admitted through the generalized authority path before legacy execution.

## Selected action

The proof uses the existing `set_mobilise` GameAction.

This was chosen instead of pause or speed because mobilization is a sovereign military decision.

## New path

```text
actor
  |
  | jurisdiction
  | target legacy country_index
  | requested mobilised state
  v
InstanceManager::queue_authorized_legacy_mobilise()
  |
  v
LegacyMobiliseCommand::encode()
  |
  v
CommandAdmissionRuntime
  |
  +-- reject ------------------------------> no GameAction queued
  |
  `-- accept
        |
        +--> durable ordered command log
        |
        `--> queue_game_action<set_mobilise_argument_t>()
                    |
                    v
             existing GameAction queue
                    |
                    v
             existing GameAction visitor
                    |
                    v
             CountryInstance::set_mobilised()
```

## Compatibility payload

Schema v1 is five bytes:

- uint32 `country_index_t`, little-endian;
- one byte boolean requested mobilization state.

This is a compatibility encoding, not the final modern military command schema.

## Jurisdiction vs legacy country index

The generalized authority system uses opaque jurisdiction IDs. Legacy OpenVic actions use numeric
`country_index_t`.

FOUNDATION-010 records both but does not claim they are semantically identical. A later ruleset/actor
resolution layer must map stable geopolitical jurisdiction identities to runtime country instances.

## Atomicity note

The bridge rejects submission while legacy GameActions are currently executing. After that guard,
authorization is recorded before queueing the legacy action.

At present the legacy queue has no ordinary failure mode after that guard. If future queue semantics
introduce one, command-log/queue atomicity must become an explicit transaction.

## Migration rule proven

A legacy mechanic can be migrated incrementally:

```text
new authority/admission semantics
        +
old proven mutation implementation
```

## Next

After this bridge passes, foundation surgery should be considered nearly complete.

The next work should be a short reconciliation pass covering persistence composition of the live
command runtime, actor/jurisdiction bootstrap ownership, truth/perceived-state seam, and transaction
boundary rules. Then begin vertical modernization rather than continuing an indefinite foundation series.