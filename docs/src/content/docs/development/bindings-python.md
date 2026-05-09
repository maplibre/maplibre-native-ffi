---
title: Python Binding Notes
description: Language-specific implementation notes for Python bindings.
---

Tracking issue:
[Add Python bindings](https://github.com/maplibre/maplibre-native-ffi/issues/49).

The Python binding targets CPython 3.11+ and exposes a safe low-level API while
keeping generated C and Rust declarations private. Use PyO3 with `abi3-py311`
and maturin where practical. Treat PyPy as an experimental source-build target
until CI covers it.

Package names:

```text
maplibre-native          PyPI distribution
maplibre_native          Python package
maplibre_native._native  private native extension
maplibre_native._internal
```

Build `_native` with PyO3 over the shared internal `maplibre-native-support`
crate and `maplibre-native-sys` declarations. This shares low-level Rust ABI
adaptation with the public Rust crate and Node add-on without wrapping the
public Rust API. The Python extension owns Python-facing lifetime, GIL,
exception, callback, and packaging behavior.

Do not ship `ctypes` or `cffi` as supported runtime binding paths.

Public APIs expose Python classes, dataclasses, enums, exceptions, copied event
objects, `bytes`, writable buffer inputs, and opaque `NativePointer` values.
They hide PyO3 internals, raw C structs, raw integer handle pointers, and
generated Rust FFI layouts.

Owned native objects use `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`,
and `RenderSessionHandle`. Handles provide explicit `close()` or `destroy()`
methods and context-manager helpers. `__del__`, weak reference finalizers, and
GC callbacks are leak-reporting aids unless the binding can prove owner-thread
destruction and parent-before-child order.

Status-returning C calls raise a stable `MapLibreNativeError` hierarchy with the
C status category and copied same-thread diagnostic. The Python GIL is separate
from the native owner-thread model, so wrappers rely on native owner-thread
validation and synchronize Python-owned live/released state when releasing the
GIL.

Use Python-owned descriptor and event types. Materialize C descriptor graphs and
JSON values at the call boundary, reject embedded `NUL` bytes for
null-terminated strings, reject non-finite floats for JSON-like values, and copy
borrowed native output before its borrow window ends.

`NativePointer` is an opaque borrowed backend-native address. Texture frame
access uses context-manager or callback-scoped helpers that acquire the native
frame, expose unsafe backend-pointer access only inside the scope, and release
the frame in a `finally` path.

The first release exposes polled events, explicit handle APIs, and queue-backed
resource-provider request objects. Python callback adapters acquire the GIL,
catch exceptions, and convert failures to the documented C callback behavior.
Logging may use a bounded best-effort queue; resource-transform and
custom-geometry callbacks stay out of the first public surface until a native
shim avoids running arbitrary Python on MapLibre threads.
