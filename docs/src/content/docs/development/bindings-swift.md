---
title: Swift Binding Conventions
description: Language-specific implementation conventions for Swift bindings.
---

Resources:

- Tracking issue:
  [#44](https://github.com/maplibre/maplibre-native-ffi/issues/44)
- [Imported C and Objective-C APIs](https://developer.apple.com/documentation/swift/imported-c-and-objective-c-apis)
- [Using imported C functions in Swift](https://developer.apple.com/documentation/swift/using-imported-c-functions-in-swift)
- [Swift C interoperability](https://developer.apple.com/documentation/swift/c-interoperability)

The Swift binding uses Swift's C importer over the public C headers, with a
private C module target and a public Swift target.

Use Swift 6 concurrency annotations for public types and callbacks. Keep
owner-thread-affine handles non-`Sendable`, and make callback state explicitly
`Sendable` only when it is safe to invoke from MapLibre worker, network,
logging, or render-related threads. Use Swift's stable C importer over the
public C headers.
[Safe C interop annotations](https://swift.org/documentation/cxx-interop/safe-interop/)
may inform future ergonomics, but the low-level binding remains anchored to the
shared C ABI.

Use final public classes for native handles. They provide explicit throwing
`close()` methods. Use `deinit` for leak reporting. Use it for cleanup only for
native resources whose release function is documented as thread-independent and
infallible.

Keep owner-thread-affine handles non-`Sendable`. Keep the low-level binding out
of `@MainActor`; the native owner thread is the creation and pump thread, not
necessarily the Apple main thread. UI adapters can add actor confinement above
this layer.

Callbacks use noncapturing `@convention(c)` trampolines and pass Swift state
through C `user_data` using `Unmanaged`. Balance retained callback state with
the native registration scope. Trampolines catch Swift errors and convert them
to the documented C callback behavior.

Use `Data` or `[UInt8]` for copied buffers. Use `withUnsafeBytes` and
`withUnsafeMutableBytes` only for call-duration borrows. Keep pointers derived
from Swift arrays, strings, or `Data` scoped to the borrow unless storage is
explicitly native-owned.

Represent backend handles with an opaque `NativePointer` value that stores a
private address integer. Convert it to `UnsafeRawPointer?` or
`UnsafeMutableRawPointer?` only inside the C module boundary.
