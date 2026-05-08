---
title: Java JNI Bindings
description: Design rules for safe low-level Java JNI bindings.
---

## Scope

The Java JNI binding is a safe low-level binding over the public C API. It
exposes MapLibre Native functionality with Java ownership, error, memory, and
thread-safety conventions.

Higher-level Java and Kotlin adapters like MapLibre Compose and Android UI
integrations should be able to build on this layer. Such integrations will own
UI lifecycle, dispatch, rendering policy, and application-level map objects
while delegating native calls to this binding.

The binding uses the
[Java Native Interface](https://docs.oracle.com/en/java/javase/25/docs/specs/jni/index.html).
It targets Android and other JVMs where the
[Java Foreign Function & Memory API](https://docs.oracle.com/en/java/javase/25/core/foreign-function-and-memory-api.html)
is unavailable. Modern JVMs with FFM support are covered by the separate
[Java FFM binding](/maplibre-native-ffi/development/bindings-java/).

Keep the public API shape aligned with the Java FFM binding where the underlying
C API semantics match. API parity is required for future Kotlin Multiplatform
commonization.

## Package And API Shape

Owned long-lived native objects use a `Handle` suffix:

```text
org.maplibre.nativekit.jni.RuntimeHandle
org.maplibre.nativekit.jni.MapHandle
org.maplibre.nativekit.jni.RenderSessionHandle
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
```

Keep public Java names close to the C concepts. Rename where Java readability or
namespace clarity benefits.

## Binding Layers

Separate JNI access from the safe public binding.

```text
org.maplibre.nativekit.jni.internal.c
  Generated native-method declarations and JNI bridge
  entry points that call the public C API.

org.maplibre.nativekit.jni.internal
  Status conversion, diagnostics, handle state, native library loading, direct
  buffers, and callback bridging.

org.maplibre.nativekit.jni
  Safe low-level Java API.
```

Generate the internal C layer from the public C headers. Evaluate JavaCPP and
SWIG for this layer. Treat successful generation and compilation as the header
bindability check for this path. The public Java layer is handwritten and wraps
the generated layer with stable names, ownership rules, diagnostics, and
lifetime control.

Keep JNI types internal. Public APIs do not expose `JNIEnv`, `jobject`, JNI
reference handles, generated C layout classes, or raw `long` pointers.
Backend-native handles that cross the public API use a small opaque
`NativePointer` value type. The JNI layer converts that value to the pointer
representation required by the C API.

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
immediately on the same native thread and include it in the exception. Another C
call on that thread may replace the diagnostic.

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

- the native pointer as private implementation state;
- parent handles needed for native validity;
- open or closed state;
- optional debug leak context.

`close()` calls the matching C destroy function through JNI. A successful close
makes later close calls no-ops. If the C API reports `MLN_STATUS_WRONG_THREAD`,
the binding throws `WrongThreadException` with the native diagnostic.

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

This leaves room for
[a future C API](https://github.com/maplibre/maplibre-native-ffi/issues/121)
that exposes render sessions owned by a render thread distinct from the runtime
owner thread. If Java needs to inspect owner threads directly, add a C getter.

Resource provider request completion follows the C API and may run from any
thread. JNI callback code attaches native threads to the JVM before invoking
Java code and detaches threads that the binding attached.

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

Most input descriptors store Java fields and materialize native structs in the
JNI bridge at the call boundary. Use object-owned native memory when the Java
object represents storage that C fills or later consumes.

## Native Memory

Use native storage according to lifetime and purpose.

```text
per-call native stack or scoped allocation
  MapOptions, CameraOptions, FeatureStateSelector, temporary UTF-8 strings,
  out parameters for create functions, and scratch buffers.

object-owned native allocation
  Small transparent native structs whose native storage is part of a Java
  object, such as a reusable TextureImageInfo native view when direct struct
  reuse is useful.

explicit native buffer
  Large caller-owned storage, such as CPU texture readback buffers reused across
  frames.

runtime-owned callback state
  JNI global references, callback adapters, and C callback registrations owned
  by a runtime.
```

The default descriptor pattern is Java-owned fields plus per-call native
materialization inside the JNI bridge:

```java
private static native void nativeSetCamera(long mapHandle, CameraOptions camera);
```

The native implementation copies Java fields into C structs, calls the public C
API, reads any output values before returning, and releases JNI local references
and temporary UTF-8 strings before the native frame exits.

Small transparent structs may use Java heap fields when they do not need a
stable native address. Large buffers and native-state resources use explicit
ownership and `AutoCloseable`.

A Java heap array works well when C writes caller-provided storage during one
call and Java owns the result. A direct `ByteBuffer` or explicit native buffer
works well when the caller needs a stable native address or wants to reuse large
off-heap storage.

## Native Pointers

`NativePointer` represents an opaque backend-native address that the Java
binding does not own. It is a value object, not a JNI reference or a memory
view.

Use `NativePointer` for C `void*` fields that represent host-owned backend
objects, such as Metal devices, Metal textures, Vulkan devices, Vulkan queues,
Vulkan images, Android native windows, hardware buffers, and native surfaces.
Passing a `NativePointer` transfers no ownership and grants no memory access.

Public APIs accept or return `NativePointer` only where the C API already
accepts or returns an opaque backend-native handle. The binding converts between
`NativePointer` and internal JNI pointer values at the native-method boundary.

## Callback-Scoped Borrows

A callback-scoped borrow is native data exposed only during a Java callback. The
binding acquires the native borrow before invoking the callback and releases it
in a `finally` block after the callback returns or throws.

Owned texture frames use callback-scoped access:

```java
session.withMetalOwnedTextureFrame(frame -> {
    NativePointer texture = frame.textureUnsafe();
    NativePointer device = frame.deviceUnsafe();
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

Backend-native handles returned from acquired texture frames are callback-scoped
borrows represented as `NativePointer` values.

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

`RuntimeHandle` keeps a registry of live `MapHandle` wrappers keyed by native
map pointer. When a map-originated event contains a source pointer, the binding
can attach the matching `MapHandle` to the copied Java event. If no wrapper is
live, the event carries only copied source kind and native identity metadata for
diagnostics.

The low-level binding preserves event names and payload categories close to the
C API. Translating events into listeners, flows, coroutines, or UI state belongs
to adapters above this layer.

## Native Callbacks

Keep callback lifetimes explicit and runtime-scoped.

A Java callback is a Java interface implementation stored by the binding. The
JNI bridge stores a global reference for each Java callback that native code may
invoke after the registering native method returns. Native code calls a C
callback adapter. The adapter obtains a `JNIEnv` for the current thread, copies
or wraps callback arguments according to this document, invokes the Java
interface method, and converts the result back to C data.

Resource transforms and resource providers store Java callback implementations
strongly for the lifetime required by the C API. Their JNI global references and
native adapter state live in runtime-owned callback state that outlives all
native uses.

Callbacks catch Java exceptions and convert them to the documented C callback
behavior. Exceptions do not unwind through native code. JNI code clears or
reports pending Java exceptions before returning to C according to the callback
contract.

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
NativePointer textureUnsafe();
NativePointer imageUnsafe();
NativePointer deviceUnsafe();
```

Unsafe accessors document the scope in which the returned native handle is
valid. They do not transfer ownership or expose JNI references or native memory
access.

## Constants And Enums

Expose C enum domains as Java enums when the domain is closed and type-safe. Map
values explicitly. Java enum ordinals are not ABI values.

Expose C bit masks as `EnumSet` values in the public Java API. The internal JNI
layer keeps raw integer constants internal.

Output values that may grow across C API versions use stable unknown-value
representations where forward compatibility matters.

## Testing

Test the Java adaptation layer rather than duplicating the full C ABI test
suite. C ABI tests prove native behavior. Java tests prove that generated or
generated-assisted JNI bindings, handwritten wrappers, Java ownership, copying,
callbacks, and exceptions preserve that behavior at the JVM boundary.

Prefer small adaptation tests around real C calls. When a C ABI test already
covers native validation, the Java test only needs to show that the
corresponding Java method propagates the native status, diagnostic, or copied
output correctly.

Add regression tests when the Java layer owns a lifetime or threading invariant
that the JNI layer cannot express, such as releasing a texture frame after a
throwing callback or preserving parent handles while child handles are
reachable.
