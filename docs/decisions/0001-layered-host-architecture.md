# 0001: Layered Host Architecture

## Status

Accepted

## Decision

The host is organized into explicit layers:

1. UI
2. Applications
3. Service API
4. Module Manager
5. Module Drivers
6. Transport

## Rationale

These layers separate user workflow, policy, protocol translation, and link
management so that module-specific details do not leak into applications.

## Consequences

- The repo should mirror these layers in `firmware/host/`.
- Cross-layer shortcuts are considered architectural violations.
- New host code should be placed according to ownership, not convenience.
