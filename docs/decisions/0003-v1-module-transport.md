# 0003: DeK v1 Module Transport

## Status

Accepted

## Decision

DeK v1 will use a standardized host-to-module transport rather than allowing each
module family to define its own wire protocol.

For v1, the selected transport is SPI with a shared DeK packet protocol layered on
top. Modules may differ internally, but every module must present the same
host-facing transport behavior for:

- Enumeration
- Descriptor exchange
- Capability discovery
- Command/response packets
- Stream or event delivery
- Error reporting

SPI is a transport choice, not the architectural center of the system. The DeK
packet protocol and capability model remain the primary contracts.

## Rationale

SPI is chosen for v1 because it fits the intended product shape:

- One host owns timing and scheduling.
- Modules are expected to be physically close to the host.
- The system benefits from one link carrying both control traffic and sampled data.
- ESP32-S3 and RP2350 both support SPI well.
- Throughput is substantially better than UART or I2C for instrumentation-style
  workloads.

This choice is not merely based on currently available hardware. It follows from
the architecture in which the host is the source of truth, the communication
layer is centralized, and applications remain agnostic to module details.

## Why Not Per-Module Protocols

Allowing each module to invent its own host-facing protocol would:

- Make enumeration inconsistent
- Complicate hot-plug and fault recovery
- Push too much complexity into module drivers
- Make diagnostics and tooling harder to standardize
- Undermine the goal of a stable capability-first architecture

Modules may implement capabilities differently, but they should not present a
different transport personality to the host.

## Why Not UART or I2C

UART is easy to prototype, but it is a weaker long-term fit for a module bus
because it provides lower throughput and weaker framing expectations for mixed
command and streaming traffic.

I2C reduces pin count, but it is a poor fit for higher-rate instrumentation data
and is less attractive for a modular system that may eventually stream samples or
frequent events.

## Why Not CAN or RS-485 First

CAN and RS-485 are stronger system-bus choices for longer cables, noisier
environments, or larger external module ecosystems. They are not the v1 choice
because DeK is currently envisioned as a short-range modular instrument with a
host-controlled backplane-style relationship to modules.

SPI is the better first choice when:

- The connection is short
- The host is physically central
- Module count is limited
- Deterministic host-driven scheduling matters more than distributed networking

## Conditions That Would Trigger Reconsideration

The transport decision should be revisited if DeK evolves toward any of these:

- Modules connected by longer cables rather than short board-to-board links
- Frequent user hot-plug in electrically noisy environments
- A larger multi-drop ecosystem where per-module chip-select lines become awkward
- Stronger ruggedness requirements
- External modules that are physically distributed rather than enclosed together

If those conditions become central to the product, CAN or RS-485 should be
evaluated as a future transport while preserving the same DeK packet and
capability contracts above the transport layer.

## Consequences

- `transport/` should be designed around a transport interface, not around SPI
  assumptions leaking upward.
- The DeK packet protocol should be specified independently of electrical
  transport where practical.
- Module drivers should depend on normalized transport behavior rather than
  custom per-module wire conventions.
- Future transports remain possible, but SPI is the only required v1 transport.
