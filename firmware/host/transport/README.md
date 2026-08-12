# Transport Layer

This directory contains host-side adapters above the shared DeK-Protocol
library, and will contain link implementations such as SPI, UART, USB, or
other physical transports.

`host_transport_adapter` currently converts host requests into shared DeK
control-plane and command packets, but delegates byte exchange to an injected
callback. The GPIO slice provides that callback with an in-process module
fixture. It is not yet a physical SPI implementation.

Physical transport code owns:

- framing at the byte-stream boundary
- link I/O
- retries and timeouts
- disconnect detection
- raw link diagnostics

It should not own capability routing or application policy.
