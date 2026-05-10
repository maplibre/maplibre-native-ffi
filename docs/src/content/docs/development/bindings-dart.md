---
title: Dart Binding Conventions
description: Language-specific implementation conventions for Dart bindings.
---

Resources:

- Tracking issue:
  [#51](https://github.com/maplibre/maplibre-native-ffi/issues/51)
- [Dart C interop using `dart:ffi`](https://dart.dev/interop/c-interop)
- [`ffigen`](https://pub.dev/packages/ffigen)

The Dart binding uses `dart:ffi` with private `ffigen` output over the public C
umbrella header.

Use Dart 3.10+ so the package can use stable build hooks and code assets to
bundle or download native MapLibre libraries for `dart:ffi`. Flutter support is
a distribution concern for mobile and desktop native libraries; the low-level
binding stays independent of Flutter widgets, platform channels, and UI
integration. Flutter web is out of scope for this C ABI binding.

Keep `ffigen` output internal. Public APIs expose Dart handles, descriptors,
exceptions, events, and opaque native pointers rather than generated ABI
classes.

Use explicit `close()` or `destroy()` methods for native handles. Use
`NativeFinalizer` for leak reporting. Use it for cleanup only for native
resources whose release function is documented as thread-independent and
infallible.

Ordinary calls preserve caller execution and report native wrong-thread errors.
Isolate coordination and Flutter scheduling belong in adapters above this layer.

Define public descriptor and event types as Dart classes or records. Convert
descriptors to generated C structs at the FFI boundary, and copy native events
into Dart values before returning them. Typed-data views over native memory are
valid only for documented scoped access.

`NativePointer` is a small opaque wrapper around an address integer. Convert it
to `Pointer<Void>` only inside the FFI layer.

Matching resource-provider requests copy request data, post to Dart, and
complete later through one-shot request objects. Avoid making experimental
isolate-group callbacks a core design dependency.
