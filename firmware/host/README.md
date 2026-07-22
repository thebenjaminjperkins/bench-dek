# Host Source Layout

The host firmware is split by responsibility:

- `app_main.c` for host startup and top-level task bring-up
- `core/` for host-wide runtime plumbing such as system bootstrap, events, and config
- `bringup/` for development-only boot flows, smoke tests, and temporary harnesses
- `ui/` for presentation and input handling
- `applications/` for user-facing tool workflows
- `service-api/` for app-facing capability contracts
- `module-manager/` for discovery, routing, lifecycle, and arbitration
- `module-drivers/` for per-module protocol adapters
- `transport/` for link-level communication and framing
- `tests/` for host-side test code that can be run from bringup paths or dedicated targets

This tree exists to keep policy, transport, and module-specific behavior
separate as the implementation grows.
