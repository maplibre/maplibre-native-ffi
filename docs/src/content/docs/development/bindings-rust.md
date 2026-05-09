---
title: Rust Binding Notes
description: Language-specific implementation notes for Rust bindings.
---

Tracking issue:
[Add Rust bindings and smoke example](https://github.com/maplibre/maplibre-native-ffi/issues/41).

The Rust binding exposes a safe low-level API in one Cargo package while keeping
generated C declarations private.

Package and module names:

```text
maplibre-native          Cargo package
maplibre_native          Rust crate
maplibre_native::sys     private bindgen output
```

Generate the internal C layer with `bindgen` from the public umbrella header.
Treat successful generation and compilation as the header bindability check for
this path. Keep bindgen types internal; public APIs expose Rust wrappers and
values.

The public crate owns native handles through `RuntimeHandle`, `MapHandle`,
`MapProjectionHandle`, and `RenderSessionHandle`. These owner-thread-affine
handle types are `!Send` and `!Sync`. Maps retain their runtime, render sessions
retain their map, and parent state tracks live children without strong
ownership. With those invariants, `Drop` destroys native handles on the owner
thread. Explicit `close` or `destroy` methods return `Result<()>` when callers
want native status and diagnostics.

Status-returning C calls become `Result<T, Error>` with copied same-thread
diagnostics.

Use Rust-owned semantic descriptor and event types. Materialize C descriptor
graphs in temporary Rust-owned storage at the call boundary, and copy borrowed
native output before its borrow window ends. Encode `&str` as UTF-8 and reject
embedded `NUL` bytes for null-terminated `const char*` inputs. Runtime events
use copied source metadata, such as a Rust-assigned `MapId`, rather than cloned
map handles.

Represent backend-native addresses with an opaque `NativePointer` value. Raw
pointer conversion is unsafe and transfers no ownership. Texture frame access
uses a closure-scoped helper that acquires the native frame, exposes unsafe
backend-pointer accessors only during the callback, and releases the frame on
scope exit.

Callback trampolines catch Rust panics and convert them to the documented C
callback behavior. Callback state is stored strongly for the native owner scope
and is thread-safe where MapLibre may call from worker, network, logging, or
render-related threads. Resource provider request wrappers are `Send`, enforce
one-shot completion, and release the C request handle exactly once.

Map C enums to Rust enums with explicit conversions. Use `bitflags` for
user-visible masks and hide C field masks behind option structs, builders, or
setters. Mark public C-backed enums `#[non_exhaustive]`; add `Unknown(raw)` for
values read from native output where forward compatibility matters.
