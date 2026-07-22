# Transport Layer

This directory is reserved for host-side link code such as UART, SPI, USB, or
other physical transport implementations.

Transport code owns:

- framing at the byte-stream boundary
- link I/O
- retries and timeouts
- disconnect detection
- raw link diagnostics

It should not own capability routing or application policy.
