# Firmware Layout

Firmware source is organized to mirror the intended DeK architecture rather than
the current temporary test program.

## Layout

- `main.c` is the thin ESP-IDF entrypoint that hands off to host startup.
- `host/` contains host-side production architecture layers.
- `modules/` contains module-side source and protocol-facing module logic.
- `../external/dek-protocol/` contains the shared packet, CRC, and transport
  sources consumed by the firmware build.

Host-specific runtime code should live under `host/` rather than accumulating in
`main.c`.
