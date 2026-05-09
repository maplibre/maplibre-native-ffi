---
title: Vala / GObject Binding Notes
description: Language-specific implementation notes for Vala and GObject bindings.
---

Tracking issue:
[Add Vala bindings](https://github.com/maplibre/maplibre-native-ffi/issues/119).

The Vala binding exposes a GLib/GObject-style low-level API over the public C
headers while keeping raw C declarations private.

Package and namespace names:

```text
maplibre-native-vala     source/package name
maplibre-native-c.vapi   private raw C declarations
MapLibreNative           public Vala namespace
```

Build a handwritten public Vala wrapper over a private raw `.vapi`. GIR can be
revisited later if the project adds a GObject-shaped C facade; the current C API
is handle-, status-, diagnostic-, and callback-oriented.

Use stable Vala 0.56.x unless repository CI requires another compiler. Rely on
established GLib and GObject features rather than development-only Vala
features.

Public handle types are `GLib.Object` wrappers named `RuntimeHandle`,
`MapHandle`, `MapProjectionHandle`, and `RenderSessionHandle`. Each stores the
native handle privately and exposes explicit `close()` or `destroy()` methods
that throw `MapLibreNative.Error`. `dispose` and `finalize` are reference
cleanup and leak-reporting aids, not the primary owner-thread destruction path.

Define a public errordomain:

```vala
public errordomain MapLibreNative.Error {
    INVALID_ARGUMENT,
    INVALID_STATE,
    WRONG_THREAD,
    UNSUPPORTED,
    NATIVE_ERROR
}
```

Every status-returning C call either returns normally or throws this error with
the copied same-thread diagnostic.

Preserve native owner-thread validation. Methods run on the native owner thread
and translate `MLN_STATUS_WRONG_THREAD` into
`MapLibreNative.Error.WRONG_THREAD`. A wrapper may record the thread-default
`GLib.MainContext` for diagnostics or higher-level dispatch helpers, while
ordinary low-level calls keep the C API's threading model visible.

Callbacks use Vala delegates backed by C-compatible trampolines. Store callback
state strongly for the native owner scope, protect shared state for MapLibre
worker, network, logging, or render-related threads, and convert Vala/GLib
failures to the documented C callback behavior.

Use `GLib.Bytes` for immutable copied byte data, `uint8[]` or `GLib.ByteArray`
for mutable buffers, and copied Vala objects for events and snapshots. Encode
strings as UTF-8, reject embedded `NUL` bytes for null-terminated inputs, and
copy borrowed native output before its borrow window ends.

Represent backend-native handles with an opaque `NativePointer` value over
`void*`. It transfers no ownership and exposes no memory access.

Install the public `.vapi`, generated wrapper C/header artifacts if the Vala
layer is compiled to C, and a `pkg-config` file. Keep the raw C `.vapi` private
unless the project intentionally exposes the internal C layer.
