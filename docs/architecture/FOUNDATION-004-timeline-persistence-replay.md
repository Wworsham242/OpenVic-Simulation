# FOUNDATION-004 â€” Timeline persistence and replay continuity

Status: durable generalized-time state proven; no campaign file format yet.

This slice adds a canonical `SimulationTimelineSnapshot` containing:

- schema version;
- current generalized `SimTime`;
- next event sequence cursor;
- canonically ordered scheduled events and payloads.

The snapshot is a persistence DTO, not a serializer. It keeps the simulation core independent of JSON,
binary archives, Godot resources, or any future campaign container.

The checksum is architecture-independent: fixed FNV-1a, explicit integer byte order, explicit
string/byte-array lengths, canonical event order, and no pointer/padding/std::hash state.

Restore is transactional. Unsupported schema versions or noncanonical scheduler state are rejected
without mutating the live timeline.

Replay tests checkpoint one timeline, restore another, advance both independently, and require
identical future delivery, terminal state, checksums, and post-restore sequence allocation.

Not included yet: full campaign package, ECS serialization, domain stores, command log, RNG streams,
bookmark wiring, Godot save UI, or gameplay event dispatch.