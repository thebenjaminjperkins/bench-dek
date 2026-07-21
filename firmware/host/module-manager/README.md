# Module Manager Layer

This directory is for module discovery, provider registration, lifecycle state,
capability routing, and arbitration logic.

It is the host policy layer. It should not own byte framing or per-module wire
details.

The module manager owns:

- discovered module records
- capability provider registration
- provider selection inputs such as preferred module identity
- runtime service-instance ownership and lifecycle state
