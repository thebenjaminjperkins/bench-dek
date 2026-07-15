# Contributing

## Working Principles

DeK is being built architecture-first. Before adding feature code, prefer to
place it in the correct layer and update the relevant design document if the
change alters a contract or ownership boundary.

## Source Layout Rules

- User-facing workflow belongs in `firmware/host/applications/`.
- Presentation code belongs in `firmware/host/ui/`.
- App-to-module contracts belong in `firmware/host/service-api/`.
- Routing, lifecycle, and arbitration belong in
  `firmware/host/module-manager/`.
- Module-specific protocol translation belongs in
  `firmware/host/module-drivers/`.
- Byte transport and framing belong in `firmware/host/transport/`.
- Module-side source belongs in `firmware/modules/`.
- Reusable ESP-IDF components belong in `components/`.

## Documentation Expectations

Update docs when changing:

- Capability contracts
- Layer responsibilities
- Module lifecycle behavior
- Provider selection policy
- Transport framing or compatibility rules

Architectural decisions that would be hard to reverse should be recorded in
`docs/decisions/`.
