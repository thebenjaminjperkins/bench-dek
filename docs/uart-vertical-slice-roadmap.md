# UART Vertical Slice Sub-Roadmap

This document breaks the short-term roadmap into file-level outputs for the
`uart.stream` vertical slice.

It is written as a project execution guide. Each section lists:

- the files to create or update
- the purpose of those files
- the deliverables that should exist when the section is done

The filenames below are recommended rather than mandatory, but keeping them
close to this plan will make the repo easier to navigate as the architecture
turns into implementation.

## Working Assumption

This plan assumes the first reference slice includes:

- one host
- one reference module
- one capability: `uart.stream`
- one minimal host-side validation path

It does not assume a polished UI or multi-module behavior yet.

## 1. Finalize the UART Capability Contract

### Files to Create or Update

- Create `docs/capabilities/uart-stream.md`
  Purpose: the source of truth for the `uart.stream` contract.
- Update `docs/architecture.md`
  Purpose: keep the architecture examples aligned with the real first
  capability if any wording needs to become less hypothetical.
- Update `docs/ROADMAP.md`
  Purpose: reflect any narrowed scope or clarified milestones after the UART
  contract is locked.
- Create `docs/sketches/uart-stream-open-questions.md`
  Purpose: hold unresolved design questions separately from the normative spec.

### What the Capability Spec Should Contain

- Capability ID and version
- Capability purpose and non-goals
- Configuration fields
- Supported commands
- Stream and event payload types
- Error model
- Resource policy
- Performance limits
- Compatibility expectations

### Deliverables

- A complete `uart.stream` capability specification
- A separate open-questions list
- A short list of decisions that may need a future ADR if they become
  architecture-level choices

### Done When

- Someone else could implement host and module UART behavior from the document
  without guessing at the meaning of core UART operations

## 2. Lock the Control Plane Payloads for the UART Slice

### Files to Create or Update

- Update `docs/module-interface.md`
  Purpose: turn the current transport and message inventory into field-level
  payload schemas for the packets needed by the UART slice.
- Create `docs/sketches/control-plane-payloads-v1.md`
  Purpose: stage payload tables, examples, and notes before folding stable
  parts into the canonical interface doc.
- Update `docs/decisions/0004-standardize-control-plane-and-capability-commands.md`
  Purpose: only if the UART work exposes a missing policy boundary around
  control-plane versus capability-scoped commands.

### What the Interface Work Should Produce

- Payload fields for `HELLO` and `HELLO_ACK`
- Payload fields for `GET_DESCRIPTOR` and `DESCRIPTOR`
- Payload fields for `GET_CAPABILITIES` and `CAPABILITIES`
- Payload fields for `OPEN_CAPABILITY` and `OPEN_ACK`
- Payload fields for `CLOSE_CAPABILITY` and `CLOSE_ACK`
- A defined `ERROR` payload shape for the slice
- Rules for representing capability IDs, versions, and configuration blobs

### Deliverables

- A field-level control-plane payload definition for the UART slice
- At least one worked request/response example for enumeration
- At least one worked request/response example for opening `uart.stream`

### Done When

- Host and module implementers can build compatible control-plane packets from
  the docs alone

## 3. Replace the Test Entrypoint with a Layered Host Bootstrap

### Files to Create or Update

- Update `firmware/main.c`
  Purpose: reduce it to a thin startup handoff instead of feature logic.
- Create `firmware/host/bootstrap.h`
  Purpose: expose the host bootstrap entrypoint.
- Create `firmware/host/bootstrap.c`
  Purpose: own startup sequencing for the host runtime.
- Update `firmware/README.md`
  Purpose: explain that startup now flows through the host bootstrap.
- Update `docs/host-software.md`
  Purpose: document any bootstrap details that become concrete during this work.
- Create `docs/sketches/host-bootstrap-sequence.md`
  Purpose: capture startup order, responsibilities, and failure points.

### What the Bootstrap Work Should Cover

- platform initialization
- transport initialization
- module enumeration startup
- capability registration startup
- service API startup
- application or validation harness startup
- diagnostic logging checkpoints

### Deliverables

- A documented host bootstrap sequence
- A clear division of startup responsibilities by layer
- A thin `main.c` that no longer acts as the project's real implementation home

### Done When

- The startup path visibly follows the architecture instead of the old demo
  structure

## 4. Build the Transport and Enumeration Skeleton

### Files to Create or Update

- Create or adopt `external/DeK-Protocol/include/dek_transport.h`
  Purpose: define the shared transport-facing protocol interface used by host
  and module builds.
- Create or adopt `external/DeK-Protocol/src/dek_transport.c`
  Purpose: implement shared packet assembly and transport request helpers.
- Create or adopt `external/DeK-Protocol/include/dek_packet.h`
  Purpose: define shared packet structures, constants, and helpers.
- Create or adopt `external/DeK-Protocol/src/dek_packet.c`
  Purpose: implement shared packet encode, decode, and validation behavior.
- Create or adopt `external/DeK-Protocol/include/dek_crc.h`
  Purpose: define CRC helpers used by the packet layer.
- Create or adopt `external/DeK-Protocol/src/dek_crc.c`
  Purpose: implement shared CRC16-CCITT support for DeK packets.
- Keep `firmware/host/transport/README.md`
  Purpose: reserve the host-only transport layer for SPI/ATTN/RESET integration
  that sits above the shared protocol library.
- Create `docs/sketches/enumeration-sequence.md`
  Purpose: record the one-module enumeration flow step by step.
- Update `docs/module-interface.md`
  Purpose: reconcile any transport details that become concrete during bring-up.

### What the Transport Skeleton Should Cover

- packet layout constants
- sequence number ownership
- channel `0` handling
- `POLL` and `EMPTY` behavior
- `ATTN` and `RESET` handling expectations
- timeout and retry policy boundaries
- descriptor and capability fetch flow

### Deliverables

- A transport module plan or implementation scaffold
- A packet-layer definition aligned with the v1 protocol
- A written enumeration flow for the reference module

### Done When

- The host transport layer can support a full control-plane enumeration pass for
  one module

## 5. Implement a Minimal Service API and Module Manager for One Capability

### Files to Create or Update

- Create `firmware/host/service-api/service_api.h`
  Purpose: define the host-facing capability request interface.
- Create `firmware/host/service-api/service_api.c`
  Purpose: implement the minimal app-facing service path.
- Create `firmware/host/module-manager/module_registry.h`
  Purpose: define registered module and capability records.
- Create `firmware/host/module-manager/module_registry.c`
  Purpose: implement module and capability registration.
- Create `firmware/host/module-manager/service_instance.h`
  Purpose: define the runtime representation of one capability service instance.
- Create `firmware/host/module-manager/service_instance.c`
  Purpose: implement service instance lifecycle tracking.
- Create `docs/sketches/service-lifecycle-uart.md`
  Purpose: show open, configure, start, run, stop, and close behavior.
- Update `docs/host-software.md`
  Purpose: capture any newly concrete host responsibilities or lifecycle rules.

### What the Service API and Manager Work Should Cover

- registering one module
- registering one capability
- selecting the only available provider
- opening one UART capability service instance
- tracking runtime state
- surfacing normalized errors upward

### Deliverables

- A minimal service API contract for one capability
- A provider registry model for the UART slice
- A service instance lifecycle definition

### Done When

- Upper host layers can ask for `uart.stream` without dealing with module IDs or
  transport details

## 6. Add the First Module Driver and Reference Module

### Files to Create or Update

- Create `firmware/host/module-drivers/uart_reference_driver.h`
  Purpose: define the host driver interface for the reference UART module.
- Create `firmware/host/module-drivers/uart_reference_driver.c`
  Purpose: translate normalized UART requests into module-specific capability
  commands.
- Create `firmware/modules/reference-uart/README.md`
  Purpose: describe the module personality, assumptions, and supported
  capability behavior.
- Create `firmware/modules/reference-uart/module_main.c`
  Purpose: host-facing reference module entrypoint or stub.
- Create `firmware/modules/reference-uart/descriptor.c`
  Purpose: return descriptor and manifest data for enumeration.
- Create `firmware/modules/reference-uart/uart_service.c`
  Purpose: implement or stub the `uart.stream` capability behavior.
- Create `docs/sketches/reference-module-behavior.md`
  Purpose: record the expected responses and session behavior of the reference
  module.

### What the Driver and Module Work Should Cover

- descriptor parsing
- capability manifest parsing
- capability version checks
- opening and closing one UART session
- translating capability-scoped commands
- exposing stream data or stubbed data for validation

### Deliverables

- One host driver for the UART reference module
- One module-side reference implementation or believable stub
- One document describing the expected reference module behavior

### Done When

- The host can enumerate the reference module and drive one UART capability path
  through the driver boundary

## 7. Prove the Vertical Slice End to End

### Files to Create or Update

- Create `firmware/host/bringup/smoke/uart_smoke_app.c`
  Purpose: provide a minimal consumer for the `uart.stream` capability.
- Create `firmware/host/bringup/smoke/uart_smoke_app.h`
  Purpose: define the entrypoint for the temporary validation app or harness.
- Create `docs/sketches/uart-bringup-checklist.md`
  Purpose: define the exact validation steps to run.
- Create `docs/sketches/uart-slice-test-log.md`
  Purpose: capture observed results, failures, and follow-up work.
- Update `CHANGELOG.md`
  Purpose: record the first real vertical-slice milestone once it exists.
- Update `docs/ROADMAP.md`
  Purpose: move completed work out of the short-term queue and note new next
  steps.

### What the Validation Path Should Prove

- host boot reaches the bootstrap path
- a module enumerates successfully
- the host sees `uart.stream`
- the host opens a UART capability session
- UART configuration reaches the module
- UART data can flow upward
- stop and close behavior works cleanly

### Deliverables

- A minimal UART validation app or harness
- A bring-up checklist
- A test log with findings, gaps, and next actions

### Done When

- There is one repeatable, documented path from power-up to observed UART data

## Suggested Milestone Package

If you want the cleanest project structure, the short-term phase should leave
you with at least these new project artifacts:

- `docs/capabilities/uart-stream.md`
- `docs/sketches/uart-stream-open-questions.md`
- `docs/sketches/control-plane-payloads-v1.md`
- `docs/sketches/host-bootstrap-sequence.md`
- `docs/sketches/enumeration-sequence.md`
- `docs/sketches/service-lifecycle-uart.md`
- `docs/sketches/reference-module-behavior.md`
- `docs/sketches/uart-bringup-checklist.md`
- `docs/sketches/uart-slice-test-log.md`
- `firmware/host/bootstrap.h`
- `firmware/host/bootstrap.c`
- `external/DeK-Protocol/include/dek_transport.h`
- `external/DeK-Protocol/src/dek_transport.c`
- `external/DeK-Protocol/include/dek_packet.h`
- `external/DeK-Protocol/src/dek_packet.c`
- `external/DeK-Protocol/include/dek_crc.h`
- `external/DeK-Protocol/src/dek_crc.c`
- `firmware/host/transport/README.md`
- `firmware/host/service-api/service_api.h`
- `firmware/host/service-api/service_api.c`
- `firmware/host/module-manager/module_registry.h`
- `firmware/host/module-manager/module_registry.c`
- `firmware/host/module-manager/service_instance.h`
- `firmware/host/module-manager/service_instance.c`
- `firmware/host/module-drivers/uart_reference_driver.h`
- `firmware/host/module-drivers/uart_reference_driver.c`
- `firmware/host/bringup/smoke/uart_smoke_app.h`
- `firmware/host/bringup/smoke/uart_smoke_app.c`
- `firmware/modules/reference-uart/README.md`
- `firmware/modules/reference-uart/module_main.c`
- `firmware/modules/reference-uart/descriptor.c`
- `firmware/modules/reference-uart/uart_service.c`

Not every one of these files must be fully implemented in the first pass, but
they give you a concrete target structure for the UART slice project.
