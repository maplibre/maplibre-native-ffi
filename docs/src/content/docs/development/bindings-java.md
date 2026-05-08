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

Higher-level Java and Kotlin adapters build on this layer. For example, a
Compose adapter can own recomposition, coroutines, lifecycle, image
presentation, and application-level map objects while delegating native calls to
this binding.

The binding uses the Java Foreign Function & Memory API. It targets the final
FFM API available in modern JDKs.

## Package And API Shape

Packages provide project context. Class names describe the C concept they wrap.

Owned native resources use a `Handle` suffix:

```text
org.maplibre.ffi.RuntimeHandle
org.maplibre.ffi.MapHandle
org.maplibre.ffi.RenderSessionHandle
org.maplibre.ffi.JsonSnapshotHandle
org.maplibre.ffi.OfflineRegionSnapshotHandle
org.maplibre.ffi.OfflineRegionListHandle
```

`Handle` means the object wraps a native resource and closes it. Java-owned
values, descriptors, events, and copied data omit the suffix:

```text
CameraOptions
MapOptions
AnimationOptions
TextureImageInfo
ResourceRequest
ResourceResponse
RuntimeEvent
```

Keep public Java names close to the C concepts. Rename only where Java
readability or namespace clarity benefits.

## Binding Layers

Separate FFM machinery from the safe public binding.

```text
org.maplibre.ffi.internal.c
  FFM symbol lookup, layouts, downcall handles, upcall stubs, and C constants.

org.maplibre.ffi.internal
  Status conversion, diagnostics, handle state, owner-thread checks, and arenas.

org.maplibre.ffi
  Safe low-level Java API.
```

The public API exposes `MemorySegment` only for backend-native interop that
already crosses the C API as opaque native handles. Keep `Arena`, `MethodHandle`,
and C layout classes internal.

Use `jextract` to validate that public headers remain bindable. The shipped API
may use curated hand-written FFM stubs for clearer names, version checks,
diagnostics, and lifetime control.

## Java Version

Target JDK 22 or newer. Earlier JDKs used preview or incubator FFM APIs that
produce a different binding surface.

Older JVM support belongs in a separate artifact if the project later needs it.

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

Java prechecks may catch open state, owner thread, nulls, and obvious argument
ranges for clearer failures. The C API remains authoritative.

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

- the native pointer;
- the owner thread recorded at creation;
- parent handles needed for native validity;
- open or closed state;
- optional debug leak diagnostics.

`close()` calls the matching C destroy function on the required owner thread.
Closing a handle from the wrong thread throws before calling C. A successful
close makes later close calls no-ops.

Parent handles stay reachable while child handles are live. `MapHandle` keeps
its `RuntimeHandle` reachable. `RenderSessionHandle` keeps its `MapHandle`
reachable.

A `Cleaner` may report leaked handles in debug builds. It does not destroy
thread-affine native handles, because cleaner threads are not C API owner
threads. Managed adapters may enqueue cleanup to their owner thread.

## Owner Threads

Mirror the C API's owner-thread model.

Runtime creation records the runtime owner thread. Map creation currently runs
on the runtime owner thread and makes that same thread the map owner thread.
Surface and texture attachment currently create render sessions whose session
owner thread is the map owner thread.

Store the owner thread on each handle independently:

```text
RuntimeHandle       runtime owner thread
MapHandle           map owner thread
RenderSessionHandle session owner thread
```

This keeps the Java object model aligned with the C concepts and leaves room for
a future C API that gives render sessions a distinct render-thread owner.

Thread-affine methods check the relevant handle owner. Resource provider request
completion follows the C API and may run from any thread.

## Options And Transparent Structs

Model C option structs as Java-owned descriptor objects. Mutating descriptor
methods use `setFoo`, return `this`, and update any corresponding field mask.

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

Immutable value objects use `withFoo` methods. A type uses either mutating
`setFoo` methods or immutable `withFoo` methods, not both.

Most input descriptors store Java fields and materialize native structs into a
short-lived arena at the call boundary. Use object-owned native memory when the
Java object represents storage that C fills or later consumes.

## Native Memory

Use arenas according to lifetime.

```text
per-call confined arena
  Temporary input structs, UTF-8 strings, out parameters, and scratch buffers.

object-owned auto arena
  Small transparent native structs whose memory lives with a Java object.

explicit native buffer
  Large reusable off-heap buffers such as image readback storage.

runtime-owned callback arena
  Upcall stubs and callback descriptors that native code may call later.
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

## Borrowed Data

Borrowed data from C becomes copied Java data unless the Java API exposes an
explicit lexical borrow.

`pollEvent()` copies runtime events before it returns. Event payload pointers,
messages, and strings never escape their C event storage window.

Snapshot handles own native copied data. Values borrowed from a snapshot remain
valid while the snapshot handle is live. The Java API may expose
snapshot-scoped views or copy methods. It does not expose free-floating borrowed
views.

Backend-native handles returned from acquired texture frames are lexical
borrows.

## Scoped Borrows

Expose scoped borrowed resources through one lexical API.

Owned texture frames use callback-scoped access:

```java
session.withMetalOwnedTextureFrame(frame -> {
    MemorySegment texture = frame.textureUnsafe();
    MemorySegment device = frame.deviceUnsafe();
});
```

The binding calls the C acquire function before invoking the callback and calls
the matching release function in a `finally` block. The frame view is valid only
during the callback.

The frame type is not publicly closeable. Its unsafe native accessors check that
the frame is active. The parent render session tracks the outstanding frame and
rejects nested acquisition, render updates, resize, detach, and close while the
frame is active.

This mirrors the C API rule that an acquired frame blocks resize, render update,
detach, destroy, and a second acquire until release.

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
the next native poll. Event source handles map back to live Java handles when
possible. Otherwise, the event carries copied source kind and native identity
metadata for diagnostics.

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
return when they need those fields later.

A handled resource request uses a Java object that owns the provider's reference
to the C request handle. It enforces one-shot completion and exactly-once
release. Completion and cancellation checks may run from any thread when the C
API allows it.

## Render Sessions And Render Targets

`RenderSessionHandle` represents one attached render target for one map. Current
C attachment APIs make the session owner thread the map owner thread. The Java
binding still records a separate session owner thread field.

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

Backend interop sometimes requires raw native handles. Unsafe accessors are
allowed where the C API itself exposes backend-native handles.

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

Expose bit masks as `EnumSet` or typed mask helpers when Java callers benefit
from validation. Preserve raw mask access only for low-level compatibility and
future C values.

Unsupported or unknown native input values throw explicit exceptions. Output
values that may grow across C API versions use stable unknown-value
representations where forward compatibility matters.

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
- snapshot view and copy lifetime behavior;
- callback lifetime and exception conversion;
- resource provider pass-through, synchronous completion, asynchronous
  completion, cancellation, double completion, and release;
- texture frame callback acquisition and guaranteed release after normal return
  and exceptions;
- CPU readback buffer sizing and copying.

Keep a generated or checked symbol/layout test so changes to public C headers
are visible to Java binding maintainers.

## Design Boundary

This layer owns native handle lifetimes, memory safety, diagnostics, event
copying, and callback bridging. Adapters above it own UI lifecycle, Compose
state, coroutine dispatch, render loop policy, and application-level map
abstractions.
