# Host Tests

This directory contains host-side tests and harness code. It currently covers
both module/service registry behavior and the simulated `gpio.digital`
transport slice.

`fixtures/reference-gpio/` is an in-process module simulator. It validates the
host's use of shared packet and control-plane contracts without implying that
the fixture is deployable module firmware. Tests may be invoked from bringup
flows during early development, but they are not part of the production
application layer.
