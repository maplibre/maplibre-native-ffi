---
title: Java Bindings
description: Design rules for safe low-level Java FFM bindings.
sidebar:
  order: 3
---

## Scope

The Java binding is a safe low-level binding over the public C API. It exposes
the C API's runtime, map, render session, event, callback, and render target
model with Java ownership, error, memory, and thread-safety conventions.

Higher-level Java and Kotlin adapters build on this layer. JavaFX, Compose,
LWJGL, Skija, and other integrations can own UI lifecycle, dispatch, rendering
policy, and application-level map objects while delegating native calls to this
binding.

The binding uses the Java Foreign Function & Memory API. It targets the final
FFM API available in modern JDKs. Android and other JVMs where FFM is
unavailable or undesirable are covered by the separate
[Java Android binding](./bindings-java-android/) and its
[JNI tracking issue](https://github.com/maplibre/maplibre-native-ffi/issues/47).
The modern Java FFM work is tracked in
[issue 45](https://github.com/maplibre/maplibre-native-ffi/issues/45).

## Package And API Shape

Packages provide project context. Class names describe the C concept they wrap.

Owned long-lived native objects use a `Handle` suffix:

```text
org.maplibre.ffi.RuntimeHandle
org.maplibre.ffi.MapHandle
org.maplibre.ffi.RenderSessionHandle
```

`Handle` means the object wraps a closeable native object with an identity used
across multiple operations. Java-owned values, descriptors, events, copied data,
and one-shot snapshots omit the suffix:

```text
CameraOptions
MapOptions
AnimationOptions
TextureImageInfo
ResourceRequest
ResourceResponse
RuntimeEvent
JsonSnapshot
OfflineRegionSnapshot
OfflineRegionList
```

Keep public Java names close to the C concepts. Rename where Java readability or
namespace clarity benefits.

## Binding Layers

Separate generated FFM access from the safe public binding.

```text
org.maplibre.ffi.internal.c
  Generated FFM bindings from the public C headers.

org.maplibre.ffi.internal
  Status conversion, diagnostics, handle state, arenas, and callback bridging.

org.maplibre.ffi
  Safe low-level Java API.
```

Generate the internal C layer with `jextract`. Treat successful generation as
the header bindability check. The public Java layer is handwritten and wraps the
generated layer with stable names, ownership rules, diagnostics, and lifetime
control.

The public API keeps `Arena`, `MethodHandle`, and generated C layout classes
internal. It exposes `MemorySegment` only through explicitly unsafe accessors for
backend-native handles already exposed by the C API, such as
`textureUnsafe()` or `deviceUnsafe()` on a callback-scoped texture frame.

## Java Version

Target JDK 22 or newer. Earlier JDKs used preview or incubator FFM APIs that
produce a different binding surface.

Older JVM support belongs in the separate JNI binding path.

## Status And Diagnostics

Status-returning C calls become Java methods that either complete normally or
throw unchecked exceptions.

Map C status categories to stable Java exception classes:

```text
InvalidArgumentException
InvalidStateException
WrongThreadException
UnsupportedException
NativeException
MapLibreFfiException
```

When a native call returns a non-OK status, read the C thread-local diagnostic
immediately on the same thread and include it in the exception. Another C call on
that thread may replace the diagnostic.

Let the C API validate native arguments and native state. The Java layer checks
Java-owned state such as closed wrappers, active callback-scoped borrows, and
one-shot resource request completion.

## Owned Handles

Every long-lived C-owned opaque handle maps to an `AutoCloseable` Java
`*Handle`.

```java
try (RuntimeHandle runtime = RuntimeHandle.create(runtimeOptions);
     MapHandle map = MapHandle.create(runtime, mapOptions);
     RenderSessionHandle session = map.attachOwnedTexture(textureOptions)) {
    session.renderUpdate();
}
```

A handle stores:

- the native pointer;
- parent handles needed for native validity;
- open or closed state;
- optional debug leak context.

`close()` calls the matching C destroy function. A successful close makes later
close calls no-ops. If the C API reports `MLN_STATUS_WRONG_THREAD`, the binding
throws `WrongThreadException` with the native diagnostic.

Parent handles stay reachable while child handles are live. `MapHandle` keeps
its `RuntimeHandle` reachable. `RenderSessionHandle` keeps its `MapHandle`
reachable.

A `Cleaner` reports leaked handles in debug builds. The report includes the
native handle type, native pointer value, and allocation stack trace when debug
leak tracking is enabled. It does not destroy thread-affine native handles.

## Owner Threads

Mirror the C API's owner-thread model in documentation and exceptions.

Runtime creation records the runtime owner thread in native code. Map creation
currently runs on the runtime owner thread and makes that same thread the map
owner thread. Surface and texture attachment currently create render sessions
whose session owner thread is the map owner thread.

The Java layer does not duplicate owner-thread validation for ordinary calls.
Native `MLN_STATUS_WRONG_THREAD` results become `WrongThreadException`.

Keep Java type boundaries aligned with C owner concepts:

```text
RuntimeHandle       runtime owner thread in C
MapHandle           map owner thread in C
RenderSessionHandle session owner thread in C
```

This leaves room for a future C API that exposes render sessions owned by a
render thread distinct from the map owner thread. If Java needs to inspect owner
threads directly, add a C getter instead of maintaining a second source of truth.

Resource provider request completion follows the C API and may run from any
thread.

## Options And Transparent Structs

Model C option structs as Java-owned descriptor objects. Mutating descriptor
methods use `set...`, return `this`, and update any corresponding field mask.

```java
MapOptions options = MapOptions.defaults()
    .setSize(width, height)
    .setScaleFactor(scaleFactor)
    .setMapMode(MapMode.CONTINUOUS);
```

Field-mask structs use `empty()` plus explicit setters and clearers:

```java
CameraOptions camera = CameraOptions.empty()
    .setCenter(latitude, longitude)
    .setZoom(12.0)
    .clearBearing();
```

The binding initializes `size` fields and masks internally. Java callers set
semantic fields, not ABI bookkeeping fields.

Immutable value objects use `with...` methods. A type uses either mutating
`set...` methods or immutable `with...` methods, not both.

Most input descriptors store Java fields and materialize native structs into a
short-lived arena at the call boundary. Use object-owned native memory when the
Java object represents storage that C fills or later consumes.

## Native Memory

Use arenas according to lifetime and purpose.

```text
per-call confined arena
  MapOptions, CameraOptions, FeatureStateSelector, temporary UTF-8 strings,
  out parameters for create functions, and scratch buffers.

object-owned auto arena
  Small transparent native structs whose native storage is part of a Java
  object, such as a reusable TextureImageInfo native view when direct struct
  reuse is useful.

explicit native buffer
  Large caller-owned storage, such as CPU texture readback buffers reused across
  frames.

runtime-owned callback arena
  Resource transform and resource provider upcall stubs registered with a
  runtime.
```

The default descriptor pattern is Java-owned fields plus per-call native
materialization:

```java
try (Arena arena = Arena.ofConfined()) {
    MemorySegment nativeOptions = options.toNative(arena);
    check(nativeCall(handle, nativeOptions));
}
```

Small transparent structs may use GC-managed native memory when they do not need
deterministic release. Large buffers and native-state resources use explicit
ownership and `AutoCloseable`.

A Java heap array works well when C writes caller-provided storage during one
call and Java owns the result. An explicit native buffer works well when the
caller needs a stable native address or wants to reuse large off-heap storage.

## Callback-Scoped Borrows

A callback-scoped borrow is native data exposed only during a Java callback. The
binding acquires the native borrow before invoking the callback and releases it
in a `finally` block after the callback returns or throws.

Owned texture frames use callback-scoped access:

```java
session.withMetalOwnedTextureFrame(frame -> {
    MemorySegment texture = frame.textureUnsafe();
    MemorySegment device = frame.deviceUnsafe();
});
```

The frame view is valid only during the callback. The frame type is not publicly
closeable. Its unsafe native accessors check that the frame is active.

The native session already rejects nested acquisition, render updates, resize,
detach, and destroy while a frame is acquired. The Java wrapper relies on those
native checks and always releases the frame after the callback scope ends.

## Borrowed Data

Borrowed C data becomes copied Java data unless it is exposed through a
callback-scoped borrow.

`pollEvent()` copies runtime events before it returns. Event payload pointers,
messages, and strings never escape their C event storage window.

Snapshot objects own native snapshot storage. Values read from a snapshot become
copied Java values unless the API exposes a view object tied to the snapshot's
lifetime. The Java API does not expose free-floating borrowed views.

Backend-native handles returned from acquired texture frames are
callback-scoped borrows.

## Events

Expose runtime event polling as copied Java values.

```java
Optional<RuntimeEvent> event = runtime.pollEvent();
```

A drain helper may exist when it has the same semantics:

```java
runtime.drainEvents(consumer);
```

Events remain runtime-owned in the C API. Java event objects are independent of
the next native poll.

`RuntimeHandle` keeps a registry of live `MapHandle` wrappers keyed by native map
pointer. When a map-originated event contains a source pointer, the binding can
attach the matching `MapHandle` to the copied Java event. If no wrapper is live,
the event carries only copied source kind and native identity metadata for
diagnostics.

The low-level binding preserves event names and payload categories close to the
C API. Translating events into listeners, flows, coroutines, or UI state belongs
to adapters above this layer.

## Native Callbacks

Keep callback lifetimes explicit and runtime-scoped.

A Java callback is a Java interface implementation stored by the binding. The
binding creates an FFM upcall stub for a static adapter method. Native code calls
the stub as a C function pointer. The adapter receives raw FFM arguments, copies
or wraps them according to this document, invokes the Java interface method, and
converts the result back to C data.

Resource transforms and resource providers store Java callback implementations
strongly for the lifetime required by the C API. Their upcall stubs live in a
runtime-owned arena that outlives all native uses.

Callbacks catch Java exceptions and convert them to the documented C callback
behavior. Exceptions do not unwind through native code.

Java callback documentation carries forward C callback restrictions that remain
visible to users. For example, resource provider callbacks may run on worker or
network threads, so the Java interface documentation states that implementations
must return quickly and must not call map or runtime methods from the callback.
Borrowed request fields are copied before the Java method returns when the
binding needs them later.

A handled resource request uses a Java object that owns the provider's reference
to the C request handle. It enforces one-shot completion and exactly-once
release. Completion and cancellation checks may run from any thread when the C
API allows it.

## Render Sessions And Render Targets

`RenderSessionHandle` represents one attached render target for one map. Current
C attachment APIs make the session owner thread the map owner thread. The Java
type remains distinct because render sessions have separate lifecycle and render
target state.

Attach methods return a `RenderSessionHandle`:

```java
try (RenderSessionHandle session = map.attachVulkanBorrowedTexture(descriptor)) {
    session.renderUpdate();
}
```

Surface descriptors and caller-owned texture descriptors contain backend-native
handles. The Java binding treats those handles as borrowed. The caller keeps
backend objects valid and synchronized for the lifetime documented by the C API.

Session-owned texture targets expose rendered backend objects through
callback-scoped frame access. CPU readback APIs copy into Java arrays,
`ByteBuffer`, or explicit native buffers and return copied `TextureImageInfo`
metadata.

## Unsafe Escape Hatches

Backend interop requires raw native handles in specific render-target APIs.
Unsafe accessors are limited to those APIs.

Name those accessors with an `Unsafe` suffix:

```java
MemorySegment textureUnsafe();
MemorySegment imageUnsafe();
MemorySegment deviceUnsafe();
```

Unsafe accessors document the scope in which the returned native handle is
valid. They do not transfer ownership.

## Constants And Enums

Expose C enum domains as Java enums when the domain is closed and type-safe.
Map values explicitly. Java enum ordinals are not ABI values.

Expose C bit masks as `EnumSet` values in the public Java API. The generated
internal FFM layer keeps raw integer constants internal.

Output values that may grow across C API versions use stable unknown-value
representations where forward compatibility matters.

## Testing

Test Java adaptation, not the entire C API behavior already covered by C ABI
tests.

Cover:

- generated FFM binding as part of the build;
- library loading through the generated symbols;
- status-to-exception mapping and immediate diagnostic extraction;
- `AutoCloseable` close behavior and parent-child reachability;
- cleaner leak diagnostics in debug mode;
- descriptor-to-native `size` field initialization and field-mask encoding;
- copied runtime event payloads across repeated polls;
- snapshot copy behavior and snapshot-scoped view invalidation when views exist;
- FFM upcall lifetime and Java exception conversion;
- resource provider Java wrapper rules: pass-through, one-shot completion,
  asynchronous completion, cancellation, and release;
- texture frame callback acquisition and guaranteed release after normal return
  and exceptions;
- Java array, `ByteBuffer`, and explicit native-buffer readback paths.

## Design Boundary

This layer owns native handle lifetimes, memory safety, diagnostics, event
copying, and callback bridging. Adapter projects own UI lifecycle, dispatch,
render loop policy, and application-level map abstractions.
