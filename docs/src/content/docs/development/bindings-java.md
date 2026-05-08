---
title: Java Bindings
description: Design rules for safe low-level Java FFM bindings.
sidebar:
  order: 3
---

## Scope

The Java binding is a safe low-level binding over the public C API. It keeps the
runtime, map, render session, event, callback, and render target model visible
instead of hiding them behind a full Java map framework.

Higher-level Java and Kotlin adapters may build idiomatic APIs above this layer.
For example, a Compose adapter can own recomposition, coroutines, lifecycle,
image presentation, and application-level map objects while still delegating all
native calls to this binding.

This binding uses the Java Foreign Function & Memory API. It targets the final
FFM API available in modern JDKs rather than JNI, JNA, or preview-era Panama
APIs.

## Package And API Shape

Use packages to provide context. Avoid repeating the project or C prefix in every
class name.

Owned native resources use a common `Handle` suffix:

```text
org.maplibre.ffi.RuntimeHandle
org.maplibre.ffi.MapHandle
org.maplibre.ffi.RenderSessionHandle
org.maplibre.ffi.JsonSnapshotHandle
org.maplibre.ffi.OfflineRegionSnapshotHandle
org.maplibre.ffi.OfflineRegionListHandle
```

The suffix tells readers that the object wraps a native handle and must be
closed. Value objects, descriptors, events, and copied data do not use the
suffix:

```text
CameraOptions
MapOptions
AnimationOptions
TextureImageInfo
ResourceRequest
ResourceResponse
RuntimeEvent
```

Keep public Java names close to the C concepts. This layer may rename for Java
readability, but it does not invent a different map model.

## Binding Layers

Separate generated or hand-written FFM machinery from the public safe binding.

```text
org.maplibre.ffi.internal.c
  FFM symbol lookup, layouts, downcall handles, upcall stubs, C constants.

org.maplibre.ffi.internal
  Status conversion, diagnostics, handle state, owner-thread checks, arenas.

org.maplibre.ffi
  Safe low-level Java API.
```

The public API does not expose raw `MemorySegment`, `Arena`, `MethodHandle`, or
C layout classes except through explicitly unsafe escape hatches needed for
backend-native interop.

`jextract` may help validate that headers remain bindable. The shipped API is
allowed to use curated hand-written FFM stubs when that gives clearer naming,
version checks, diagnostics, and lifetime control.

## Java Version

Target JDK 22 or newer for the low-level FFM binding. Earlier JDKs used preview
or incubator variants of the API and produce a different binding surface.

Support for older JVMs belongs in a separate artifact if the project later needs
it. Do not compromise this binding's design to match preview-era FFM APIs.

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

Every native call that returns a non-OK status reads the C thread-local
diagnostic immediately on the same thread and includes it in the exception. The
binding does not defer diagnostic lookup because another C call on the same
thread may replace the diagnostic.

The C status remains authoritative. Java may pre-check open state, owner thread,
nulls, and obvious argument ranges for clearer failures, but native validation
still runs.

## Owned Handles

Every C-owned opaque handle maps to an `AutoCloseable` Java `*Handle`.

```java
try (RuntimeHandle runtime = RuntimeHandle.create(runtimeOptions);
     MapHandle map = MapHandle.create(runtime, mapOptions);
     RenderSessionHandle session = map.attachOwnedTexture(textureOptions)) {
    session.renderUpdate();
}
```

A handle stores:

- the native handle pointer;
- the owner thread recorded at creation;
- strong references to parent handles needed for native validity;
- closed/open state;
- optional leak diagnostics for debug builds.

`close()` performs the corresponding C destroy function on the required owner
thread. Closing is idempotent after a successful close. Closing a handle from the
wrong thread throws before calling C, and C wrong-thread status is still handled
as a safety net.

Parent handles stay reachable while child handles are live. For example,
`MapHandle` keeps its `RuntimeHandle` reachable, and `RenderSessionHandle` keeps
its `MapHandle` reachable.

The binding may register a `Cleaner` for diagnostics, but a cleaner does not
directly destroy thread-affine native handles. Cleaner threads are not the owner
threads required by the C API. Managed higher-level adapters may enqueue cleanup
to their owner thread, but this low-level layer relies on explicit close.

## Owner Threads

Mirror the C API's owner-thread model.

Runtime creation records the runtime owner thread. Map creation currently
happens on the runtime owner thread and makes that same thread the map owner
thread. Surface and texture attachment currently create render sessions whose
session owner thread is the map owner thread.

Store the owner thread on each handle independently even when the current C API
makes them the same:

```text
RuntimeHandle       runtime owner thread
MapHandle           map owner thread
RenderSessionHandle session owner thread
```

This keeps the Java object model ready if the C API later allows render sessions
to be owned by a render thread distinct from the map owner thread.

Thread-affine methods check the relevant handle owner, not only the runtime
owner. Resource provider request completion follows the C API and may run from
any thread.

## Options And Transparent Structs

Model C option structs as Java-owned descriptor objects. Descriptor methods that
mutate the receiver use `setFoo` and return `this` for chaining.

```java
MapOptions options = MapOptions.defaults()
    .setSize(width, height)
    .setScaleFactor(scaleFactor)
    .setMapMode(MapMode.CONTINUOUS);
```

Field-mask structs use `empty()` and explicit setters or clearers:

```java
CameraOptions camera = CameraOptions.empty()
    .setCenter(latitude, longitude)
    .setZoom(12.0)
    .clearBearing();
```

The binding initializes `size` fields and masks internally. Java callers do not
write ABI bookkeeping fields.

Use immutable `withFoo` methods only for immutable value objects. Do not mix
mutating and immutable naming on the same type.

For most input descriptors, store Java fields and materialize the native struct
into a short-lived arena at the call boundary. Use object-owned native memory
only when the Java object represents native storage that C fills or requires for
later calls.

## Native Memory

Use arenas according to lifetime.

```text
per-call confined arena
  Temporary input structs, UTF-8 strings, out parameters, and scratch buffers.

object-owned auto arena
  Small transparent native structs whose memory may live with a Java object.

explicit native buffer
  Large reusable off-heap buffers such as image readback storage.

runtime-owned callback arena
  Upcall stubs and callback descriptors that native code may call later.
```

The default pattern for descriptors is Java-owned fields plus per-call native
materialization:

```java
try (Arena arena = Arena.ofConfined()) {
    MemorySegment nativeOptions = options.toNative(arena);
    check(nativeCall(handle, nativeOptions));
}
```

Small transparent structs may use GC-managed native memory when deterministic
freeing is unnecessary. Large buffers and resources that affect native state use
explicit ownership and `AutoCloseable`.

A Java heap array is appropriate when C only needs caller-provided storage for
the duration of a call and the data should become Java-owned immediately. An
explicit native buffer is appropriate when the caller needs a stable native
address beyond a single call boundary or wants to avoid copying large images.

## Borrowed Data

Borrowed data from C becomes copied Java data unless the Java API exposes an
explicit lexical borrow.

Runtime events are copied before `pollEvent()` returns. Event payload pointers,
messages, and strings must not escape the C event storage window.

Snapshot handles own native copied data, but values borrowed from a snapshot are
valid only while the snapshot remains live. The low-level binding may expose
snapshot-scoped views, or it may expose copy methods. Free-floating borrowed
views are not part of the public API.

Backend-native handles returned from acquired texture frames are the main
exception. They are exposed only inside a callback-scoped borrow.

## Scoped Borrows

Do not expose public acquire/release pairs for scoped borrowed resources when a
single lexical API can express the C contract.

Owned texture frames use callback-scoped access:

```java
session.withMetalOwnedTextureFrame(frame -> {
    MemorySegment texture = frame.textureUnsafe();
    MemorySegment device = frame.deviceUnsafe();
});
```

The binding calls the C acquire function before invoking the callback and calls
the matching release function in a `finally` block. The frame view is invalid
after the callback returns or throws.

The frame type itself is not publicly closeable. Its unsafe native accessors
check that the frame is still active. The parent render session tracks the
outstanding frame and rejects nested acquisition, render updates, resize,
detach, and close while the frame is active.

This mirrors the C API rule that an acquired frame blocks resize, render update,
detach, destroy, and a second acquire until release.

## Events

Expose runtime event polling as copied Java values.

```java
Optional<RuntimeEvent> event = runtime.pollEvent();
```

A drain helper may exist if it has the same semantics:

```java
runtime.drainEvents(consumer);
```

Events remain runtime-owned in the C API, but Java event objects are independent
of the next native poll. Event source handles are matched back to live Java
handles when possible; otherwise the event still carries copied source kind and
native identity metadata useful for diagnostics.

The low-level binding preserves event names and payload categories close to the
C API. Higher-level adapters may translate events into listeners, flows,
coroutines, or UI state.

## Native Callbacks

Keep callback lifetimes explicit and runtime-scoped.

Resource transforms and resource providers store Java callback objects strongly
for the lifetime required by the C API. Their upcall stubs live in an arena that
outlives all native uses.

Callbacks catch Java exceptions and convert them to the documented C callback
behavior. Exceptions do not unwind through native code.

Resource provider callbacks may run on worker or network threads. They return
quickly, avoid map and runtime calls, and copy borrowed request fields before
return when they need them later.

A handled resource request uses a Java object that owns the provider's reference
to the C request handle. It enforces one-shot completion and exactly-once
release. Completion and cancellation checks may be called from any thread when
the C API allows it.

## Render Sessions And Render Targets

`RenderSessionHandle` represents one attached render target for one map. Current
C attachment APIs make the session owner thread the map owner thread. The Java
binding records a separate session owner thread field to preserve the conceptual
boundary.

Attach methods return a `RenderSessionHandle`:

```java
try (RenderSessionHandle session = map.attachVulkanBorrowedTexture(descriptor)) {
    session.renderUpdate();
}
```

Surface descriptors and caller-owned texture descriptors contain backend-native
handles. The Java binding treats those handles as borrowed. The caller keeps
backend objects valid and synchronized for the lifetime documented by the C API.

Session-owned texture targets expose rendered backend objects only through
callback-scoped frame access. CPU readback APIs copy into Java arrays,
`ByteBuffer`, or explicit native buffers and return copied `TextureImageInfo`
metadata.

## Unsafe Escape Hatches

Backend interop sometimes requires raw native handles. Unsafe accessors are
allowed only where the C API itself exposes backend-native handles.

Name those accessors with an `Unsafe` suffix:

```java
MemorySegment textureUnsafe();
MemorySegment imageUnsafe();
MemorySegment deviceUnsafe();
```

Unsafe accessors document the exact scope in which the returned native handle is
valid. They do not transfer ownership.

## Constants And Enums

Expose C enum domains as Java enums when the domain is closed and type-safe.
Map values explicitly; do not rely on Java enum ordinal values matching C.

Expose bit masks as `EnumSet` or typed mask helpers where Java callers benefit
from validation. Preserve raw mask access only for low-level compatibility and
future C values.

Unsupported or unknown native values produce explicit exceptions for input and
stable unknown-value representations for output where forward compatibility is
needed.

## Testing

Test the Java binding as a conformance layer over the C ABI.

Cover:

- library loading and symbol resolution;
- C ABI version checks;
- status-to-exception mapping and diagnostic extraction;
- owner-thread checks for runtime, map, and render session handles;
- explicit close ordering and parent-child lifetime retention;
- cleaner leak diagnostics in debug mode;
- `size` field initialization and field-mask encoding;
- copied runtime event payloads across repeated polls;
- snapshot view/copy lifetime behavior;
- callback lifetime and exception conversion;
- resource provider pass-through, synchronous completion, asynchronous
  completion, cancellation, double completion, and release;
- texture frame callback acquisition and guaranteed release after normal return
  and exceptions;
- CPU readback buffer sizing and copying.

Also keep a generated or checked symbol/layout test so changes to public C
headers are visible to Java binding maintainers.

## Design Boundary

This layer provides safe access to the C API, not a full application framework.
It owns native handles, memory safety, diagnostics, event copying, and callback
bridging. It leaves UI lifecycle, Compose state, coroutine dispatch, render loop
policy, and application-level map abstractions to adapters above it.
