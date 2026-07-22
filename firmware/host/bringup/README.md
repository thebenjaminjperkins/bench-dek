# Bringup Layer

This directory contains development-only host runtime code used while the
production application and service layers are still taking shape.

Examples include:

- smoke tests that validate a transport or protocol path end to end
- temporary boot flows that run embedded test harnesses
- diagnostics that intentionally bypass the final service API layering

Code here should be easy to delete, replace, or refactor as production
architecture becomes available.
