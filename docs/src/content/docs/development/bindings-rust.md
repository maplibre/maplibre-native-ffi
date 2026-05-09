---
title: Rust Binding Conventions
description: Language-specific implementation conventions for Rust bindings.
---

Resources:

- Tracking issue:
  [#41](https://github.com/maplibre/maplibre-native-ffi/issues/41)
- [Rust Nomicon: FFI](https://doc.rust-lang.org/nomicon/ffi.html)
- [`bindgen` user guide](https://rust-lang.github.io/rust-bindgen/)
- [Rust API Guidelines: FFI](https://rust-lang.github.io/api-guidelines/interoperability.html)

The Rust binding exposes one public safe low-level Cargo package while sharing
internal Rust crates with native-extension bindings such as Python and Node.

Generate `maplibre-native-sys` with `bindgen` from the public umbrella header.
Treat successful generation and compilation as the header bindability check for
this path. The `sys` crate mirrors the C ABI: raw extern functions, constants, C
layouts, and opaque pointer types.

`maplibre-native-support` is shared implementation glue above `sys`, not the
public safety layer. It provides shared helpers for status checking, diagnostic
copying, strings, descriptors, memory guards, callback boundaries, and
`NativePointer` utilities. Keep bindgen and support types internal; public APIs
expose Rust wrappers and values.

Owner-thread-affine handle types are `!Send` and `!Sync`. Maps retain their
runtime, render sessions retain their map, and parent state tracks live children
without strong ownership. With those invariants, `Drop` destroys native handles
on the owner thread. Explicit `close` or `destroy` methods return `Result<()>`
when callers want native status and diagnostics.

Runtime events use copied source metadata, such as a Rust-assigned `MapId`,
rather than cloned map handles.

`NativePointer` exposes backend-native addresses through explicit unsafe raw
pointer conversion. Texture frame access uses a closure-scoped helper that
acquires the native frame, exposes unsafe backend-pointer accessors only during
the callback, and releases the frame on scope exit.

Callback trampolines catch Rust panics and convert them to the documented C
callback behavior. Callback state is stored strongly for the native owner scope
and is thread-safe where MapLibre may call from worker, network, logging, or
render-related threads. Resource provider request wrappers are `Send`, enforce
one-shot completion, and release the C request handle exactly once.

Map C enums to Rust enums with explicit conversions. Use `bitflags` for
user-visible masks and hide C field masks behind option structs, builders, or
setters. Mark public C-backed enums `#[non_exhaustive]`; add `Unknown(raw)` for
values read from native output where forward compatibility matters.
