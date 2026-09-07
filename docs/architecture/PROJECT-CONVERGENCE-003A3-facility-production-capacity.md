# PROJECT-CONVERGENCE-003A3 â€” Facility capacity constrains production

Status: third OpenVic industrial-substrate generalization.

## Purpose

Make installed facility capacity causally authoritative over the existing native
production runtime.

## Change

The existing `AggregateProducer` already owns a capacity value and computes
desired output as:

```text
base output Ã— capacity Ã— utilization
```

This milestone does not add another production algorithm.

Instead, the live runtime can bind upstream producer capacity to an inherited
OpenVic `BuildingType` facility at a specific installed level:

```text
BuildingType.capacity_per_level
Ã— installed building level
â†’ AggregateProducer capacity
â†’ desired/actual production
```

The binding is accepted only when:

- the building is a setting-general capacity asset;
- it has a production process;
- that process is the same process owned by the producer;
- the requested level is within the facility's allowed range.

## Proof

The native steel facility was adjusted to two capacity units per level so the
effect is observable against the existing four-unit live scenario.

The proof demonstrates:

```text
facility level 1
â†’ installed capacity 2
â†’ upstream steel output 2

facility level 2
â†’ installed capacity 4
â†’ upstream steel output 4
```

The change is visible on subsequent live production ticks.

## Architectural significance

The OpenVic skeleton now forms a causal chain:

```text
ProductionType / PROCESS
â†’ BuildingType / facility definition
â†’ installed facility level
â†’ AggregateProducer capacity
â†’ production output
â†’ inventory/logistics/market/downstream production
```

This is the intended convergence pattern: reuse and connect inherited OpenVic
systems instead of introducing parallel modern-only owners.

## Scope boundary

This milestone binds one live upstream producer to one facility definition/level
for proof.

It does not yet establish a general world registry of facility instances mapped
to every producer, nor does it model construction input consumption.

## Next

Reconcile the next inherited OpenVic causal input with this chain. Candidates:

- population/labor/workforce constraints;
- construction goods/capital expansion;
- additional processor/facility bindings.

Prefer the connection that removes the most scenario-only authority while
preserving lowest-sufficient-resolution simulation.