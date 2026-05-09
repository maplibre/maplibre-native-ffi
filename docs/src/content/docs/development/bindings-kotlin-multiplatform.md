---
title: Kotlin Multiplatform Binding Notes
description: Language-specific implementation notes for Kotlin Multiplatform bindings.
---

Tracking issue:
[Add Kotlin Multiplatform bindings](https://github.com/maplibre/maplibre-native-ffi/issues/47).

The Kotlin Multiplatform binding exposes one common safe low-level API.
`jvmMain` delegates to the
[Java FFM notes](/maplibre-native-ffi/development/bindings-java-ffm/),
`androidMain` delegates to the
[Java JNI notes](/maplibre-native-ffi/development/bindings-java-jni/), and
`nativeMain` wraps Kotlin/Native cinterop output. WASM and JS are out of scope
for now.

Source sets:

```text
commonMain
  org.maplibre.nativekit
    Common value types, exceptions, descriptors, events, safe low-level APIs,
    and expect declarations where platform storage differs.

jvmMain
  Actual implementations that delegate to org.maplibre.nativekit.ffm.

androidMain
  Actual implementations that delegate to org.maplibre.nativekit.jni.

nativeMain
  Actual implementations that wrap Kotlin/Native cinterop declarations.
```

Configure Kotlin/Native `cinterop` against the public umbrella header and expose
its output only through an internal package. Treat successful cinterop
compilation as the header bindability check for Kotlin/Native.

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
