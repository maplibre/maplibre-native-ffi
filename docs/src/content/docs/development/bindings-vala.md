---
title: Vala Binding Conventions
description: Language-specific implementation conventions for Vala bindings.
---

Resources:

- Tracking issue:
  [#119](https://github.com/maplibre/maplibre-native-ffi/issues/119)
- [Vala manual](https://docs.vala.dev/)
- [Vala bindings documentation](https://docs.vala.dev/developer-guides/bindings.html)
- [Generating a VAPI with GObject Introspection](https://docs.vala.dev/developer-guides/bindings/generating-a-vapi-with-gobject-introspection.html)
- [GObject API reference](https://docs.gtk.org/gobject/)
- [GObject Introspection](https://gi.readthedocs.io/)
- [Rust `glib` crate](https://docs.rs/glib/)

The Vala binding is generated from a low-level GLib/GObject adapter. Vala's
idiomatic native library boundary is GObject Introspection: the project builds
an introspectable GLib library, generates GIR and typelib files, then runs
`vapigen` to produce the Vala API.

This binding targets the GLib ecosystem. The GLib layer preserves the C API's
runtime, map, render-session, and event model. UI widgets and application
lifecycle policy belong above this layer.

Build the GLib adapter library in Rust with the gtk-rs `glib` crate and GObject
infrastructure over the shared internal crates defined by the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/). The
adapter exports an annotated GObject-style C API for GObject Introspection.
Treat successful `g-ir-scanner`, typelib generation, `vapigen`, and Vala
compilation as the bindability check for this path.

The generated GIR and typelib are useful beyond Vala. They can also serve other
GObject Introspection consumers while still representing the same low-level
MapLibre Native FFI model.

Public handle types are GObject classes with the `Handle` suffix. Each object
stores the native C handle privately and exposes an explicit `close()` method.
`close()` reports native status through `GError`, which Vala presents as a
thrown error. Use `dispose` to release managed references. Use `finalize` for
leak reporting. Reserve finalization cleanup for resources whose release
function is documented as thread-independent and infallible.

Expose a public GLib error domain that maps the C status categories directly.
Vala sees the same domain as an `errordomain`:

```vala
public errordomain MapLibreNative.Error {
    INVALID_ARGUMENT,
    INVALID_STATE,
    WRONG_THREAD,
    UNSUPPORTED,
    NATIVE_ERROR
}
```

Use GLib data types for copied buffers, events, snapshots, and other public
values. Backend-native handles cross the public API as an opaque `NativePointer`
boxed value. The GLib adapter converts them to `void*`.

Ordinary low-level calls preserve the C API's owner-thread model and execute on
the calling thread. Expose runtime event polling as copied event values. GLib
signals may be emitted as a convenience while the owner thread drains runtime
events. Main-loop and UI integration belong in adapters above this layer.

Callback adapters store callback state for the native owner scope, protect it
for calls from native threads, and convert GLib/Vala failures to the documented
C callback behavior. Handled resource requests expose one-shot completion
objects and release the C request handle exactly once.
