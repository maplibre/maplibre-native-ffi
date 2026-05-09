---
title: Kotlin Multiplatform Binding Conventions
description: Language-specific implementation conventions for Kotlin Multiplatform bindings.
---

Resources:

- Tracking issue:
  [#47](https://github.com/maplibre/maplibre-native-ffi/issues/47)
- [Kotlin Multiplatform project structure](https://kotlinlang.org/docs/multiplatform/multiplatform-discover-project.html)
- [Kotlin/Native C interop](https://kotlinlang.org/docs/native-c-interop.html)
- [Kotlin/Native definition files](https://kotlinlang.org/docs/native-definition-file.html)

The Kotlin Multiplatform binding exposes one common safe low-level API.
`jvmMain` delegates to the
[Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/),
`androidMain` delegates to the
[Java JNI conventions](/maplibre-native-ffi/development/bindings-java-jni/), and
`nativeMain` wraps Kotlin/Native cinterop output. WASM and JS are out of scope
for now.

Source sets:

```text
commonMain   Common values, exceptions, descriptors, events, APIs, and expect declarations.
jvmMain      Actual implementations backed by the Java FFM binding.
androidMain  Actual implementations backed by the Java JNI binding.
nativeMain   Actual implementations backed by Kotlin/Native cinterop.
```

Configure Kotlin/Native `cinterop` against the public umbrella header and expose
its output only through an internal package. Treat successful cinterop
compilation as the header bindability check for Kotlin/Native.

Common APIs preserve the owner-thread behavior of their actual implementations.
Coroutine dispatcher and UI-thread routing belong in adapters above the common
low-level API.

Kotlin/Native C interop declarations are experimental. Keep
`@OptIn(ExperimentalForeignApi::class)` in `nativeMain` internals and actual
implementations. Common public APIs do not expose `kotlinx.cinterop` types,
`CPointer`, `COpaquePointer`, `CValue`, `CValuesRef`, `NativePlacement`,
`StableRef`, platform C type aliases, or cinterop struct classes.

Use Kotlin/Native storage by lifetime:

- `memScoped` for temporary structs, strings, out parameters, and scratch
  buffers;
- `nativeHeap` for storage whose address outlives one call and is released from
  the owning object;
- `ByteArray.usePinned` or `refTo()` for primitive arrays passed for one call;
- explicit native buffers for stable or reusable off-heap storage.

Kotlin/Native cinterop may map `const char*` function parameters to `String`.
Use that generated conversion only when the C API consumes or copies the string
before returning and the binding has already rejected embedded `NUL` characters.
Configure cinterop with `noStringConversion` where automatic conversion would
hide pointer lifetime or byte-length semantics.

The `nativeMain` callback layer creates non-capturing C function pointers with
`staticCFunction`. Callback state crosses the C boundary through `StableRef`
values stored in C `user_data` pointers. Create each `StableRef` when
registering the callback and dispose it exactly once after native code can no
longer call it. Callback state is thread-safe because callbacks may arrive on
MapLibre worker, network, logging, or render-related threads.
