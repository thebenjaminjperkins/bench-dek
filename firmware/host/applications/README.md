# Applications Layer

Applications define user-facing tools such as UART terminals, logic analyzers,
GPIO monitors, and protocol viewers.

Code in this directory should depend on stable capabilities provided by the
service API rather than on specific modules.

Development-only smoke tests and early transport bringup tools live under
`../bringup/` until they are refactored behind stable service APIs.
