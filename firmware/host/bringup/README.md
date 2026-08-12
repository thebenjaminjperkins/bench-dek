# Bringup Layer

This directory contains development-only host runtime code used while the
production application and service layers are still taking shape.

Examples include:

- the active GPIO vertical-slice harness, which runs discovery, session open,
  set-mode/write/read commands, and close against a simulated reference module
- smoke tests that validate a transport or protocol path end to end
- temporary boot flows that run embedded test harnesses
- diagnostics that intentionally bypass the final service API layering

The active runtime currently selects `unit_tests_then_gpio` in
`dev_runtime.c`. This is intentional for bringup only. Code here should be easy
to replace with a physical transport and a production application as those
layers become available.
