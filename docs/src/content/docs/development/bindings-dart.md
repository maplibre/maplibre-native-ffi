---
title: Dart Binding Notes
description: Language-specific implementation notes for Dart bindings.
---

Tracking issue:
[Add Dart bindings](https://github.com/maplibre/maplibre-native-ffi/issues/51).

The Dart binding uses `dart:ffi` with private `ffigen` output over the public C
umbrella header. Public APIs expose Dart-owned handles, descriptors, exceptions,
copied events, and opaque native pointers rather than generated ABI classes.

Package and library names:

```text
maplibre_native                         Dart package
package:maplibre_native/maplibre_native.dart
lib/src/ffi/                             private ffigen layer
```

Use Dart 3.10+ for the first package so native asset and build-hook packaging is
available. Flutter support is a distribution concern for mobile and desktop
native libraries; the low-level binding stays independent of Flutter widgets,
platform channels, and UI integration. Flutter web is out of scope for this C
ABI binding.

Owned native objects use `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`,
and `RenderSessionHandle` classes with explicit `close()` or `destroy()`
methods. `NativeFinalizer` is a leak-reporting or proven-safe cleanup aid, not
the normal ownership mechanism for owner-thread-affine handles.

Status-returning C calls throw a `MapLibreNativeException` with a stable status
enum and copied same-thread diagnostic.

Use Dart-owned descriptor and event types. Materialize C descriptor graphs at
the native boundary, encode strings as UTF-8, reject embedded `NUL` bytes for
null-terminated strings, and copy borrowed native output before its borrow
window ends. Typed-data views over native memory are valid only for documented
scoped access.

`NativePointer` is a small opaque wrapper around `Pointer<Void>` or an address
integer. It transfers no ownership and grants no memory access.

Expose polled events first. Resource-provider APIs should use a native
trampoline that copies request data, posts to Dart, and completes request
handles later. Avoid making experimental isolate-group callbacks a core design
dependency.
