# GPIO Vertical Slice Continuation Roadmap

## Purpose and Current Baseline

The active implementation proves a host-side `gpio.digital` path against an
in-process reference module fixture. It currently completes HELLO, descriptor
and manifest discovery, provider registration, capability open, GPIO set-mode,
write and read commands, and capability close.

This roadmap continues that work without treating the fixture as production
module firmware. The goal is a repeatable, physical SPI demonstration with one
host, one GPIO reference module, and the same shared DeK-Protocol contracts.

The existing `uart.stream` documents remain useful future-capability design
work. They are not the acceptance criteria for this GPIO milestone.

## Milestone 1: Stabilize the Simulated Slice

### Deliverables

- A host-side test target or documented bringup procedure that reports pass or
  failure through its process result, not only log text.
- Negative-path tests for malformed packets, unsupported commands, invalid pin
  numbers, invalid pin state, bad descriptors/manifests, duplicate discovery,
  and failed close.
- Tests for channel exhaustion, reuse after close, and more than one open
  service where the advertised resource policy permits it.
- A GPIO bringup checklist and test log under `docs/`.

### Exit Criteria

The simulated reference module can be used to reproduce both successful and
important failing host paths without physical hardware.

## Milestone 2: Complete DeK-Protocol Coverage

DeK-Protocol remains the sole owner of shared wire contracts. Host drivers and
module firmware must not introduce private copies of packet or payload layouts.

### Deliverables

- A reviewed `gpio.digital` v1 contract covering pin range/identity, supported
  modes, levels, resource policy, command statuses, and response payloads.
- Validation helpers for every control-plane and GPIO payload that is currently
  decoded by the host or reference module.
- Golden packet fixtures for HELLO, DESCRIPTOR, CAPABILITIES, OPEN/OPEN_ACK,
  CLOSE/CLOSE_ACK, and the GPIO command/response pairs.
- Packet and receiver tests for sequence behavior, bad CRCs, truncated frames,
  oversized payloads, unknown message types, and response-channel mismatches.
- A documented compatibility rule for protocol and capability versions, plus
  an ERROR-payload policy for rejected requests.
- A portable C test command for the protocol library, run independently of
  ESP-IDF.

### Exit Criteria

The protocol repository can demonstrate byte-for-byte compatibility for the
GPIO control and command paths on host and module builds.

## Milestone 3: Turn the Fixture into a Reference Module Specification

The fixture is valuable behaviorally, but it belongs to host tests. Define the
deployable module separately before copying or moving implementation code.

### Deliverables

- A `firmware/modules/reference-gpio/` module project (or a separately versioned
  module repository) with an explicit build target for the intended MCU.
- A module README defining supported pins, electrical limits, startup defaults,
  SPI/ATTN/RESET behavior, descriptor fields, manifest contents, and build/flash
  instructions.
- A protocol dispatcher that uses DeK-Protocol for packet handling and owns
  only module-local GPIO hardware access and session state.
- GPIO HAL boundaries so pin operations are testable without target hardware.
- Module-side tests using the same golden frames as DeK-Protocol and a mock GPIO
  HAL.
- A descriptor/manifest generation or validation step that keeps advertised
  capability data synchronized with the implemented command set.

### Exit Criteria

A fresh module build can enumerate with the host and faithfully implement the
documented `gpio.digital` v1 contract without using host-test fixture code.

## Milestone 4: Implement the Physical SPI Adapter

### Deliverables

- An ESP-IDF SPI host implementation behind the host transport exchange
  boundary, including per-module chip-select configuration.
- ATTENTION and RESET handling with bounded transaction, retry, and recovery
  behavior that follows `docs/module-interface.md`.
- A module-side SPI-slave transport integration that feeds received bytes to
  DeK-Protocol and queues module responses/events.
- Link diagnostics: counters for transactions, CRC failures, timeouts, resets,
  and retry exhaustion.
- A board/pin configuration document and a minimal wiring diagram.

### Exit Criteria

The GPIO vertical-slice app runs unchanged above the exchange boundary while
using real host and module hardware.

## Milestone 5: Demonstrate and Harden the Slice

### Deliverables

- An end-to-end checklist: boot, reset, HELLO, enumeration, open, set output,
  write/read high, write/read low, close, module disconnect, reconnect, and
  recovery.
- Captured serial logs and a test report for at least one real hardware run.
- Fault-injection results for reset during an open service, bad packet input,
  and a lost or late response.
- Updates to the root README, CHANGELOG, and architecture documents identifying
  GPIO as the first demonstrated capability and listing remaining limitations.

### Exit Criteria

One documented command sequence proves a GPIO level change on physical hardware
and reads it back through the application-facing service API.

## Recommended Order

1. Stabilize the simulated slice and add negative tests.
2. Finalize the GPIO contracts and protocol test vectors in DeK-Protocol.
3. Specify and build the deployable reference module.
4. Add physical SPI implementations on both sides.
5. Run the hardware checklist and record the results.
6. Use the established pattern to implement `uart.stream`.

## Non-Goals for This Slice

- Multi-module arbitration beyond the current provider-selection model.
- GPIO interrupts, high-rate sampling, analog functions, or protocol analysis.
- A polished UI or general-purpose application framework.
- Declaring the host fixture to be production module firmware.
