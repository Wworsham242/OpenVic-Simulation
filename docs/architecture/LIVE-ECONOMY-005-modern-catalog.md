# LIVE-ECONOMY-005 â€” Modern catalog foundation

The first application-owned modern chain is:

```text
modern_iron_ore
    â†“
modern_primary_steel
    â†“
modern_industrial_machinery
```

It deliberately covers a raw material, an industrial intermediate, and a
capital good without attempting to define the final global goods catalog.

Modern goods are preloaded before the legacy `common/goods.txt`, after which
the existing goods loader performs its normal one-time registry lock. This
keeps the modern catalog additive instead of replacing Victoria's full goods
file or reopening a finalised registry.

Modern production recipes are appended after the legacy production file.
`ProductionType::template_type_t::AGGREGATE` is introduced so generalized
industry-region recipes do not masquerade as Victoria factories, RGOs, or
artisans. Aggregate processes have no owner/job actor semantics.

The OpenVic application overlay now owns:

- `common/modern_goods.txt`
- `common/modern_production_types.txt`
- `common/live_economy.txt`

`live_economy.txt` resolves the modern identifiers directly.

Do not immediately explode the goods list. Prefer visible economy presentation
or a second supply chain only when it exercises a missing causal mechanism.