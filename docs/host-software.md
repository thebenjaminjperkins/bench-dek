# Host Software

This document refines the host responsibilities defined in
[architecture.md](architecture.md).

## Host Responsibilities

The host is responsible for:

- Bootstrapping the system
- Running the user interface and applications
- Discovering modules and tracking their lifecycle
- Exposing a stable service API to applications
- Arbitrating access to shared or exclusive capabilities
- Translating host requests into module-specific operations through drivers
- Managing transport health and degraded behavior

## Layer Breakdown

### UI

The UI renders state and collects user intent. It does not reference module
identifiers, transport details, or driver-specific concepts.

### Applications

Applications define workflows in terms of capabilities. They may subscribe to
streams, issue commands, or request resource reservations through the service
API. When activation is required, the service API may return a handle to a runtime
capability service instance rather than exposing a module directly.

### Service API

The service API is the stable host-facing contract for applications. It should
eventually define:

- Capability discovery queries
- Capability configuration requests
- Capability activation and stop operations
- Capability service instance queries and lifecycle reporting
- Stream subscription management
- Lease and reservation operations
- Error and health reporting

### Module Manager

The module manager owns:

- Module discovery
- Capability registration
- Provider selection
- Capability service instance reuse and compatibility decisions
- Lease enforcement
- Lifecycle state transitions
- Health monitoring at the policy level

It should not own byte framing, checksums, or per-module wire details.

### Module Drivers

Drivers adapt one module family or protocol version into normalized host
operations. They validate compatibility and shield upper layers from
module-specific command shapes.

### Transport

The transport layer owns link-level communication, framing, integrity checks,
timeouts, retries, and disconnect detection.

## Bootstrap Sequence

The intended host bootstrap order is:

1. Initialize platform services.
2. Bring up transport.
3. Enumerate modules.
4. Register capabilities with the module manager.
5. Start the service API and app runtime.
6. Expose module state to the UI.

## Recommended Source Layout

- `firmware/host/ui/`
- `firmware/host/applications/`
- `firmware/host/service-api/`
- `firmware/host/module-manager/`
- `firmware/host/module-drivers/`
- `firmware/host/transport/`

This keeps implementation structure aligned with architectural ownership.
