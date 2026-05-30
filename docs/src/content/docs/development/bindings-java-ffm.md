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

## Architecture

The binding layers public API over generated C declarations across concept
packages. The root package contains the process-global `Maplibre` entry point.
Public subpackages group related concepts, such as `runtime`, `map`, `render`,
`resource`, `style`, `geo`, and `camera`.

Internal packages follow the implementation role: `internal.loader` loads the
native library, `internal.memory` owns FFM memory helpers, `internal.struct`
materializes C descriptors and copies native results, and `internal.status`
converts native status codes. The `internal.c` package contains generated
`jextract` declarations only.

The `org.maplibre.nativeffi` module exports public concept packages. Internal
packages remain unexported implementation details.

FFM types (`Arena`, `MemorySegment`, `MethodHandle`, generated layout classes)
stay internal. Public APIs pass backend-native handles as `NativePointer` and
convert at the generated layer boundary.

The internal C layer is entirely `jextract` output—do not hand-edit it. Refresh
when the C API changes:

```sh
mise run //bindings/java-ffm:jextract:update-includes
mise run //bindings/java-ffm:build
```

The binding calls `NativeAccess.ensureLoaded()` before touching generated
classes. The lookup order is:

1. exact library file path from `org.maplibre.nativeffi.library.path`;
2. exact library file path from `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`;
3. `System.loadLibrary("maplibre-native-c")` through `java.library.path`.

## Public Types

Process-global operations live as static methods on `Maplibre`. Long-lived
native objects follow the shared `Handle` convention and implement
`AutoCloseable`.

Immutable copied values are records. Mutable descriptor classes represent
field-mask structs. They use fluent accessors and mutators: `clip()` reads the
value, `clip(value)` sets the value and returns `this`, `clearClip()` clears
presence, and `hasClip()` reports presence. Internal materializers write C
`size` fields and masks.

C enums map to Java enums. For output that may drift across C ABI versions, the
enum includes an `UNKNOWN` variant and preserves the raw native value. C bit
masks become `EnumSet<T>`.

JSON and geometry data model as Java value trees: sealed interfaces, immutable
record variants, singletons for empty or null values. C `uint64_t` values map to
`long` with the bit pattern preserved. JSON object member order and duplicate
keys are preserved. Input trees materialize into temporary native descriptor
graphs at the call boundary; native snapshots and result views copy into
independent Java values before releasing native handles. Java-side depth limits
apply before native materialization.

Runtime polling returns copied Java events. Unknown payloads become
`RuntimeEventPayload.Unknown`. Native result and list handles stay internal;
internal readers copy contents into Java records or lists, then release the
native handle in `finally`.

## Handle Lifetime

Handle state lives in `HandleState`: release state, parent references, leak
reporting. Successful `close()` releases once; later closes no-op. A child holds
its parent wrapper strongly while live, because native validity depends on it.
`MapProjectionHandle` is the exception—it owns a standalone transform snapshot
after creation and does not depend on its source `MapHandle`.

Owner-thread-affine methods run on the calling Java thread. The binding does not
dispatch internally. Native wrong-thread statuses become `WrongThreadException`.

`Status.check(…)` throws unchecked `MaplibreException` subclasses carrying
`MaplibreStatus`, the raw status code, and the copied diagnostic. The binding
validates Java-owned state—wrapper lifetime, callback scope, descriptor depth,
one-shot completion, buffer and string shapes—and lets the C API validate native
arguments, state, and ranges.

Cleaner callbacks report leaks but do not destroy thread-affine native handles,
because cleaners run on an arbitrary GC thread. Correct cleanup flows through
`AutoCloseable.close()` on the owner thread.

## FFM Memory

Per-call storage and temporary descriptors use confined arenas. Shared arenas
are reserved for callback state or reusable buffers that outlive one call.
Reusable large byte storage uses `NativeBuffer`.

Pointer out-parameters start as `MemorySegment.NULL`. C `size` fields are set
through native default constructors or internal materializers.

`MemoryUtil` and `CoreStructs` handle UTF-8 strings, string views, and copied
borrowed data. Null-terminated C string inputs reject embedded NUL.

`NativePointer` is a borrowed address value—it grants no memory access and
transfers no ownership. `MemorySegment.ofAddress()` conversions stay internal
and limited to APIs whose C contract accepts opaque host pointers.

`NativeBuffer` owns off-heap bytes until `close()`. Synchronize access while a
native readback or upload call borrows its segment.

## Callbacks

Callback objects, upcall stubs, and arenas are stored for the owner scope
defined by the C API.

Upcall stubs may run on MapLibre worker, network, logging, or render-related
threads, so callback state is thread-safe. Each upcall catches `Throwable` and
converts it to the C callback's documented return behavior.

When replacing a callback, install the new native descriptor before closing the
old Java state. If native installation fails, close the replacement state and
keep the previous state active.

Resource transform callbacks copy the request URL before invoking Java. The
binding passes replacement URLs through the C API response helper so native
copies temporary Java storage before the callback returns.

Resource provider callbacks copy the request into `ResourceRequest` before user
code runs. `ResourceRequestHandle` owns the provider's native request reference
only while Java handles the request or completes it inline: it enforces one-shot
completion and releases the reference exactly once. A handled request may
complete during the callback or later from another thread when the C API allows
it.

Custom geometry source callbacks are map/style scoped. They catch user failures,
track active upcalls, and delay arena release until in-flight callbacks finish.
Java callbacks that need map methods hand work back to the map owner thread.

## Render Targets

Render target descriptors are mutable Java objects. Surface and borrowed-texture
descriptors use `NativePointer` for host-owned backend handles.

`RenderSessionHandle` owns one attached target for one map and keeps the map
alive. Closing the map while a session is live reports native invalid state.

Texture readback supports reusable off-heap storage through `NativeBuffer` and a
convenience path that returns a copied `PremultipliedRgba8Image`.

Owned texture frame access uses explicit `AutoCloseable` frame handle APIs. The
handle acquires the native frame, exposes copied metadata and scoped
`NativePointer` values, and keeps the MapLibre-owned texture alive until callers
close it. Callers synchronize GPU use, close the handle on the render session
owner thread, and close it before resize, another render update, detach, or
session destruction. Scoped frame values and pointers reject access after the
handle closes.
