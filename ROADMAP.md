# Roadmap

## Near Term

1. Finalize the host software contract and module interface contract.
2. Define the first capability set, including naming, versioning, and resource
   policies.
3. Replace the test-oriented firmware entrypoint with a layered host bootstrap.
4. Introduce the first module driver and transport implementation skeleton.

## Mid Term

1. Implement module discovery and enumeration.
2. Add capability routing and lease management in the module manager.
3. Build a minimal UI shell and a first real application, likely UART or GPIO.
4. Stand up a reference module and validate the end-to-end request flow.

## Longer Term

1. Support multiple modules exposing overlapping capabilities.
2. Add robust degraded-mode handling for hot-plug and transport failure.
3. Expand the application set to protocol analysis and mixed-signal tools.
4. Refine reusable components for host services, transport, and shared data
   types.
