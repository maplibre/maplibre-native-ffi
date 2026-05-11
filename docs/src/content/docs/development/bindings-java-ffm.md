---
title: Java FFM Binding Conventions
description: Language-specific implementation conventions for Java FFM bindings.
---

Resources:

- Tracking issue:
  [#45](https://github.com/maplibre/maplibre-native-ffi/issues/45)
- [Foreign Function & Memory API](https://docs.oracle.com/en/java/javase/25/core/foreign-function-and-memory-api.html)
- [`jextract`](https://jdk.java.net/jextract/)
- [Java JNI conventions](/maplibre-native-ffi/development/bindings-java-jni/)

## Package Boundaries

Use three layers:

| Package                             | Purpose                                        |
| ----------------------------------- | ---------------------------------------------- |
| `org.maplibre.nativeffi`            | Public symbols.                                |
| `org.maplibre.nativeffi.internal`   | Native loading, conversion, and adapter logic. |
| `org.maplibre.nativeffi.internal.c` | Generated `jextract` declarations only.        |

Keep FFM types internal: `Arena`, `MemorySegment`, `MethodHandle`, and generated
C layout classes. Pass backend-native handles through public APIs as
`NativePointer`; convert them at the generated layer boundary.

Generate the internal C layer with `jextract`. When the C API changes, refresh
the symbol include argfile, then build:

```sh
mise run //bindings/java-ffm:jextract:update-includes
mise run //bindings/java-ffm:build
```

When wrapping a C function, add internal struct conversion when needed, call
`Status.check(...)` for native statuses, and test the real C call.

## Native Loading And Access

Call `NativeAccess.ensureLoaded()` before touching generated `jextract` classes.
The lookup order is:

1. exact library file path from `org.maplibre.nativeffi.library.path`;
2. exact library file path from `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`;
3. `System.loadLibrary("maplibre-native-c")` through `java.library.path`.

## Public Java Shape

Use static methods on `MapLibre` for process-global operations. Long-lived
native objects follow the shared `Handle` convention and implement
`AutoCloseable`.

Use records for immutable copied values. Defensively copy mutable inputs.

Use mutable descriptor classes for field-mask structs. Setters return `this`,
`clear...()` clears presence, and `has...()` reports presence. Internal
materializers write C `size` fields and masks.

Use Java enums for C enums. For native output that may drift across C ABI
changes, expose the mapped enum and the raw native value; map unknown values to
`UNKNOWN`. Represent public C bit masks with `EnumSet<T>`.

## Status And Validation

`Status.check(...)` throws unchecked `MapLibreException` subclasses. Each
exception carries `MapLibreStatus`, the raw status code, and the copied
diagnostic.

Validate Java-owned state in Java: wrapper lifetime, callback scope, descriptor
depth, one-shot completion, and buffer or string shapes that Java owns.

## Handles And Owner Threads

Store handle lifecycle in `HandleState`: release state, parent references, and
leak reporting. Successful `close()` calls release once; later closes no-op.
Cleaner callbacks report leaks; they do not destroy thread-affine native
handles.

Retain parent wrappers strongly when native validity depends on them.
`MapProjectionHandle` is standalone after creation.

Use the runtime's weak map registry to attach live `MapHandle` sources to copied
map events.

Owner-thread-affine methods run on the calling Java thread. Do not dispatch
inside the low-level binding. Native wrong-thread statuses become
`WrongThreadException`.

## Native Memory And Strings

Use confined arenas for per-call storage and temporary descriptors. Use shared
arenas only for callback state or other storage that outlives one call. Use
`NativeBuffer` for reusable large byte storage.

Initialize pointer out parameters to `MemorySegment.NULL`. Initialize C `size`
fields through native default constructors or internal materializers.

Use `MemoryUtil` and `CoreStructs` for UTF-8 strings, string views, and copied
borrowed data. Reject embedded NUL in null-terminated C string inputs.

`NativePointer` is a borrowed address value. It grants no memory access and
transfers no ownership. Keep `MemorySegment.ofAddress()` conversions internal
and limited to APIs whose C contract accepts opaque host pointers.

`NativeBuffer` owns off-heap bytes until `close()`. Synchronize access while a
native readback or upload call borrows its segment.

## JSON, GeoJSON, And Feature Values

Model JSON and geometry data as Java value trees: sealed interfaces, immutable
record variants, and singleton variants for empty or null values.

Represent C `uint64_t` values as `long` and preserve the bit pattern. Preserve
JSON object member order and duplicate keys.

Materialize input trees into temporary native descriptor graphs at the call
boundary. Copy native snapshots and result views into independent Java values
before releasing native handles. Apply Java-side depth limits before native
materialization.

## Events And Native Results

Runtime polling returns copied Java events. Represent unknown payloads as
`RuntimeEventPayload.Unknown`.

Keep native result and list handles internal. Internal readers copy their
contents into Java records or lists, then release the native handle in
`finally`.

## Native Callbacks

Store callback objects, upcall stubs, and arenas for the owner scope defined by
the C API.

Upcall stubs may run on MapLibre worker, network, logging, or render-related
threads. Use thread-safe callback state. Catch `Throwable` inside every upcall
and convert it to the C callback's documented return behavior.

When replacing a callback, install the new native descriptor before closing the
old Java state. If native installation fails, close the replacement state and
keep the previous state active.

Resource transform callbacks copy the request URL before invoking Java. Keep
response storage that C borrows after the callback alive until native consumes
it. Close per-thread response scratch storage on the next callback for that
thread and during runtime teardown.

Resource provider callbacks copy the request into `ResourceRequest` before user
code runs. `ResourceRequestHandle` owns the provider's native request reference
only when Java handles the request or completes it inline. It enforces one-shot
completion and releases the native request reference exactly once. A handled
request may complete during the callback or later from another thread when the C
API allows it.

Custom geometry source callbacks are map/style scoped. They catch user failures,
track active upcalls, and delay arena release until in-flight callbacks finish.
Java callbacks that need map methods hand work back to the map owner thread.

## Render Targets And Frame Access

Render target descriptors are mutable Java objects. Surface and borrowed-texture
descriptors use `NativePointer` for host-owned backend handles.

`RenderSessionHandle` owns one attached target for one map and keeps the map
alive. Closing the map while a session is live reports native invalid state.

Texture readback supports reusable off-heap storage through `NativeBuffer` and a
convenience path that returns a copied `PremultipliedRgba8Image`.

Owned texture frame access uses closure-scoped helpers. The helper acquires the
native frame, exposes copied metadata and scoped `NativePointer` values,
releases the frame in `finally`, and invalidates the frame scope after the
callback returns or fails. Scoped frame values and pointers reject access after
the callback.
