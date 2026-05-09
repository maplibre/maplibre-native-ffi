---
title: Swift Binding Notes
description: Language-specific implementation notes for Swift bindings.
---

Tracking issue:
[Add Swift bindings](https://github.com/maplibre/maplibre-native-ffi/issues/44).

The Swift binding uses Swift's C importer over the public C headers, with a
private C module target and a public Swift target.

Package and module names:

```text
MapLibreNative     Swift package/product/module
CMapLibreNative    private C target/module
```

Design for Swift 6 strict-concurrency compatibility, but do not require
experimental safe C or C++ interop for the first binding.

Public APIs expose final classes named `RuntimeHandle`, `MapHandle`,
`MapProjectionHandle`, and `RenderSessionHandle`. They provide explicit throwing
`close()` or `destroy()` methods. `deinit` is for leak reporting or best-effort
cleanup only when owner-thread destruction is guaranteed.

Status-returning C calls throw `MapLibreNativeError` values that preserve the C
status category and copied same-thread diagnostic.

Do not mark owner-thread-affine handles as `Sendable`. Do not put the low-level
binding under `@MainActor`; the native owner thread is the creation and pump
thread, not necessarily the Apple main thread. UI adapters can add actor
confinement above this layer.

Callbacks use noncapturing `@convention(c)` trampolines and pass Swift state
through C `user_data` using `Unmanaged`. Balance retained callback state with
the native registration scope. Trampolines catch Swift errors and convert them
to the documented C callback behavior.

Use `Data` or `[UInt8]` for copied buffers. Use `withUnsafeBytes` and
`withUnsafeMutableBytes` only for call-duration borrows. Never persist pointers
derived from Swift arrays, strings, or `Data` unless storage is explicitly
native-owned.

Represent backend handles with an opaque `NativePointer` value around
`UnsafeRawPointer?` or `UnsafeMutableRawPointer?`. It exposes no typed memory
access.

Distribute source packages through SwiftPM. Prebuilt Apple native artifacts
should use XCFrameworks, with separate device and simulator slices where
required.
