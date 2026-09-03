# VERTICAL-003 â€” Fiscal collection capacity

Status: first concrete polity/state-capacity mechanism, connected to a real inherited revenue path.

## Mechanism

Fiscal collection capacity represents only the state's ability to realize legally assessed tax liability.

It contains three separable bottlenecks:

- `tax_base_coverage` â€” taxable activity visible/registered to the fiscal state;
- `compliance_rate` â€” observed liability actually complied with;
- `collection_execution` â€” ability to convert liability into received revenue.

The realization factor is multiplicative:

```text
tax_base_coverage * compliance_rate * collection_execution
```

## Integration

Existing effective tax rate:

```text
legacy tax_efficiency * tax_slider
```

becomes:

```text
legacy tax_efficiency * fiscal_collection_realization * tax_slider
```

All new inputs default to `1.0`, so inherited OpenVic balance/behavior is unchanged until a modern ruleset supplies values.

## Non-goals

This does not govern military readiness, logistics, policing, education delivery, regulation, intelligence,
or infrastructure. Those require their own domain-native mechanisms if strategically needed.

The inherited `administrative_efficiency_from_administrators` is left untouched because it currently feeds
multiple Victoria-era systems and cannot safely be renamed into a modern universal capacity variable.

## Future causal inputs

Modern rulesets may derive these dimensions from registration coverage, informal-sector share, digital
reporting, staffing, audit reach, corruption/leakage, territorial control, war/disaster disruption, and
legal complexity. Those drivers are deliberately not hard-coded here.