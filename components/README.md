# Components

This directory is reserved for reusable ESP-IDF components shared by the host
firmware, module drivers, or future support libraries.

Prefer `components/` for code that is cross-cutting and reusable across layers.
Prefer `firmware/host/` or `firmware/modules/` for code that belongs to a
specific architectural area.
