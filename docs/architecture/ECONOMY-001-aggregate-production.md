# ECONOMY-001 â€” Aggregate production

The existing OpenVic `ProductionType` remains the recipe definition. ECONOMY-001 introduces a coarse-grained
`AggregateProducer` executor instead of duplicating the recipe ontology.

## Causal path

```text
ProductionType
 + installed capacity
 Ã— utilization
 â†’ desired output
 â†’ input-inventory constraints
 â†’ actual output
 â†’ consume inputs
 â†’ add output inventory
```

Example:

```text
capacity 16 Ã— utilization 0.5 = desired output 8
oil requirement = 2 per output
oil inventory = 12
actual output = 6
oil inventory -> 0
output inventory -> 6
```

## Resolution

The producer represents an industry aggregate at the lowest sufficient resolution: national, regional, or a
strategic firm/facility when systemic importance justifies separate representation.

It does not model an individual generic factory, POP workers, owners, wages, market prices, trade routes, or finance.

## Invariants tested

- capacity/utilization determine desired output;
- scarce inputs constrain actual output;
- tightest input constraint wins;
- inputs are physically consumed;
- output inventory is physically created;
- required-input absence prevents output;
- capacity is nonnegative;
- utilization is clamped to [0,1].

## Next

ECONOMY-002 should bridge aggregate inventories to the existing market-order kernel:

```text
input requirement -> buy request -> acquired inventory
-> production -> output inventory -> sell offer
```

Market accessibility and transport remain separate later mechanisms.