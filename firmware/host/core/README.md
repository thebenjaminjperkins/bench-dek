# Core

This directory is reserved for host-wide runtime plumbing that does not belong
to a single application or hardware-facing layer.

Examples include:

- system bootstrap helpers
- event distribution
- configuration loading and validation

Keep DeK-specific orchestration in `host/` and move only narrowly reusable code
into top-level `components/`.
