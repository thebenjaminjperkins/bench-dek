# Architecture

DeK is organized as a host and one or more modules. The host owns user interaction,
application execution, module discovery, scheduling, and policy. Modules extend the
system by providing hardware-backed capabilities such as UART, logic analysis, GPIO,
or protocol-specific instrumentation.

The primary architectural goal is that applications depend on stable capabilities,
not on specific modules, pins, or transport details.

## Principles

1. The host is the source of truth for system state.
2. Applications are capability-driven and module-agnostic.
3. Hardware access is centralized behind a small number of layers.
4. Modules are replaceable as long as they honor the capability contract.
5. Failure of one module must degrade service gracefully rather than destabilize the system.

## Host Architecture

The host is divided into six layers. Each layer has a single responsibility and may
only depend on the layer directly below it.

### 1. User Interface

The user interface renders screens, captures user intent, and displays state. It does
not talk to modules directly and does not contain module-specific logic.

Responsibilities:
- Present navigation, status, warnings, and app output.
- Translate user actions into application commands.
- Show degraded or unavailable capability states clearly.

### 2. Applications

Applications are the user-facing tools that run on the DeK, such as a UART terminal,
logic analyzer, bus sniffer, or GPIO monitor.

Applications:
- Request capabilities from the service API.
- Interpret returned data and transform it into app-specific behavior.
- Manage app workflow, recording, decoding, and presentation rules.
- Remain agnostic to which module provides a capability.

Example:
A UART terminal asks for a `uart.stream` capability. It does not need to know which
module owns the UART peripheral or which physical pins are in use. It binds to the
capability contract exposed by the host.

### 3. Service API

The service API is the boundary between applications and the module subsystem. It
provides a stable, typed contract for requesting, configuring, and consuming module
capabilities.

The service API supports three interaction styles:
- Command/response for control operations.
- Subscription streams for continuous data.
- Reservations for exclusive or timing-sensitive resources.

Applications never issue raw module commands. They operate strictly through this API.

### 4. Module Manager

The module manager is the policy and orchestration layer between apps and modules. It
is the only part of the host allowed to coordinate module access.

Responsibilities:
- Discover connected modules and track their lifecycle.
- Build a registry of available capabilities.
- Resolve application requests to specific module providers.
- Enforce arbitration, leases, and sharing rules.
- Track health, timeouts, and degraded states.
- Route high-level capability requests to the correct driver.

The module manager should not contain transport framing logic or per-module protocol
quirks. Those belong in lower layers.

### 5. Module Drivers

Module drivers are adapters between normalized host requests and module-specific
command sets. Each driver understands one module family or one protocol version.

Responsibilities:
- Parse module descriptors during enumeration.
- Validate module firmware and capability compatibility.
- Translate service API calls into module commands.
- Convert raw module responses into normalized host data structures.
- Hide module-specific command details from the module manager.

This layer allows the system to support multiple module implementations for the same
capability without leaking module-specific logic into applications.

### 6. Communication Layer

The communication layer owns the byte-level link between host and modules.

Responsibilities:
- Detect module presence and loss.
- Frame packets and validate integrity.
- Assign sequence numbers for request/response matching.
- Enforce timeouts, retries, and link reset behavior.
- Deliver bytes to the correct driver without exposing physical transport details upward.

This layer should be transport-aware but capability-agnostic. It knows how to move
messages, not what the messages mean.

## Module Architecture

Each module is a self-contained hardware extension that implements one or more
capabilities. A module is responsible for local hardware control, sampling, signal
generation, or protocol work that the host requests.

At minimum, a module must expose:
- A stable module identifier.
- Hardware revision and firmware revision.
- A capability manifest.
- Health and fault status.
- A command endpoint compatible with the host communication layer.

Modules should not assume that a single app is their only client. The host may route
multiple requests through the same module over time.

## Capability Model

Capabilities are the core abstraction of the system. A capability is a host-visible
service contract, not a raw hardware block.

Each capability must define:
- Capability ID, such as `uart.stream` or `gpio.digital`.
- Capability version.
- Supported commands.
- Event or stream types, if any.
- Required configuration parameters.
- Resource policy: shareable, exclusive, or reservable.
- Performance limits, such as max sample rate or buffer depth.

The module manager matches apps to capabilities, not to modules directly. A single
module may expose many capabilities, and multiple modules may expose the same one.

## Resource Ownership and Arbitration

Contention is inevitable, so the architecture defines ownership rules up front.

Rules:
- Physical hardware is owned by the providing module.
- Capability routing is owned by the module manager.
- Presentation and workflow are owned by the application.
- Exclusive capabilities require a lease before activation.
- Shareable capabilities may have multiple subscribers if the module declares support.

If multiple modules provide the same capability, provider selection follows this order:
1. Existing user-selected preference.
2. A module already holding compatible app state.
3. Highest compatible capability version.
4. Deterministic fallback, such as lowest slot number or lowest module ID.

This keeps provider selection predictable and debuggable.

## Module Lifecycle

Every module moves through explicit states:

1. `Detected`
2. `Enumerating`
3. `Ready`
4. `Reserved`
5. `Streaming`
6. `Degraded`
7. `Offline`

State transitions are owned by the module manager. Applications may observe module
state, but they do not control it directly.

Hot-plug must be treated as a normal event. If a module disappears while in use, the
manager tears down affected leases, notifies dependent applications, and returns the
system to a safe state without crashing unrelated apps.

## Failure Handling

The architecture must assume unreliable links, incompatible firmware, and transient
module faults.

Required behavior:
- Invalid descriptors cause the module to be quarantined.
- Unsupported capability versions are rejected during enumeration.
- Timed-out commands are retried only by the communication layer.
- Repeated transport failure moves the module to `Degraded` or `Offline`.
- Application-visible errors are reported as capability failures, not raw transport noise.
- One failing module must not block unrelated modules from operating.

The host should prefer explicit degraded behavior over silent failure.

## Request Flow

The normal flow is:

1. The user interacts with the UI.
2. The application decides which capability it needs.
3. The application requests the capability through the service API.
4. The module manager selects a provider and allocates any required lease.
5. The module driver translates the request into module-specific commands.
6. The communication layer exchanges packets with the module.
7. The response is normalized back up the stack and rendered by the application.

This keeps hardware, policy, and presentation responsibilities cleanly separated.

## Architectural Boundaries

To keep the design maintainable, the following boundaries are strict:

- UI must not reference module transport or module IDs directly.
- Applications must not encode module-specific command sets.
- Module manager must not own byte framing or checksum logic.
- Drivers must not contain UI policy.
- Modules must not assume host application behavior beyond the defined capability contract.

Violating these boundaries will create tight coupling and make modules harder to evolve.

## Summary

The UI captures intent and displays results.
Applications express user workflows in terms of capabilities.
The service API provides a stable contract for apps.
The module manager discovers modules, arbitrates access, and routes requests.
Module drivers translate normalized requests to module-specific behavior.
The communication layer moves validated messages across the physical link.
Modules perform the actual hardware work.
