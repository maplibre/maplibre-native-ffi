---
title: Vala Binding Conventions
description: Language-specific implementation conventions for Vala bindings.
---

Resources:

- Tracking issue:
  [#119](https://github.com/maplibre/maplibre-native-ffi/issues/119)
- [Vala manual](https://docs.vala.dev/)
- [Vala bindings documentation](https://docs.vala.dev/developer-guides/bindings.html)
- [GObject API reference](https://docs.gtk.org/gobject/)

The Vala binding exposes a handwritten GLib/GObject-style low-level API over a
private raw `.vapi` for the public C headers.

Use stable Vala 0.56.x. Rely on established GLib and GObject features rather
than development-only Vala features.

Public handle types are `GLib.Object` wrappers. Each stores the native handle
privately and exposes explicit `close()` methods that throw
`MapLibreNative.Error`. Use `dispose` to release managed references. Use
`finalize` for leak reporting. Use them for native cleanup only for resources
whose release function is documented as thread-independent and infallible.

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

Ordinary low-level calls keep the C API's threading model visible. Higher-level
adapters can add `GLib.MainContext` dispatch helpers above this layer.

Callbacks use Vala delegates backed by C-compatible trampolines. Store callback
state strongly for the native owner scope, protect shared state for MapLibre
worker, network, logging, or render-related threads, and convert Vala/GLib
failures to the documented C callback behavior.

Use `GLib.Bytes` for immutable copied byte data, `uint8[]` or `GLib.ByteArray`
for mutable buffers, and copied Vala objects for events and snapshots.

Represent backend-native handles with an opaque `NativePointer` value that
stores a private address integer. Convert it to `void*` only inside the private
raw C layer.
