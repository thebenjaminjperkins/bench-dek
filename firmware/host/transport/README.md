# Transport Layer

This directory is for link-level communication with modules, including framing,
integrity checks, timeouts, retries, and disconnect detection.

Transport code moves bytes and manages link health. It does not decide which
capability an application should receive.
