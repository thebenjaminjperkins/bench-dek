# DeK

DeK is a modular embedded tool platform built around a host and hot-pluggable
modules. The host provides the user experience, application runtime, capability
routing, and system policy. Modules provide hardware-backed services such as
UART, GPIO, protocol analysis, and other instrumentation features.

This repository is currently organized around the target architecture rather than
around a large implementation surface. The firmware entrypoint remains a test
program for now; the surrounding structure reflects the intended production
design.

## Repository Layout

- `docs/` contains the system vision, architecture, interface contracts, and
  design decisions.
- `firmware/` contains firmware source organized by architectural layer.
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

The repo is in an architecture-first phase:

- The firmware tree has been shaped to match the intended host/module split.
- Architectural support docs are being written before major implementation work.
- `firmware/main.c` is still a throwaway test entrypoint and should not be used
  as the long-term structure reference.
