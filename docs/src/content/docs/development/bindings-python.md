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

## Architecture

The Python binding targets CPython 3.14+ and exposes a safe low-level package
over the C API's runtime, map, render, event, and callback model. It adapts
those concepts to Python ownership, exceptions, and buffers.

Build the native extension with PyO3 and maturin as `_native`. Keep PyO3 types,
raw C declarations, generated Rust layouts, pointer-sized handles, and callback
trampolines private. The Python package wraps `_native` with typed public
classes, enums, dataclasses, exceptions, and helper modules grouped by C API
concept.

Use the Rust bridge path rather than a direct Python C import. `_native` depends
on the shared Rust ABI-adaptation crates described in the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/),
then adds Python-specific lifetime state, exception conversion, GIL handling,
and the public wrapper boundary. Standard and free-threaded CPython use the same
source conventions, with narrow conditional code for PyO3 features and
synchronization.

## Public Surface

Long-lived native objects use the shared `Handle` suffix. Each handle exposes
`close()` and implements the context-manager protocol. A successful `close()`
releases the native object exactly once; later closes no-op. Failed native
destruction leaves the handle live for retry or inspection.

Use Python dataclasses or small classes for descriptors. `_native` materializers
write C `size` fields, masks, string views, and nested structs. Python callers
set semantic fields only. Closed C enum domains map to Python `Enum` classes
with explicit raw-value conversion. Output domains that may grow preserve
unknown raw values for diagnostics.

`NativePointer` is an opaque borrowed address value for backend-native objects.
It grants no memory access and transfers no ownership. Public APIs accept it
only where the C API already accepts an opaque host pointer.

Represent JSON-like inputs with ordinary Python values: `None`, `bool`, `int`,
finite `float`, `str`, `list`, and mapping items in iteration order. Reject
non-finite floats and strings with embedded NUL where the native input is a
null-terminated C string.

## Handles, the GIL, and Threads

The Python GIL is not the MapLibre owner-thread identity. A handle method runs
on the native thread that called it, even when that caller holds the GIL. The
binding does not dispatch internally to another thread or event loop. Native
`MLN_STATUS_WRONG_THREAD` results become `WrongThreadError` with the copied
native diagnostic.

Store handle state in synchronized Rust/PyO3-owned state: native pointer,
live/released flag, parent references, active borrow flags, and optional leak
context. Free-threaded CPython uses the same invariants without relying on a
process-wide GIL.

Child handles keep parent wrappers alive when native validity depends on the
parent. `MapProjectionHandle` follows the shared exception: after creation it
owns a standalone projection snapshot and does not keep the source `MapHandle`
alive for native validity.

Use `__del__`, weakref finalizers, and GC callbacks for leak reporting. They may
run on the wrong thread or during interpreter finalization. They destroy native
resources only when the release function is thread-independent and infallible.

## Exceptions and Diagnostics

Status-returning operations raise `MaplibreError` subclasses. Each exception
stores the mapped status kind, raw status value, and the thread-local native
diagnostic copied immediately after the failing C call. Unknown future status
values become an `UnknownStatusError` that preserves the raw value.

The binding validates Python-owned state before crossing into native code:
closed handles, active callback-scoped borrows, one-shot request completion,
string shape, buffer mutability, descriptor depth, and callback lifetime. The C
API validates native state, thread affinity, ranges, and MapLibre rules.
Callback adapters catch `BaseException` and convert failures to the C callback's
documented behavior. Python exceptions never unwind through native frames.

## Memory, Buffers, and Events

Materialize native inputs at the call boundary with Rust-owned temporary
storage. Copy borrowed native strings, event payloads, list entries, and result
views into Python-owned values before releasing native handles. Runtime polling
returns independent Python event objects; later polls do not mutate earlier
events.

Use `bytes` for immutable copied byte output. For caller-owned writable storage,
accept objects that expose a writable contiguous buffer, such as `bytearray` or
a suitable `memoryview`. Hold the buffer borrow only for the native call or the
explicit scoped operation. Release the GIL around long native calls only when
all accessed Python objects have been converted to Rust-owned storage or pinned
through a valid PyO3 buffer guard.

Session-owned texture frames use explicit frame handles. Safe metadata access
returns copied Python values. Backend pointer accessors return scoped
`NativePointer` values tied to the live frame; they reject use after close and
document caller synchronization duties.

## Callbacks

Native callbacks may arrive on MapLibre worker, network, logging, or
render-related threads. Callback state is thread-safe and remains live for the
native owner scope. Acquire the GIL only while creating Python objects or
running Python callables; keep C-visible storage valid until native code has
finished with it.

Use bounded queueing at the low-level Python boundary for callbacks that cannot
run Python safely and quickly on the native callback thread. Queue overflow
follows the callback contract: fail the request, drop a best-effort
notification, or report cancellation rather than blocking MapLibre worker,
network, logging, or render-related threads. Resource-provider callbacks copy
request fields, create a one-shot Python request object for matching requests,
and complete inline or later from an allowed thread. The request object releases
the native request handle exactly once. Non-matching requests pass through
before Python code runs when native routing rules can settle them.

Resource transforms use native-owned rewrite rules configured from Python, or a
synchronous Python callable only when the binding can honor the callback without
blocking unsafe native work. Custom geometry callbacks enqueue Python
notifications and require tile data, cancellation, and map mutation to return
through owner-thread map APIs. Logging uses a bounded best-effort queue. Higher
level adapters may add `asyncio`, GUI-loop, or executor policy above this layer.

## Render Targets

Render descriptors are Python values. Surface and texture descriptors use
`NativePointer` for host-owned backend handles; callers keep those objects valid
and synchronized for the C API's required lifetime.

`RenderSessionHandle` represents one attached target for one map and keeps the
map wrapper alive. Texture readback supports caller-owned writable buffers and a
convenience path returning copied image bytes plus metadata. Frame handles close
on the render-session owner thread and before resize, another render update,
detach, or session destruction.

## Testing

Python tests exercise the public package against the real native library. Focus
on adaptation invariants: exception mapping and diagnostics, context-manager
cleanup, wrong-thread propagation, free-threaded synchronization assumptions,
writable-buffer validation, copied events, callback queueing, and exactly-once
request completion.
