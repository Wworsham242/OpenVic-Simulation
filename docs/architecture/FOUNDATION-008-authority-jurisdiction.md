# FOUNDATION-008 â€” Authority and jurisdiction admission

Status: generalized command-admission authority primitive established.

## Purpose

Insert a deterministic validation layer in front of FOUNDATION-007's accepted-command log.

```text
human / AI / institution / command / script
                  |
               intent
                  |
      actor + command + jurisdiction
                  |
          AuthorityRegistry
            /           \
      reject           authorize
                         |
                OrderedCommandRuntime
                         |
                 accepted command log
```

Authority is represented by explicit `(actor_id, command_type, jurisdiction_id)` grants.

The primitive is intentionally generic. It does not hard-code presidents, parliaments, central banks,
NATO, corporations, or military echelons. Those are ruleset definitions layered above this contract.

Human and AI controllers can receive equivalent authority and use the same admission path.

Accepted command records now persist `jurisdiction_id`, and campaign checksums cover that field.

Non-goals for FOUNDATION-008 include institutional procedure, votes, vetoes, delays, delegation,
emergency powers, political legitimacy, actor lifecycle, command modification, and legacy GameAction translation.