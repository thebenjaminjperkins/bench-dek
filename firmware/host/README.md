# Host Source Layout

The host firmware is split by responsibility:

- `ui/` for presentation and input handling
- `applications/` for user-facing tool workflows
- `service-api/` for app-facing capability contracts
- `module-manager/` for discovery, routing, lifecycle, and arbitration
- `module-drivers/` for per-module protocol adapters
- `transport/` for link-level communication and framing

This tree exists to keep policy, transport, and module-specific behavior
separate as the implementation grows.
