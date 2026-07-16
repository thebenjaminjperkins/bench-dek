# 0004: Standardize Control Plane and Capability-Scoped Commands

## Status

Accepted

## Decision

DeK standardizes:

- The host-to-module control plane
- Capability contracts, including capability-specific command and payload schemas

DeK does not define one global opcode table in which every numeric command value
must have the same meaning across every capability and every module.

Instead, command meaning is scoped by:

- Capability ID
- Capability version
- The message context of the opened capability session

## Rationale

The architecture is capability-first rather than module-first. Applications ask
for stable capabilities such as `uart.stream` or `spi.capture`, not for raw
module commands.

A single global opcode registry would create unnecessary coupling between
unrelated capabilities. It would also make evolution harder because every new
capability or revision would have to compete for shared numeric meanings that do
not matter outside that capability.

By contrast, standardizing capability contracts preserves interoperability where
it matters:

- The host can reason about what a capability does
- Applications can remain module-agnostic
- Module drivers can translate between a normalized capability contract and a
  module's private implementation when needed

## Consequences

- Control-plane messages such as enumeration, descriptor exchange, session
  setup, streaming envelopes, and error reporting must remain standardized.
- Capability-specific commands should be defined within the versioned contract
  for that capability rather than in one product-wide opcode table.
- Reusing an existing capability contract is the preferred way for a new module
  to maximize host compatibility.
- A module that already speaks an existing capability contract may be able to
  reuse an existing host driver.
- A module that exposes the same high-level capability but uses different
  command semantics or payload shapes will require a host driver that performs
  translation.
