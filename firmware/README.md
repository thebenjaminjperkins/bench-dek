# Firmware Layout

Firmware source is organized to mirror the intended DeK architecture rather than
the current temporary test program.

## Layout

- `main.c` is a temporary test entrypoint.
- `host/` contains host-side production architecture layers.
- `modules/` contains module-side source and protocol-facing module logic.

As real implementation replaces the test entrypoint, code should move into the
layered directories below instead of accumulating in `main.c`.
