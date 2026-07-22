# Module Drivers Layer

This directory is reserved for module-family or protocol-version adapters that
translate a module's concrete command set into normalized host-side operations.

Drivers belong here when they:

- understand module-specific command layouts or quirks
- adapt one module family into shared service/provider behavior
- shield upper layers from module-specific wire details
