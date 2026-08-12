# Module Drivers Layer

This directory contains module-family or protocol-version adapters that
translate a module's concrete command set into normalized host-side operations.

The current `gpio_remote_provider` discovers a `gpio.digital` capability from
the simulated reference module, registers it with the module manager, and maps
typed GPIO operations to shared capability commands. It is the reference shape
for later hardware-backed drivers, not a substitute for an SPI adapter or
module firmware.

Drivers belong here when they:

- understand module-specific command layouts or quirks
- adapt one module family into shared service/provider behavior
- shield upper layers from module-specific wire details
