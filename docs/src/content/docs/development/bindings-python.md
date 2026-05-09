---
title: Python Binding Conventions
description: Language-specific implementation conventions for Python bindings.
---

Tracking issue:
[#49](https://github.com/maplibre/maplibre-native-ffi/issues/49).

The Python binding targets CPython 3.11+ and exposes a safe low-level API.
Generated C and Rust declarations stay private. Use PyO3 with `abi3-py311` and
maturin where practical. Treat PyPy as an experimental source-build target until
CI covers it.

Build `_native` with PyO3 over the shared internal crates defined by the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/).
Python and Node share Rust ABI adaptation with the public Rust crate, but
neither wraps the public Rust API. The Python extension owns Python-facing
lifetime, GIL, exception, callback, and packaging behavior.

Do not ship `ctypes` or `cffi` as supported runtime binding paths.

Use Python classes, dataclasses, enums, exceptions, `bytes`, writable buffer
inputs, and opaque `NativePointer` values at the public surface. Keep PyO3
internals, raw C structs, raw integer handle pointers, and generated Rust FFI
layouts private.

Handles provide explicit `close()` methods and context-manager helpers. Use
`__del__`, weak reference finalizers, and GC callbacks for leak reporting. Use
them for cleanup only for native resources whose release function is documented
as thread-independent and infallible.

The Python GIL is separate from the native owner-thread model. Wrappers rely on
native owner-thread validation and synchronize Python-owned live/released state
when releasing the GIL.

Represent JSON-like descriptor values with Python values and reject non-finite
floats.

Texture frame access uses callback-scoped helpers. The callback must finish
synchronously because the native frame is released when it returns.

Native callbacks from MapLibre worker, network, logging, or render-related
threads use native queues to hand work back to Python code while holding the
GIL. Matching resource-provider requests complete later through a one-shot
Python request object. Custom geometry fetch and cancel callbacks notify Python
through the same queue; Python posts tile data later through the owner-thread
map API. Logging uses a bounded best-effort queue. Resource transforms stay
synchronous by using native-owned rewrite rules configured from Python, not a
Python callback on the MapLibre network thread. Callback adapters catch Python
exceptions and convert failures to the documented C callback behavior.
