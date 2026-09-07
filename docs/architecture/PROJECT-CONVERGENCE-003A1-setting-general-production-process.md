# PROJECT-CONVERGENCE-003A1 â€” Setting-general production process

Status: first OpenVic industrial-substrate generalization.

## Purpose

Use the inherited OpenVic `ProductionType` as the native setting-general process
definition instead of creating a parallel modern industrial-process class.

## Change

`ProductionType::template_type_t` now has canonical `PROCESS` semantics.

`AGGREGATE` remains a compatibility alias to `PROCESS` for convergence-era
content.

Native process loading continues to reuse the inherited OpenVic fields:

- input goods;
- maintenance requirements;
- output good;
- base output quantity;
- workforce scale.

Victoria owner/job semantics remain available for legacy FACTORY/RGO content,
but setting-general PROCESS content does not require those actor assumptions.

## Causal significance

This means the modern engine can grow industrial detail by extending the same
OpenVic process definition rather than layering an unrelated production model.

The intended path is:

```text
OpenVic ProductionType
â†’ general setting-neutral process definition
â†’ facilities/capacity
â†’ labor/skills
â†’ inventories/logistics
â†’ markets
â†’ downstream production
```

## Proof

The native ore-to-steel process is loaded as a `PROCESS`, retains physical input
requirements, uses inherited maintenance-requirement machinery, produces the
expected output good, and remains compatible with the existing native runtime.

## Next

Generalize OpenVic building machinery into setting-general facilities/capacity
assets while preserving Victoria compatibility.