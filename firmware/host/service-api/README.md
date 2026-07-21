# Service API Layer

This directory is for the host-facing contracts that applications use to request
capabilities, subscribe to streams, and reserve exclusive resources.

The service API is the seam that keeps applications module-agnostic.

It should translate capability requests into runtime service handles without
leaking provider selection or transport details into applications.
