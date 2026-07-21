# Module Manager Layer

This directory is for module discovery, provider registration, lifecycle state,
capability routing, and arbitration logic.

It is the host policy layer. It should not own byte framing or per-module wire
details.

The module manager knows which modules are associated with which services e.v.v.