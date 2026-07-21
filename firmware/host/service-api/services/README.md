# Service Wrappers

This directory is for host-local typed wrappers such as GPIO or UART service
helpers.

These wrappers sit on top of generic runtime service handles returned by the
service API. They should not redefine wire contracts that already belong in the
shared `external/DeK-Protocol` library.
