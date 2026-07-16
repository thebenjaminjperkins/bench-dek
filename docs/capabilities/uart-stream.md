# uart.stream v1

`uart.stream` is the first concrete DeK capability contract.

It defines a host-visible UART service that lets an application:

- configure one UART session
- start and stop the session
- transmit bytes to the module-backed UART
- receive UART bytes as a normalized stream
- inspect service status without knowing module-specific details

This document makes the design choices for the first DeK vertical slice so the
host, driver, and reference module can be implemented against one stable target.

## Design Choices

### Capability Identity

- Capability ID: `uart.stream`
- Capability version: `1`

Why:

- The dotted ID matches the capability naming style already used elsewhere in
  the repo.
- A single integer version is easier to implement and compare during bring-up
  than a semantic-version tuple.

### Capability Scope

`uart.stream v1` models one full-duplex UART endpoint owned by one provider.

What it is for:

- terminal-style UART communication
- sending raw bytes to a UART peer
- receiving raw bytes from a UART peer
- basic line-status and fault reporting

What it is not for:

- logic-analyzer style UART sniffing
- bus decoding beyond raw UART framing
- dynamic pin routing from the application layer
- multi-port aggregation inside one service instance

Why:

- The first slice should prove the host architecture, not solve every serial
  use case.
- A narrow scope reduces driver and module complexity.

### Resource Policy

- Resource policy: `exclusive`
- One live service instance maps to one UART endpoint on one provider.
- Only one lease owner may control the service instance at a time in v1.
- Fan-out to multiple host subscribers is out of scope for v1.

Why:

- UART configuration affects the whole hardware endpoint.
- Allowing multiple independent owners would immediately create conflicts around
  baud rate, parity, and transmit ordering.

### Configuration Model

Configuration is supplied during `OPEN_CAPABILITY`.

The service is opened in a stopped state. Streaming does not begin until the
host sends `START`.

Configuration is immutable for the lifetime of the opened session. To change
baud rate or framing, the host closes the session and reopens it with a new
configuration.

Why:

- This keeps session lifecycle simple.
- It avoids complex mid-stream reconfiguration behavior in the first slice.

## Open Configuration Payload

The `OPEN_CAPABILITY` configuration blob for `uart.stream v1` uses this binary
layout.

All multibyte integer fields are little-endian.

| Offset | Size | Field | Notes |
| --- | --- | --- | --- |
| 0 | 4 | `baud_rate` | Unsigned integer, for example `115200` |
| 4 | 1 | `data_bits` | Allowed values: `5`, `6`, `7`, `8` |
| 5 | 1 | `parity` | `0=None`, `1=Even`, `2=Odd` |
| 6 | 1 | `stop_bits` | `1=1 stop bit`, `2=2 stop bits` |
| 7 | 1 | `flow_control` | `0=None`, `1=RTS_CTS` |
| 8 | 2 | `rx_packet_size_hint` | Preferred max bytes per stream packet |
| 10 | 2 | `reserved` | Must be zero in v1 |

Total size: 12 bytes

Design choices:

- No pin-selection field
- No clock-source field
- No "reconfigure while running" field

Why:

- The capability should remain module-agnostic.
- Physical routing belongs to the provider and its manifest, not to the app.
- A fixed-size blob is easy to parse on both host and module.

## Supported Commands

Capability-local commands are sent with `COMMAND` on the opened capability
channel.

### Command IDs

| ID | Name | Payload | Response |
| --- | --- | --- | --- |
| `0x01` | `START` | none | `RESPONSE` with service state |
| `0x02` | `STOP` | none | `RESPONSE` with service state |
| `0x03` | `WRITE` | byte payload | `RESPONSE` with accepted byte count |
| `0x04` | `GET_STATUS` | none | `RESPONSE` with status payload |
| `0x05` | `FLUSH_RX` | none | `RESPONSE` with cleared byte count |

Design choices:

- `START` and `STOP` are explicit commands.
- `WRITE` is the only transmit path.
- There is no separate `CONFIGURE` command in v1.
- There is no DTR/RTS control command in v1.

Why:

- Explicit start/stop matches the roadmap and makes session state easier to
  reason about.
- Putting configuration only in `OPEN_CAPABILITY` keeps lifecycle predictable.
- DTR/RTS signaling can be added later if a real use case requires it.

## Command Payloads

### `START`

Payload: none

Success response payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `service_state` |
| 1 | 1 | `reserved0` |
| 2 | 2 | `reserved1` |

`service_state` values:

- `0=Stopped`
- `1=Starting`
- `2=Running`
- `3=Stopping`
- `4=Faulted`

### `STOP`

Payload: none

Success response payload uses the same layout as `START`.

### `WRITE`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `tx_length` |
| 2 | N | `tx_bytes` |

Rules:

- `tx_length` must match the byte count in `tx_bytes`.
- `WRITE` is valid only when the service state is `Running`.
- `tx_length` must not exceed the provider's advertised `tx_chunk_max`.

Success response payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `accepted_length` |
| 2 | 2 | `reserved` |

Design choice:

- Partial acceptance is allowed.

Why:

- It gives the module room to respect limited TX buffering without treating
  every short write as a fatal error.

### `GET_STATUS`

Payload: none

Success response payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `service_state` |
| 1 | 1 | `line_status_flags` |
| 2 | 2 | `rx_overflow_count` |
| 4 | 2 | `framing_error_count` |
| 6 | 2 | `parity_error_count` |
| 8 | 2 | `break_count` |
| 10 | 2 | `reserved` |

`line_status_flags` bits:

- bit `0`: carrier present if supported, else zero
- bit `1`: break detected since last clear
- bit `2`: RX overflow observed
- bit `3`: framing error observed
- bit `4`: parity error observed

### `FLUSH_RX`

Payload: none

Success response payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `cleared_bytes` |
| 2 | 2 | `reserved` |

Why include it:

- It is useful during terminal bring-up.
- It is much simpler than adding replay, bookmarking, or complex ring-buffer
  management to v1.

## Stream Data

UART receive data is delivered through `STREAM_DATA` packets on the opened
capability channel.

### `STREAM_DATA` Payload

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | `rx_sequence` |
| 4 | 1 | `status_flags` |
| 5 | 1 | `reserved0` |
| 6 | 2 | `data_length` |
| 8 | N | `data_bytes` |

`status_flags` bits:

- bit `0`: RX overflow occurred before or during this packet
- bit `1`: framing error affected at least one received byte
- bit `2`: parity error affected at least one received byte
- bit `3`: break observed

Rules:

- `data_length` must match the size of `data_bytes`.
- `rx_sequence` increments by one for each emitted stream packet.
- The host may treat gaps in `rx_sequence` as evidence of dropped packets.

Design choices:

- RX data is streamed; TX data is not echoed as a stream by default.
- Timestamps are omitted in v1.

Why:

- The first UART slice is primarily about proving end-to-end service flow.
- Host-side receipt time is good enough for terminal use during bring-up.

## Events

`EVENT` packets on the opened capability channel are reserved for state changes
that are not ordinary RX data.

### Event Types

| ID | Name | Purpose |
| --- | --- | --- |
| `0x01` | `STATE_CHANGED` | Service entered a new lifecycle state |
| `0x02` | `FAULT` | Provider detected a UART-side fault |

### `STATE_CHANGED` Payload

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `old_state` |
| 1 | 1 | `new_state` |
| 2 | 2 | `reserved` |

### `FAULT` Payload

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `fault_code` |
| 1 | 1 | `line_status_flags` |
| 2 | 2 | `detail` |

Design choice:

- Keep events small and separate from RX data.

Why:

- It prevents control state from being hidden in byte-stream payloads.

## Capability Errors

The module reports capability errors through the shared `ERROR` envelope or
capability-local `RESPONSE` failures as appropriate.

Recommended `uart.stream v1` error codes:

| Code | Meaning |
| --- | --- |
| `0x01` | unsupported baud rate |
| `0x02` | unsupported framing |
| `0x03` | unsupported flow control |
| `0x04` | invalid state for command |
| `0x05` | TX buffer busy |
| `0x06` | RX buffer fault |
| `0x07` | provider fault |

Design choice:

- Keep the UART error list small and implementation-oriented.

Why:

- It is enough to build host handling now without over-designing rare cases.

## Provider Manifest Expectations

When a module advertises `uart.stream v1`, its capability manifest should also
declare:

- maximum supported baud rate
- maximum `WRITE` chunk size
- maximum recommended RX packet payload size
- supported data-bit values
- supported parity modes
- supported stop-bit values
- supported flow-control modes

Why:

- The host needs these limits for validation before opening a session.

## Lifecycle

The expected service lifecycle is:

1. Host enumerates the module.
2. Host sees `uart.stream v1` in the manifest.
3. Host sends `OPEN_CAPABILITY` with the UART config blob.
4. Module replies with `OPEN_ACK` and a channel ID.
5. Host sends `START` on the opened channel.
6. Module emits `STREAM_DATA` as RX bytes arrive.
7. Host sends `WRITE` commands as needed.
8. Host sends `STOP`.
9. Host sends `CLOSE_CAPABILITY`.

## Ready-To-Code Summary

You can program against these rules now:

- `uart.stream` is exclusive in v1.
- Configuration is fixed at open time.
- RX data comes through `STREAM_DATA`.
- TX data goes through `WRITE`.
- Session lifecycle is `open -> start -> run -> stop -> close`.
- Physical pin routing is not app-configurable.
- Capability-local command IDs are valid only inside `uart.stream v1`.
