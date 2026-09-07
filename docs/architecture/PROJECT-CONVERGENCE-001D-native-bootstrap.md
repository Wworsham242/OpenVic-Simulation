# PROJECT-CONVERGENCE-001D â€” Minimal native instance bootstrap

Status: implementation slice.

## Purpose

Create the first production lifecycle seam that can construct and start the
OpenVic-Simulation authoritative runtime without requiring a Victoria bookmark.

This is deliberately smaller than a complete native ruleset loader. It removes
the bookmark requirement from runtime construction without prematurely
rewriting inherited definition managers.

## Runtime boundary

```text
GameManager
    |
    +-- setup_native_instance()
    |       |
    |       `-- InstanceManager::setup()
    |
    `-- setup_instance(Bookmark)
            |
            +-- setup_native_instance()
            `-- InstanceManager::load_bookmark()
```

Both modes use the same `InstanceManager`. No second authoritative runtime,
scheduler, economy, or world owner is introduced.

## Native definition lifecycle

`InstanceManager` consumes immutable, index-stable definition registries.
Historically those registries were finalized only as a side effect of the
Victoria compatibility loader. Native construction now establishes the same
generic lifecycle directly before runtime instantiation:

- good categories and good definitions are finalized;
- country definitions are finalized;
- province definitions are finalized.

This is not a synthetic Victoria fixture. Empty registries are valid for the
minimal lifecycle proof, and later native ruleset loaders populate these
registries before this finalization boundary.
## What this proves

- `InstanceManager` construction is no longer intrinsically tied to a bookmark.
- A session can start with no bookmark loaded.
- Compatibility startup reuses the native construction path and then applies
  legacy bookmark/history state.
- Single-authority ownership is preserved.

## What this does not yet prove

This slice does not yet provide a self-sufficient modern definition/ruleset
loader. `InstanceManager` still references inherited definition-manager
structures, many of which remain Victoria-shaped.

The next convergence slice should make application-owned native definitions
sufficient to configure this bootstrap without a Victoria II data root.

## Migration rule

Do not delete compatibility loading until the native definition/bootstrap path
has equivalent test coverage. Do not route the native C++ bootstrap through
the Rust `ScenarioHost`; that runtime remains transitional donor/parity
infrastructure during convergence.