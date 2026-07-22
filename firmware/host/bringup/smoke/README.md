# Smoke Tools

These files are transport-facing bringup tools used to prove out links,
framing, and module responses before the host's long-term service-driven app
layer is complete.

They are intentionally kept out of `host/applications/` because they know too
much about transport and protocol details to represent the final application
boundary.
