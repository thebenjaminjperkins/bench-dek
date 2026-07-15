# 0002: Capability-First Module Contract

## Status

Accepted

## Decision

Applications will depend on capabilities exposed by the host rather than on
specific modules, slots, or raw commands.

## Rationale

This allows:

- Multiple modules to provide the same service
- Module hardware revisions to evolve behind a stable app contract
- Provider selection and arbitration to remain a host concern

## Consequences

- Capability descriptors and versioning become first-class architecture
  artifacts.
- The module manager must resolve providers deterministically.
- Drivers must normalize module-specific behavior into capability contracts.
