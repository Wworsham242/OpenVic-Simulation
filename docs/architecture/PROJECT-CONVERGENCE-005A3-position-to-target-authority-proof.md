# PROJECT-CONVERGENCE-005A3 — Position-to-Target Authority Proof

## Scope

This increment proves that authority-bearing positions cannot admit a target-bearing command until the command's concrete domain target has been resolved and its jurisdiction has been verified.

The proof deliberately separates generic authority mechanics from the legacy OpenVic country/mobilisation implementation.

## Native Repository Basis

The existing authoritative path already provided:

- `PositionOccupancy` as the position/controller boundary;
- `AuthorityRegistry` as the authoritative owner of actor command grants;
- `CommandAdmissionRuntime` as the command admission boundary;
- `OrderedCommandRuntime` as the authoritative accepted-command record;
- `InstanceManager::queue_authorized_legacy_mobilise()` as a bridge from generalized authority to the legacy country mobilisation action;
- `CountryInstanceManager` as the authoritative owner of concrete country instances;
- `LegacyMobiliseCommand` as the deterministic compatibility command/payload definition.

Before 005A3, `queue_authorized_legacy_mobilise()` submitted and recorded generalized authority before proving that the supplied `country_index_t` resolved to the same jurisdiction named by the caller.

## External Reference Review

No new empirical physical, economic, demographic, military-performance, political-behavior, or scientific model is introduced.

005A3 is an authority, identity, and command-admission architecture increment. External coefficient calibration is therefore not applicable.

The controlling architectural requirements are:

- one authoritative owner per state domain;
- concrete domain targets must be resolved by their owning domain;
- generalized authority must remain ontology-light;
- invalid or mismatched commands must not mutate authoritative command history;
- neither Victoria-specific nor modern-world-specific concepts should become universal core ontology.

## Chosen Mechanism

005A3 introduces `CommandTargetIdentity`.

It contains:

- `target_id`;
- `jurisdiction_id`.

The type is deliberately generic. It does not encode:

- country;
- province;
- government;
- military formation;
- firm;
- institution;
- modern state;
- Victoria-era political structure.

`InstanceManager::submit_authorized_targeted_command()` accepts:

- actor identity;
- command type;
- caller-requested jurisdiction;
- already-resolved `CommandTargetIdentity`;
- command payload.

It first verifies that the resolved target is canonical and that its resolved jurisdiction matches the requested jurisdiction.

Only after that check succeeds does it delegate to the existing generalized `submit_authorized_command()` path.

## Legacy Country Adapter

Legacy mobilisation remains a domain-specific compatibility path.

Its adapter performs:

`country_index_t`

→ authoritative `CountryInstanceManager`

→ concrete `CountryInstance`

→ stable country identifier

→ `LegacyMobiliseCommand::resolve_target_identity()`

→ generic `CommandTargetIdentity`.

The generalized authority layer therefore does not learn what a `CountryInstance` is.

The `country:` identity prefix remains part of the legacy/domain adapter rather than the generic target model.

## Calibration Status

Not applicable.

No coefficients, thresholds, probabilities, rates, or empirical constants are introduced.

## Causal Integration

### Inputs

Generic targeted admission consumes:

- actor identity;
- command type;
- requested jurisdiction;
- resolved target identity;
- payload.

The legacy mobilisation adapter additionally consumes:

- `country_index_t`;
- requested mobilised state.

### Mechanism

For legacy mobilisation:

1. Resolve the supplied country index against authoritative `CountryInstanceManager`.
2. Reject an out-of-range/nonexistent concrete target.
3. Convert the real country identity to a generic `CommandTargetIdentity`.
4. Compare the target's authoritative jurisdiction to the caller-requested jurisdiction.
5. Reject mismatch before generalized command admission.
6. Apply existing actor/command/jurisdiction authorization.
7. Record the accepted command through existing `OrderedCommandRuntime`.
8. Queue the existing legacy mobilisation mutation.

### Outputs

Accepted commands retain:

- authoritative actor identity;
- command type;
- resolved jurisdiction;
- deterministic payload.

Rejected target-resolution or target-jurisdiction requests leave accepted command history unchanged.

### Downstream

Successful legacy admission continues into the existing `set_mobilise_argument_t` game-action path.

No second military state, authority ledger, target registry, or command history is created.

### Timing

Target resolution and target-jurisdiction validation occur synchronously before command admission and before legacy action queueing.

### Units

Identity strings and typed indices only. No physical units are introduced.

### Provenance

Accepted targeted commands continue through the existing deterministic ordered-command runtime and simulation-time command record.

### Tests

005A3 proves:

- generic target identity canonical validation;
- generic resolved-target/requested-jurisdiction matching;
- mismatched target jurisdiction is rejected before command-log mutation;
- correctly matched target jurisdiction is admitted and recorded under the resolved jurisdiction;
- legacy country identifiers map through the generic target boundary;
- invalid legacy country indices fail before command admission;
- existing native bootstrap behavior remains intact;
- full regression remains intact.

## Generality Result

The reusable engine rule is now:

domain resolves target

+ target establishes authoritative jurisdiction

+ actor possesses authority over that jurisdiction

→ command may be admitted.

This can later support domains such as:

- geographic locations;
- organizations;
- military formations;
- infrastructure;
- firms;
- institutions;
- political entities;

without requiring the generalized authority core to know their concrete classes.

The test vocabulary intentionally includes generic identifiers such as `target:A` and `jurisdiction:A` so the proof does not accidentally define modern-state concepts as universal engine ontology.

## Victoria-Specific and Modern-Specific Boundary

005A3 does not attempt to generalize all of `CountryInstance`.

Existing OpenVic concepts such as westernisation, strata-specific policy, regiment/ship structures, and other Victoria-oriented state remain separate convergence concerns.

Likewise, modern concepts such as presidents, central banks, corporations, missiles, electricity grids, or modern ministries are not added to the generic target/authority contract.

The core mechanism is only identity resolution plus authority over a resolved jurisdiction.

## Scope Boundary

005A3 does not:

- rewrite `CountryInstance`;
- create a universal target registry;
- create a universal entity ontology;
- redesign military mobilisation;
- change country persistence;
- redesign player control;
- add government systems;
- add geography;
- add population;
- add production;
- add finance;
- add intelligence;
- add modern-world institutions;
- begin 005A4.

## Validation

Validation was performed on:

- branch: `work/live-economy-005-modern-catalog`;
- starting HEAD: `c8e76d43a1780616ba549be7ebf53055abfcf1ec`.

The refined implementation successfully completed:

- Debug build;
- targeted 005A3 authority/target tests;
- native bootstrap regression tests;
- full CTest regression suite;
- `git diff --check`.

The new target-aware mechanism preserves existing command-admission ownership and keeps country knowledge inside the legacy/domain adapter.
