# PROJECT-CONVERGENCE-005A2 — Native Capability Composition Proof

## Scope

This increment proves one narrow result from `PROJECT-CONVERGENCE-005A1`: a native game instance can request an optional non-economy capability at runtime construction without making that capability mandatory and without creating a second authoritative owner.

The proof uses the existing position/authority machinery. It does not introduce a universal package framework, plugin system, native geography loader, population schema, or scripting system.

## Native Repository Basis

The authoritative runtime remains owned by `GameManager` and `InstanceManager`.

Before this increment:

- `GameManager::setup_native_instance()` constructed the authoritative `InstanceManager` without loading legacy bookmark/history state.
- `InstanceManager::bootstrap_position_occupancy()` already exposed a pre-session position bootstrap seam.
- `PositionSessionBootstrap::configure()` already validated canonical occupancy, sorted and validated authority grants, registered the authoritative actor profile, and installed position occupancy through existing owners.
- `PlayerManager` remained the owner of position occupancy.
- `AuthorityRegistry` remained the owner of command authority.
- `OrderedCommandRuntime` and `CommandAdmissionRuntime` remained the command-recording/admission machinery.

The existing zero-argument native setup path and bookmark compatibility path are retained.

## External Reference Review

No new substantive physical, economic, demographic, military, or institutional model is introduced by this increment.

This is an engine-composition and ownership change rather than an empirical simulation mechanism, so no external coefficient calibration or scientific model is required.

The applicable design constraints come from the repository architecture:

- preserve a single authoritative owner for each state domain;
- compose native capabilities through existing owners;
- do not create parallel ledgers;
- capability support must not imply mandatory state;
- keep the implementation bounded to evidence demonstrated by the current repository.

## Chosen Mechanism

Two small bootstrap value types are added at the native instance setup boundary:

- `NativePositionBootstrap`
- `NativeInstanceBootstrap`

`NativePositionBootstrap` contains the already-existing inputs required by the position/session bootstrap mechanism:

- `PositionOccupancy`
- a collection of `AuthorityGrant`

`NativeInstanceBootstrap` currently contains an optional position bootstrap.

This is intentionally not an arbitrary capability registry.

The zero-argument:

`GameManager::setup_native_instance()`

remains available and delegates to the bootstrap overload with an empty `NativeInstanceBootstrap`.

The new overload:

`GameManager::setup_native_instance(NativeInstanceBootstrap bootstrap)`

performs the same authoritative runtime construction as before and then applies the requested position capability through:

`InstanceManager::bootstrap_position_occupancy()`

`GameManager` does not directly mutate `PlayerManager`, `AuthorityRegistry`, or command state.

If capability application fails, the newly constructed `InstanceManager` is reset and setup returns false. At this lifecycle boundary that provides atomic behavior to the caller without introducing a general rollback framework.

## Calibration Status

No coefficients, rates, thresholds, probabilities, or empirical parameters are introduced.

Calibration status: not applicable.

## Causal Integration

The composition path is:

native game configuration

→ `GameManager::setup_native_instance(...)`

→ authoritative `InstanceManager` construction

→ `InstanceManager::setup()`

→ optional `InstanceManager::bootstrap_position_occupancy(...)`

→ `PositionSessionBootstrap::configure(...)`

→ authoritative `PlayerManager` occupancy

+ authoritative `AuthorityRegistry` grants

→ existing command admission path.

No duplicate authority, player, or command state is introduced.

## Optionality Proof

### Capability Absent

Calling the existing zero-argument native setup constructs and starts the authoritative runtime with no position occupancy installed.

An occupied-position command therefore remains invalid because no position capability was requested.

The session can still start and end normally.

### Capability Present

A native bootstrap can provide one canonical `PositionOccupancy` plus authority grants.

The position is installed through the existing runtime owner.

A command authorized by the supplied grant is accepted through the existing command-admission machinery.

The same authoritative runtime is then started and ended normally.

### Invalid Capability

An invalid position bootstrap, demonstrated with duplicate grants, is rejected by existing position bootstrap validation.

The containing `GameManager` resets the newly constructed `InstanceManager`.

From the caller's perspective, native instance construction fails atomically:

- no `InstanceManager` remains;
- the game instance is not set up;
- no game session is active.

## Generality Result

005A2 proves that native runtime construction can compose at least one optional non-economy capability without making that capability mandatory for every game.

The engine therefore no longer has only two useful native states:

- empty runtime;
- economy bootstrap plus runtime.

It now also demonstrates the architectural pattern:

common authoritative runtime

+ requested capability state

→ capability-specific existing owner.

This is a bounded proof of capability composition.

It does not yet prove:

- an arbitrary package format;
- arbitrary module discovery;
- dynamic plugins;
- native geography composition;
- arbitrary population dimensions;
- arbitrary political entities;
- script-defined engine mechanisms;
- cross-era content completeness.

Those remain separate convergence problems.

## Scope Boundary

This increment does not add or redesign:

- geography;
- population;
- demographics;
- production;
- markets;
- logistics;
- finance;
- government;
- diplomacy;
- military;
- intelligence;
- environment;
- scripting;
- persistence.

It does not replace `GameManager`, `InstanceManager`, `PlayerManager`, `AuthorityRegistry`, or `PositionSessionBootstrap`.

It does not create a universal capability registry.

## Validation

Implementation validation performed on the authoritative branch:

- branch: `work/live-economy-005-modern-catalog`
- starting HEAD: `e0c406112e78a3f7ed9d9284cff622afc8160605`
- fresh Debug build completed successfully;
- full CTest passed;
- existing native bootstrap behavior remained intact;
- optional-position absence was tested;
- optional-position presence and command admission were tested;
- invalid optional capability atomic failure was tested.

The first build attempt from an ordinary PowerShell session failed because the Microsoft C++ standard-library include environment was not loaded. After importing the Visual Studio 2022 developer environment, the source built successfully. That environment issue was unrelated to the 005A2 implementation.
