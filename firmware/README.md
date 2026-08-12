# Firmware Layout

Firmware source follows the intended DeK architecture. The current executable
path is a development-only GPIO vertical slice using a simulated module.

## Layout

- `main.c` is the thin ESP-IDF entrypoint that hands off to host startup.
- `host/` contains host-side production architecture layers.
- `host/bringup/` contains the active development-only GPIO validation harness
  as well as older UART/SPI smoke-test experiments.
- `host/tests/fixtures/` contains simulated modules and other host-local test
  fixtures used to validate the host stack without the separate module repo.
- `../external/DeK-Protocol/` contains the shared packet, CRC, and transport
  sources consumed by the firmware build.

`main.c` should remain a thin ESP-IDF handoff. Production applications must not
depend on the simulated module fixture or the bringup runtime.
