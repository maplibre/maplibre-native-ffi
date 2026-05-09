---
title: Python Binding Conventions
description: Language-specific implementation conventions for Python bindings.
---

Resources:

- Tracking issue:
  [#49](https://github.com/maplibre/maplibre-native-ffi/issues/49)
- [PyO3 user guide](https://pyo3.rs/)
- [maturin user guide](https://www.maturin.rs/)
- [Python C API: initialization, finalization, and threads](https://docs.python.org/3/c-api/init.html)

The Python binding targets CPython 3.14+ and exposes a safe low-level API. Build
the `_native` extension with PyO3 and package it with maturin.

Publish separate wheels for each CPython ABI:

- `cp314` for standard CPython;
- `cp314t` for free-threaded CPython.

Generated C and Rust declarations stay private. Build `_native` with the shared
internal crates defined by the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/).
Python and Node share Rust ABI adaptation with the public Rust crate. Each
binding owns its public API, lifetime rules, exception behavior, callbacks, and
packaging. Binding-owned shared state uses explicit synchronization.

PyO3 is the supported runtime binding path.

Use Python classes, dataclasses, enums, exceptions, `bytes`, writable buffer
inputs, and opaque `NativePointer` values at the public surface. Keep native
implementation details private: PyO3 internals, raw C structs, raw integer
handle pointers, and generated Rust FFI layouts.

Handles provide explicit `close()` methods and context-manager helpers. Use
`__del__`, weak reference finalizers, and GC callbacks for leak reporting. Use
them for cleanup only for native resources whose release function is documented
as thread-independent and infallible.

The Python GIL is separate from the native owner-thread model. Wrappers rely on
native owner-thread validation and synchronize Python-owned live/released state.
Python event-loop integration belongs in adapters above this layer.

Represent JSON-like descriptor values with Python values. Reject non-finite
floats.

Texture frame access uses callback-scoped helpers. The callback finishes
synchronously because the native frame is released when it returns.

Native callbacks from MapLibre worker, network, logging, or render-related
threads enqueue work for Python. Matching resource-provider requests complete
later through a one-shot Python request object. Custom geometry fetch and cancel
callbacks enqueue Python notifications; Python posts tile data later through the
owner-thread map API. Logging uses a bounded best-effort queue. Resource
transforms use native-owned rewrite rules configured from Python. Callback
adapters catch Python exceptions and convert failures to the documented C
callback behavior.
