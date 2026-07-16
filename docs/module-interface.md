# Module Interface

This document defines the v1 host-to-module interface for DeK. It covers the
standard transport, wire-level packet rules, enumeration flow, and the boundary
between shared control messages and capability-specific payloads.

The intent is to standardize how the host talks to modules without forcing all
modules to share the same internal implementation.

## Scope

This document defines:

- The required v1 transport
- Required signals between host and module
- Packet framing and sequencing
- Module enumeration and descriptor exchange
- Capability discovery and session management
- Error handling and timeout expectations

This document does not define:

- The final connector or physical pinout
- Individual capability payload schemas beyond the boundary rules that scope them
- UI or application behavior
- Module-internal firmware structure

## Transport

DeK v1 uses SPI as the required host-to-module transport.

Rules:

- The host is always the SPI master.
- A module is always an SPI slave.
- The host controls transaction timing and clocks all data.
- Modules must not assume they can transmit spontaneously without host clocks.

Initial SPI operating assumptions for v1:

- 4-wire SPI
- 8-bit words
- MSB first
- SPI mode 0
- Enumeration starts at a conservative clock rate, such as 1 MHz
- Higher clock rates may be used after enumeration if both sides support them

## Required Signals

Each module connection must provide:

- `SCLK`
- `MOSI`
- `MISO`
- `CS`
- `ATTN` from module to host
- `RESET` from host to module
- `GND`
- power

Optional but recommended:

- `PRESENT` from module or connector detect logic to host

## Signal Semantics

### `CS`

`CS` selects the target module for an SPI transaction. A transaction is scoped to
exactly one module.

### `ATTN`

`ATTN` means the module has something pending for the host.

Examples:

- A deferred command response is ready
- Stream data is available
- An asynchronous event occurred
- A fault or status change needs to be reported

Rules:

- The module asserts `ATTN` when one or more outbound packets are queued.
- The host responds by initiating one or more SPI transactions to drain them.
- The module deasserts `ATTN` only when no host-visible packets remain pending.
- `ATTN` is a level signal, not a pulse contract.

### `RESET`

`RESET` lets the host force a module into a known boot state.

Rules:

- The host may assert `RESET` during recovery or startup.
- A reset module must discard open sessions and pending responses.
- After reset, the module returns to the `Detected` or boot state and must be
  enumerated again.

### `PRESENT`

`PRESENT` is optional and indicates physical presence. If unavailable, the host
may infer module presence through connector design or SPI enumeration behavior.

## Link Model

DeK uses a packet-oriented link protocol on top of SPI.

Rules:

- Each packet is self-delimiting and integrity-protected.
- Control traffic and capability traffic use the same packet envelope.
- Channel `0` is reserved for control-plane messages.
- Non-zero channels represent opened capability sessions.

The SPI bus is the byte pipe. The DeK packet protocol defines the meaning of the
bytes.

## Packet Format

Each packet has this structure:

1. Fixed header
2. Variable payload
3. CRC trailer

### Header

The v1 header is 12 bytes:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | Magic `0x44 0x4B` (`DK`) |
| 2 | 1 | Protocol version |
| 3 | 1 | Message type |
| 4 | 1 | Flags |
| 5 | 1 | Header length, currently `12` |
| 6 | 2 | Sequence number |
| 8 | 2 | Channel ID |
| 10 | 2 | Payload length |

### Payload

The payload format depends on message type and channel.

- On channel `0`, the payload uses DeK control message schemas.
- On non-zero channels, the payload uses the schema defined by the opened
  capability session.

Unless a payload definition says otherwise, all multibyte integer fields in DeK
v1 payloads are little-endian.

### Trailer

The trailer is:

| Size | Field |
| --- | --- |
| 2 | CRC16-CCITT over header and payload |

## Message Types

The shared v1 message types are:

- `HELLO`
- `HELLO_ACK`
- `GET_DESCRIPTOR`
- `DESCRIPTOR`
- `GET_CAPABILITIES`
- `CAPABILITIES`
- `OPEN_CAPABILITY`
- `OPEN_ACK`
- `CLOSE_CAPABILITY`
- `CLOSE_ACK`
- `COMMAND`
- `RESPONSE`
- `STREAM_DATA`
- `EVENT`
- `PING`
- `PONG`
- `ERROR`
- `POLL`
- `EMPTY`

Modules may not invent alternate control-plane semantics. Capability-specific behavior belongs inside capability payloads, not in transport rules.

## Command Scoping

DeK v1 intentionally separates global transport semantics from capability-local command semantics.

Rules:

- Control-plane message types are global and standardized for every module.
- Capability-specific commands are defined by the opened capability contract,
  not by one system-wide opcode table.
- The meaning of a capability command depends on capability ID, capability
  version, and session context.
- Numeric command values may be reused by different capabilities without
  conflict when their contracts are different.

Example:

- A command value used inside `spi.capture v1` may mean "start capture".
- The same numeric value may mean something else inside `uart.stream v1`.
- Neither reuse changes the meaning of the shared control-plane messages such as
  `OPEN_CAPABILITY`, `COMMAND`, `STREAM_DATA`, or `ERROR`.

This keeps the host/module protocol stable while allowing capability contracts to
evolve independently.

## Flags

The `Flags` byte is reserved for transport-wide behavior.

Initial v1 flags:

- `0x01`: response required
- `0x02`: more packets pending after this one
- `0x04`: error packet
- `0x08`: stream packet

Unassigned flags must be sent as zero and ignored when unknown unless a future
protocol version defines otherwise.

## Sequence Numbers

Sequence numbers are host-assigned for host-originated requests.

Rules:

- The host increments the sequence number for each new request packet.
- A module response must echo the matching sequence number.
- Asynchronous module-originated packets such as events or stream data use the
  most relevant associated session and may use module-managed sequencing within
  the payload if the capability requires it.
- Sequence numbers are 16-bit and may wrap.

## Channel Model

Channel `0` is the control channel and is always available.

Non-zero channels represent capability sessions created by `OPEN_CAPABILITY`.

Rules:

- The host opens a capability before sending capability-specific `COMMAND`
  packets.
- The module returns a channel ID in `OPEN_ACK`.
- The channel remains valid until `CLOSE_CAPABILITY`, module reset, transport
  fault, or module removal.
- Channel IDs are scoped to a single module instance and are not stable across
  resets.

This prevents capability traffic from being confused with enumeration and base
management traffic.

## Module Descriptor

Each module must return a descriptor on the control channel when the host sends
`GET_DESCRIPTOR`.

The descriptor must include:

- Stable module identifier
- Module family or product identifier
- Hardware revision
- Firmware revision
- Supported protocol version
- Maximum supported packet payload
- Capability manifest length or pagination info
- Health or fault flags

If the descriptor is malformed, inconsistent, or incompatible, the host must
quarantine the module rather than partially registering it.

## Capability Manifest

Each advertised capability must include:

- Capability ID
- Capability version
- Supported commands
- Event or stream types
- Required configuration parameters
- Resource policy: shared, exclusive, or reservable
- Performance limits

The host binds applications to these capabilities rather than to module-specific
commands.

If the manifest is too large for a single packet, `GET_CAPABILITIES` and
`CAPABILITIES` support pagination through payload parameters.

Capability manifests describe the command surface for a specific capability
contract. They do not imply that command IDs are globally reserved across all
other capabilities in the system.

## Minimal Control Payload Definitions for the First Vertical Slice

The v1 transport document remains capability-agnostic, but the first DeK
vertical slice needs concrete control-plane payloads so host and module code can
be implemented consistently.

These payloads are the baseline definitions for enumeration and session setup in
v1.

### Common Encoding Rules

- Multibyte integers are little-endian.
- Fixed-width numeric fields are preferred over text when a value is strictly
  machine-readable.
- Capability IDs are encoded as ASCII with a leading byte length.
- Reserved fields must be sent as zero and ignored on receipt in v1.

### `HELLO`

`HELLO` is sent by the host on channel `0`.

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `min_protocol_version` |
| 1 | 1 | `max_protocol_version` |
| 2 | 2 | `host_flags` |

Rules:

- `host_flags` must be zero in v1.
- The host should normally send `1` and `1` for min and max protocol version
  during the first slice.

### `HELLO_ACK`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `selected_protocol_version` |
| 1 | 1 | `module_flags` |
| 2 | 2 | `reserved` |

Rules:

- `module_flags` must be zero in v1 unless a future document defines bits.
- If no compatible protocol version exists, the module should reply with
  `ERROR` instead of `HELLO_ACK`.

### `GET_DESCRIPTOR`

Payload: none

### `DESCRIPTOR`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 16 | `module_id` |
| 16 | 2 | `module_family_id` |
| 18 | 2 | `module_model_id` |
| 20 | 1 | `hardware_rev_major` |
| 21 | 1 | `hardware_rev_minor` |
| 22 | 1 | `firmware_rev_major` |
| 23 | 1 | `firmware_rev_minor` |
| 24 | 1 | `firmware_rev_patch` |
| 25 | 1 | `supported_protocol_version` |
| 26 | 2 | `max_payload_size` |
| 28 | 2 | `capability_manifest_total_bytes` |
| 30 | 1 | `capability_count` |
| 31 | 1 | `health_flags` |

Rules:

- `module_id` is a stable 16-byte identifier, not a human-readable string.
- `max_payload_size` is the maximum payload length the module accepts inside one
  packet, not the total packet size.
- `health_flags` is a bitfield and may be zero when healthy.

### `GET_CAPABILITIES`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `manifest_offset` |
| 2 | 2 | `requested_length` |

Rules:

- `manifest_offset` is a byte offset into the serialized manifest.
- `requested_length` is the maximum chunk length the host wants returned.

### `CAPABILITIES`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `manifest_total_bytes` |
| 2 | 2 | `chunk_offset` |
| 4 | 2 | `chunk_length` |
| 6 | N | `chunk_bytes` |

Rules:

- The host reconstructs the manifest by offset.
- `chunk_length` must match the size of `chunk_bytes`.

### Manifest Entry Encoding

Each capability manifest entry uses this structure inside `chunk_bytes`:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `capability_id_length` |
| 1 | N | `capability_id_ascii` |
| N+1 | 2 | `capability_version` |
| N+3 | 1 | `resource_policy` |
| N+4 | 1 | `command_count` |
| N+5 | 1 | `event_count` |
| N+6 | 2 | `capability_flags` |
| N+8 | 2 | `limits_blob_length` |
| N+10 | M | `limits_blob` |

`resource_policy` values:

- `0=shared`
- `1=exclusive`
- `2=reservable`

Design choice:

- Capability IDs stay human-readable in the manifest and open request.

Why:

- It makes bring-up, logs, and future tooling much easier.

### `OPEN_CAPABILITY`

`OPEN_CAPABILITY` is sent on channel `0`.

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `capability_id_length` |
| 1 | N | `capability_id_ascii` |
| N+1 | 2 | `capability_version` |
| N+3 | 2 | `open_flags` |
| N+5 | 2 | `config_length` |
| N+7 | M | `config_blob` |

Rules:

- `open_flags` must be zero in v1.
- `config_blob` uses the capability-specific config format for the requested
  capability.
- For the first slice, `config_blob` for `uart.stream v1` is defined in
  `docs/capabilities/uart-stream.md`.

### `OPEN_ACK`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `channel_id` |
| 2 | 2 | `service_flags` |
| 4 | 2 | `accepted_config_length` |
| 6 | N | `accepted_config_blob` |

Rules:

- `channel_id` must be non-zero on success.
- If the module cannot open the capability, it should send `ERROR` instead of
  `OPEN_ACK`.
- `accepted_config_blob` may echo the requested config or a normalized accepted
  config if the contract allows negotiation.

Design choice for the first slice:

- `uart.stream v1` should echo the accepted config unchanged when the request is
  valid.

Why:

- It avoids hidden negotiation during bring-up.

### `CLOSE_CAPABILITY`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `target_channel_id` |
| 2 | 2 | `reserved` |

### `CLOSE_ACK`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | `target_channel_id` |
| 2 | 2 | `reserved` |

### `ERROR`

Payload:

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | `error_class` |
| 1 | 1 | `error_code` |
| 2 | 2 | `related_sequence` |
| 4 | 2 | `related_channel` |
| 6 | 1 | `detail_length` |
| 7 | N | `detail_bytes` |

Rules:

- `detail_bytes` may be empty.
- `related_sequence` should be zero if no specific request is implicated.
- `related_channel` should be zero for control-plane failures that are not tied
  to an opened session.

## Enumeration Flow

The v1 enumeration sequence is:

1. Host detects a module through `PRESENT`, fixed slot knowledge, or transport probing.
2. Host asserts then releases `RESET` if needed to establish a clean state.
3. Host selects a safe initial SPI clock.
4. Host sends `HELLO` on control channel `0`.
5. Module replies with `HELLO_ACK`, including supported protocol version.
6. Host sends `GET_DESCRIPTOR`.
7. Module returns `DESCRIPTOR`.
8. Host sends one or more `GET_CAPABILITIES` requests.
9. Module returns one or more `CAPABILITIES` packets.
10. Host validates compatibility and marks the module `Ready`.

Enumeration fails if:

- Protocol version is unsupported
- Descriptor integrity fails
- Required fields are missing
- Capability data is malformed

## SPI Transaction Behavior

SPI is full-duplex electrically, but DeK treats it as a packet transport with
host-driven polling.

Normal patterns:

- Host sends a request and receives an immediate response in the same
  transaction if the module is ready.
- Host sends a request and receives an `EMPTY`, `ERROR`, or a short immediate
  acknowledgment if the full response is deferred.
- Module later asserts `ATTN`.
- Host issues `POLL` or another planned transfer to fetch the queued packet.

This model avoids requiring a module to complete every request synchronously
while preserving host ownership of clocks and scheduling.

## `POLL` and `EMPTY`

`POLL` is a host packet used to fetch pending module traffic when the host has no
new command to send.

`EMPTY` means:

- the module has no packet ready for the host, or
- the requested packet is not ready yet

`EMPTY` is not an error by itself.

## Streaming and Events

Stream data and asynchronous events use the same packet envelope as control
traffic.

Rules:

- Stream data is tied to an open capability channel.
- The module asserts `ATTN` when stream packets are pending.
- The host drains pending packets at a rate appropriate for the capability.
- Capability definitions may add payload-level sample counters, timestamps, or
  drop indicators.

This keeps streaming within the same transport model rather than introducing a
separate side channel.

## Error Handling

Error reporting uses the shared `ERROR` message type on either the control
channel or a capability channel.

An error payload must include:

- Error class
- Error code
- Related sequence number, if any
- Related channel, if any

Transport-visible error classes should include:

- malformed packet
- unsupported protocol version
- unsupported message type
- invalid channel
- busy
- timeout
- internal module fault

Applications should see normalized capability failures from the host rather than
raw transport failures whenever possible.

## Timeouts and Retries

The host owns retry policy.

Baseline v1 expectations:

- A valid packet must always return either a valid response packet or a valid
  `ERROR` or `EMPTY` packet.
- The host should use a short link timeout for an SPI transaction.
- The host should use a longer logical timeout for deferred command completion.
- Only the host retries requests.
- A module must treat duplicate requests with the same live sequence number
  carefully enough to avoid corrupting state.

Capability contracts may define longer operation times, but the transport rules
remain host-driven.

## Compatibility Rules

- Protocol version mismatches are handled during `HELLO`.
- Capability version mismatches are handled when registering providers.
- Backward-compatible additions are preferred over breaking control message
  changes.
- Modules may evolve internally without affecting apps if the capability
  contract remains stable.

## Ownership Rules

- The host owns capability routing, lifecycle, and arbitration.
- The module owns the physical hardware it exposes.
- Drivers own translation between host requests and module commands.
- Applications do not talk to modules directly.
- The transport layer owns framing, CRC validation, sequence matching, and
  timeout behavior.

## Design Notes

This v1 spec intentionally standardizes the host-facing link even though modules
may use different microcontrollers or internal designs.

That means:

- one module may be ESP32-S3-based
- another may be RP2350-based
- both still enumerate and communicate through the same DeK SPI packet contract

This is the main mechanism that keeps the architecture module-agnostic.
