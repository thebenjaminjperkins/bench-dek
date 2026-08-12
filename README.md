# DeK

DeK is an extensible embedded tool platform built around a host and hot-pluggable
modules. The host provides the user experience, application runtime, capability
routing, and system policy. Modules provide hardware-backed services such as
UART, GPIO, protocol analysis, and other instrumentation features.

The repository is transitioning from architecture-first scaffolding to a
working, host-side GPIO vertical slice. The ESP-IDF entrypoint is now a thin
handoff into the host bootstrap. The current slice uses a simulated reference
module so the host stack can be exercised before physical SPI hardware and
separate module firmware are ready.

## Repository Layout

- `docs/` contains the system vision, architecture, interface contracts, and
  design decisions.
- `firmware/` contains firmware source organized by architectural layer.
- `firmware/host/tests/fixtures/` contains host-local simulated modules used
  only for tests and bringup.
- `external/DeK-Protocol/` is the shared, platform-independent packet,
  transport, control-plane, and capability-contract library.
- `components/` is reserved for reusable ESP-IDF components shared across the
  firmware.
- `build/` contains generated build artifacts and is not source of truth.

## Architectural Direction

The project is designed around these core ideas:

1. Applications depend on capabilities, not on specific modules.
2. Module access is mediated by the host through explicit service contracts.
3. Transport details, module-specific commands, and user-facing workflows are
   kept in separate layers.
4. Hot-plug, failure handling, and capability arbitration are first-class design
   concerns.

For the current architecture, start with:

- [Architecture](docs/architecture.md)
- [Host Software](docs/host-software.md)
- [Module Interface](docs/module-interface.md)
- [Vision](docs/vision.md)

## Current Status

The current implementation proves the host-side layers with `gpio.digital`:

- `main.c` hands off to `host/app_main.c` and `host/core/bootstrap.c`.
- The bringup runtime runs host unit tests and a GPIO vertical-slice harness.
- The harness performs HELLO, descriptor and capability discovery, service
  open, GPIO mode/write/read commands, and service close against a simulated
  reference module.
- The host uses the shared sources in `external/DeK-Protocol`; it does not
  duplicate packet or control-plane definitions.
- The physical SPI adapter, a deployable reference-module firmware project,
  and real-hardware validation are still outstanding.

See the [GPIO vertical-slice roadmap](docs/gpio-vertical-slice-roadmap.md) for
the continuation plan. The existing UART documents remain the design target for
the next capability, not a description of the current executable slice.
