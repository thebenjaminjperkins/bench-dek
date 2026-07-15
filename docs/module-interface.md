# Module Interface

This document defines the architectural shape of the contract between the host
and modules. It does not yet lock in a final wire format, but it establishes the
required concepts that any protocol must support.

## Module Descriptor

Each module must provide a descriptor during enumeration that includes:

- Stable module identifier
- Module family or product identifier
- Hardware revision
- Firmware revision
- Supported protocol version
- Capability manifest
- Health or fault flags

If the descriptor is malformed or incompatible, the host must quarantine the
module rather than exposing partial capability state to applications.

## Capability Manifest

Each advertised capability must include:

- Capability ID
- Capability version
- Supported commands
- Event or stream types
- Required configuration parameters
- Resource policy: shared, exclusive, or reservable
- Performance limits

The host binds applications to these capabilities rather than to module-specific
commands.

## Required Protocol Behaviors

Whatever wire protocol we adopt must support:

- Module enumeration
- Command/response exchanges
- Streaming data delivery
- Request correlation
- Transport integrity checks
- Timeouts and retry boundaries
- Fault and disconnect reporting

## Ownership Rules

- The host owns capability routing, lifecycle, and arbitration.
- The module owns the physical hardware it exposes.
- Drivers own translation between host requests and module commands.
- Applications do not talk to modules directly.

## Compatibility Rules

- Protocol version mismatches are handled at enumeration time.
- Capability version mismatches are handled when registering providers.
- Backward-compatible additions are preferred over breaking capability changes.
- Modules may evolve internally without affecting apps if the capability
  contract remains stable.

## Future Work

This document should later be expanded with:

- Concrete message envelopes
- Error codes
- Capability naming conventions
- Reservation semantics
- Streaming flow-control behavior
