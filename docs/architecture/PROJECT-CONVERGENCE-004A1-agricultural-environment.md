# PROJECT-CONVERGENCE-004A1: native agricultural environmental constraint

## Reconnaissance and ownership

`ProvinceInstance` already owns the native RGO, POPs, buildings, ownership,
terrain and state membership. It now owns one `ProvinceEnvironmentalState` by
value. Replacing conditions through `set_environmental_state` uses the existing
province identity; future regional producers can write this same API. It is an
authoritative simulation API, not an actor observation or player information API.

The repository already has `Climate` (an alias of `ProvinceSetModifier`), climate
file loading, terrain modifiers, farm/mine modifiers, and productive-site
industrial-water utility constraints. Those describe different responsibilities.
None is replaced or repurposed into the new physical state. Repository-wide
reconnaissance also found illustrative weather names in ECS documentation, not
a physical weather implementation to reuse.

`Region` groups province definitions. Environmental truth belongs to mutable
province instances rather than those static groupings or another registry.
`ProductionType::get_is_farm_for_non_tech()` exposes the actual `_is_farm` flag;
the mine compatibility accessor can classify every non-farm as a mine and is
therefore deliberately not used to select agricultural production.

## State and formula

The sole variable is fixed-point `water_availability`: water available to
vegetation as a fraction of sufficient water, bounded to `[0, 1]` at construction.
Zero means none; one means sufficient. Default construction is exactly one.
Negative inputs clamp to zero and inputs above one clamp to one, before any
arithmetic. There is no favorable-condition bonus, clock dependency or RNG.

The agriculture bridge computes:

```
yield_factor = water_availability
constrained_output = max(0, unconstrained_output) * yield_factor
```

Multiplication rounds down to the existing fixed-point quantum (1/65536).
Writing `q = max(0, unconstrained_output).raw`, `f = yield_factor.raw`, and
`S = 65536`, the exact raw result is:

```
(q / S) * f + ((q % S) * f) / S
```

Integer division truncates nonnegative operands. This decomposition prevents
intermediate overflow even for `fixed_point_t::max`; the result cannot exceed
the nonnegative input. Neutral conditions preserve nonnegative baseline output
bit for bit. An eight-unit baseline with water availability 0.5 produces four.

This is an architectural first-stage aggregate yield constraint, not calibrated
agronomy. Future inputs may include soil moisture, temperature stress, water
availability, crop-specific sensitivity, weather timing, irrigation, and
disease/pests. This increment does not generate or simulate any of those inputs.

## Native integration and factual explanation

`ResourceGatheringOperation::production_cycle` first executes the unchanged
`produce()` equation, including workforce, size, owner and ordinary modifiers.
Only an RGO-template production type with the native farm flag consumes
`constrain_agricultural_production`. Its constrained physical quantity becomes
`output_quantity_yesterday` before country output reporting and the existing
`MarketInstance::place_market_sell_order`. Market clearing and revenue/payroll
callbacks are unchanged. Mines, unclassified RGOs, factories, artisans and
process producers acquire no environmental constraint from this change.

`get_agricultural_constraint_yesterday()` exposes an optional value containing
the environmental snapshot, yield factor, unconstrained output and constrained
output. It is replaced each production cycle and absent for inactive or non-farm
RGOs, so stale farm facts cannot be mistaken for the current cycle. These facts
are observational snapshots, not another inventory, ledger, producer or graph.

## Initialization and persistence

Every newly constructed province starts neutral, including existing content and
history loading. `apply_history_to_province` does not change the new state;
`initialise_for_new_game` consumes the state already on the instance. There is
no new content key or save-format change. The current `CampaignStateSnapshot`
serializes timeline, commands, RNG streams and ECS identity, not native province
physical/economic state. It therefore does not persist this new province field
either. Any future native-province save support must serialize this authoritative
value and default absent legacy fields to neutral; this increment does not claim
to add a complete world-state save/restore facility.

## Validation

The focused Snitch suite is tagged `[004a1]`, `[convergence]`, `[economy]` and
`[rgo]`. It uses real ProvinceInstance ownership, POP aggregation/hiring, native
RGO production, MarketInstance submission and GoodMarket clearing. It checks a
known eight-unit legacy baseline, physical stress reduction, market supply and
buyer receipts, mine/unclassified immunity, extreme bounds and deterministic
outcomes. It does not introduce test-only production or employment authority.
CMake discovers the new test source automatically; the existing single CTest
entry runs all Snitch cases. Targeted regression tags include B16–B19,
GoodMarket and the economy/productive-site suites.

Validated in `out/build/windows-x64-md`, Debug configuration:

| Selection | Cases | Assertions | Result |
| --- | ---: | ---: | --- |
| 004A1 | 7 | 465 | Pass |
| B19 | 2 | 74 | Pass |
| B18 | 1 | 68 | Pass |
| B17 | 3 | 45 | Pass |
| B16 | 2 | 92 | Pass |
| GoodMarket | 2 | 29 | Pass |
| productive-site | 9 | 545 | Pass |
| economy (includes affected RGO cases) | 93 | 3009 | Pass |
| Full CTest / Snitch | 780 | 3514138 | Pass |

Full command: `ctest --test-dir out/build/windows-x64-md -C Debug --output-on-failure`.
The single registered CTest entry passed, 100%, in 16.88 seconds total.
No test failures were waived.
