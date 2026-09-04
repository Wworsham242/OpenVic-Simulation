# LIVE-ECONOMY-004 â€” Application overlay and real-session smoke

This slice moves the live economy file out of test-only territory and gives
the OpenVic application an explicit application-owned data overlay.

Load precedence is:

```text
Victoria/base root
â†’ OpenVic modern application overlay
â†’ user-selected mods
```

`GameManager::add_application_data_root()` appends the modern root after the
base root without pretending the modern ruleset is a Victoria user mod.

The application overlay currently contains a deliberately transitional
`common/live_economy.txt` that references existing Victoria production
identifiers. Its purpose is to prove the end-to-end application/session load
path. It is not the final modern goods or industry catalog.

The Godot application also exposes `is_live_economy_configured()` as a narrow
smoke-test/presentation seam. Full economy presentation remains a later slice.