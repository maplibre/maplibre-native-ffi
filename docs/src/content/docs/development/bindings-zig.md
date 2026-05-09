---
title: Zig Binding Notes
description: Language-specific implementation notes for Zig bindings.
---

Tracking issue:
[Add reusable Zig bindings](https://github.com/maplibre/maplibre-native-ffi/issues/42).

The Zig binding exposes one safe low-level package while keeping direct C
interop private.

Package and module names:

```text
maplibre_native          Zig package
maplibre_native          public module
maplibre_native.c        private @cImport layer
```

Generate the internal C layer with `@cImport` from the public umbrella header.
Treat successful compilation as the header bindability check for this path. Keep
imported C types internal; public APIs expose Zig handles, values, errors, and
copied events.

Owned native objects use `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`,
and `RenderSessionHandle`. Handles have explicit `deinit` or `destroy` methods.
Release methods return the binding error set and copy native diagnostics because
thread-affine destruction can fail. Parent state preserves native validity while
children are live.

Status-returning C calls map to a stable Zig error set. Since Zig errors do not
carry payloads, the binding copies `mln_thread_last_error_message()` into
binding-owned diagnostic storage.

Use Zig-owned semantic descriptor and event types. Materialize C descriptor
graphs in scoped temporary storage at the call boundary. APIs that copy
variable-size native output take a `std.mem.Allocator`; returned owned values
provide `deinit` where needed. Accept UTF-8 `[]const u8`, reject embedded `NUL`
bytes for null-terminated inputs, and use byte lengths for `mln_string_view`.

Runtime event polling returns copied values. Map-originated events carry copied
source metadata, such as a binding-assigned `MapId`, rather than borrowed map
handles. A scoped helper may expose borrowed handles for the callback duration.

Backend-native addresses use a small `NativePointer` value backed by
`?*anyopaque` and transfer no ownership. Texture frame access uses a scope-bound
acquire/release helper.

Public callbacks use Zig function pointers plus context pointers. Internal
`callconv(.c)` trampolines adapt callbacks to the C ABI, convert Zig errors to
the documented callback behavior, and keep callback state alive for the native
owner scope.

Map C enum domains to Zig `enum(u32)` values with explicit conversions. Use
purpose-built mask wrappers or `packed struct(u32)` values for user-visible bit
masks, hide C field masks behind option structs or builders, and use an
`unknown: u32` union case for forward-compatible native output values.
