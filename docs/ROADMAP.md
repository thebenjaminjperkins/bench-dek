# Short-Term Roadmap

## Focus

The short-term goal is to replace the architecture-only phase with one real,
end-to-end vertical slice built around the `uart.stream` capability.

At the end of this phase, DeK should be able to:

1. Boot through a host-oriented entrypoint instead of the current test program.
2. Enumerate one reference module over the v1 control plane.
3. Discover a `uart.stream` capability from that module.
4. Open, configure, start, and stop one UART service instance.
5. Deliver UART data through a normalized host-facing path.

This phase is intentionally narrow. The goal is to prove the architecture with
one capability, one driver path, and one reference module before expanding into
multi-module policy, richer UI work, or additional capabilities.

## Deliverables

The short-term phase should produce these concrete outputs:

1. A written `uart.stream` capability contract.
2. A defined set of control-plane payload schemas for enumeration and session
   setup.
3. A layered host bootstrap that replaces the throwaway `main.c` behavior.
4. A transport skeleton that can exchange valid DeK packets over SPI.
5. A minimal module manager and service API path for one capability.
6. One module driver for a reference UART-capable module.
7. A reference module implementation or stub that can enumerate and answer basic
   UART capability requests.
8. An end-to-end validation path that proves the host can discover and use the
   UART slice.

## Workstreams

### 1. Finalize the UART Capability Contract

Define the first capability as a real, versioned contract rather than as an
architectural example.

Work to complete:

- Choose the initial capability identifier and version for `uart.stream`.
- Define required configuration fields such as baud rate, parity, stop bits,
  data bits, and flow-control behavior.
- Define the supported command set for the capability, including configuration,
  start, stop, transmit, and status-related operations if they are part of v1.
- Define stream or event payload types for received UART data and status
  changes.
- Define resource policy for the capability, including whether it is exclusive,
  shareable, or reservable.
- Define capability-level errors and limits such as maximum baud rate, buffer
  depth, and backpressure behavior.

Deliverables:

- A dedicated capability spec document for `uart.stream`.
- A list of open questions that need explicit decisions rather than implicit
  assumptions.

Exit criteria:

- Another engineer can implement host and module behavior from the spec without
  inventing missing UART semantics.

### 2. Lock the Control Plane Payloads Needed for the UART Slice

The transport envelope is already defined, but the host and module still need
shared payload schemas for the control path.

Work to complete:

- Define payload fields for `HELLO`, `HELLO_ACK`, `GET_DESCRIPTOR`, and
  `DESCRIPTOR`.
- Define payload fields for `GET_CAPABILITIES` and `CAPABILITIES`, including
  pagination expectations.
- Define payload fields for `OPEN_CAPABILITY`, `OPEN_ACK`,
  `CLOSE_CAPABILITY`, and `CLOSE_ACK`.
- Define how capability IDs, versions, channel IDs, and configuration blobs are
  represented on the wire.
- Define `ERROR` payload structure and the subset of error classes required for
  the UART slice.

Deliverables:

- Updated interface documentation that makes enumeration and capability session
  setup unambiguous.

Exit criteria:

- Host and module implementers can build matching packet payloads for
  enumeration and session open/close without needing verbal clarification.

### 3. Replace the Test Entrypoint with a Layered Host Bootstrap

The current `firmware/main.c` is a temperature-sensor demo and should be
replaced with a production-shaped startup path.

Work to complete:

- Define the startup sequence around platform init, transport init, module
  enumeration, capability registration, service API startup, and app startup.
- Move startup ownership into the host-oriented directory structure rather than
  continuing to grow the temporary test entrypoint.
- Establish a small set of host bootstrap responsibilities and interfaces
  between layers.
- Add enough logging or status reporting to make startup failures diagnosable.

Deliverables:

- A host bootstrap design note or implementation checklist.
- A replacement plan for removing the temporary test behavior from `main.c`.

Exit criteria:

- The firmware startup path clearly reflects the layered architecture and no
  longer treats `main.c` as the long-term home for feature logic.

### 4. Build the Transport and Enumeration Skeleton

The next milestone is a minimal transport that can move valid DeK packets and
support one-module enumeration.

Work to complete:

- Define packet encode and decode behavior for the v1 header and CRC trailer.
- Implement or plan sequence number ownership, request matching, and channel `0`
  handling.
- Define timeout behavior for immediate and deferred responses.
- Define the minimal handling needed for `ATTN`, `RESET`, `POLL`, and `EMPTY`.
- Support the enumeration flow through `HELLO`, descriptor fetch, and capability
  manifest fetch.

Deliverables:

- A transport checklist or implementation plan tied to the v1 packet protocol.
- A documented enumeration sequence for one reference module.

Exit criteria:

- The host can complete a clean enumeration pass against one well-behaved
  module using the agreed packet contract.

### 5. Implement a Minimal Service API and Module Manager for One Capability

The architecture calls for a service API and module manager, but the short-term
goal is only the smallest useful subset.

Work to complete:

- Represent one registered module and one advertised capability.
- Add provider selection logic for the single-provider case.
- Define the host-facing request path for opening, configuring, starting,
  stopping, and closing one UART capability service instance.
- Define how service instance state is tracked and reported.
- Normalize capability failures so upper layers do not depend on raw transport
  errors.

Deliverables:

- A service lifecycle outline for one capability service instance.
- A minimal provider registry and service-handle model for the UART slice.

Exit criteria:

- The application-facing side of the host can request `uart.stream` without
  knowing anything about module-specific transport details.

### 6. Add the First Module Driver and Reference Module

The first driver should prove the normalization boundary between host policy and
module-specific behavior.

Work to complete:

- Define one reference module family or module personality for the UART slice.
- Implement a host driver that translates normalized host requests into that
  module's capability-scoped commands.
- Implement a reference module stub or firmware path that returns a descriptor,
  capability manifest, and UART session behavior.
- Validate capability version compatibility and basic session open/close rules.

Deliverables:

- One host driver for the reference UART module.
- One module-side reference implementation or stub for enumeration and UART
  session behavior.

Exit criteria:

- The host can discover the reference module, open `uart.stream`, and exchange
  UART-related control and data messages through the driver boundary.

### 7. Prove the Vertical Slice End to End

The short-term phase is complete only when the pieces work together as one
coherent path.

Work to complete:

- Stand up a minimal host-side consumer for the UART capability. This can be a
  lightweight app shell, a diagnostic harness, or a temporary validation path.
- Verify enumeration, capability open, UART configuration, start, data flow,
  stop, and close.
- Capture known limitations and failure cases observed during bring-up.
- Record the follow-up work needed before the UART slice is considered robust.

Deliverables:

- A bring-up checklist or validation notes for the UART vertical slice.
- A post-bring-up list of follow-on tasks for reliability, UX, and scalability.

Exit criteria:

- There is one demonstrable path from host boot to live UART data using the
  DeK architecture rather than the temporary test program.

## Recommended Order

The work should proceed in this order:

1. Finalize the UART capability contract.
2. Lock the control-plane payloads needed for enumeration and session setup.
3. Replace the test entrypoint with a host bootstrap plan.
4. Build the transport and enumeration skeleton.
5. Add the minimal service API and module manager path.
6. Add the first module driver and reference module.
7. Validate the end-to-end UART vertical slice.

This order keeps the project from writing implementation code against unstable
contracts while still moving steadily toward a working system.

## Out of Scope for This Phase

The following work is important, but it should not block the first UART slice:

- Multi-module arbitration and provider preference logic beyond the single-path
  case
- Rich UI work beyond a minimal validation surface
- Additional capabilities such as GPIO, SPI capture, or logic analysis
- Advanced degraded-mode handling beyond the minimum needed for bring-up
- Reusable component extraction and cleanup that can wait until the first slice
  is proven
