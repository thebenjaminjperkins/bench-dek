# Vision

DeK aims to be a modular embedded tool platform that feels like a coherent
instrument rather than a loose collection of accessories.

## Product Goals

- Let users run multiple diagnostic and analysis applications from one host.
- Allow modules to extend the system without forcing applications to know
  module-specific details.
- Make module replacement and future hardware revision changes survivable at the
  software boundary.
- Treat hardware faults, hot-plug, and capability contention as normal system
  behavior.

## Design Goals

- Capability-first application design
- Strong layering between UI, policy, protocol, and transport
- Deterministic provider selection and failure behavior
- Incremental bring-up from one module to many modules

## Non-Goals

- Tightly coupling apps to one reference module implementation
- Allowing raw module commands to leak into user-facing app logic
- Treating the current test program as the long-term firmware structure
