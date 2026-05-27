---
title: Java JNI Binding Conventions
description: Language-specific implementation conventions for Java JNI bindings.
---

Resources:

- Tracking issue:
  [#46](https://github.com/maplibre/maplibre-native-ffi/issues/46)
- [JavaCPP](https://github.com/bytedeco/javacpp)
- [JNI specification](https://docs.oracle.com/en/java/javase/25/docs/specs/jni/)
- [Android JNI tips](https://developer.android.com/training/articles/perf-jni)
- [Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/)

## Architecture

The Java JNI binding targets Android and JVMs where FFM is unavailable. Modern
JVMs with FFM support use the
[Java FFM binding](/maplibre-native-ffi/development/bindings-java-ffm/). JNI is
a bridge path: generated JavaCPP internals call the public MapLibre Native C
ABI, and curated Java wrappers present the public binding API.

Keep the public Java API parallel to Java FFM where practical. JNI and FFM use
compatible handle names, descriptor shapes, event values, exception taxonomy,
and `NativePointer` semantics. JNI uses its own artifact and package root.
Kotlin Multiplatform `commonMain` can provide the shared facade for common
declarations and actual implementations.

Keep generated JavaCPP details internal. `org.bytedeco.javacpp.Pointer`, JavaCPP
generated structs, generated C entry-point declarations, implementation handles,
and raw `long` native pointers stay inside internal packages. Public APIs use
Java values, records, descriptor classes, `AutoCloseable` handles, exceptions,
callbacks, buffers, and `NativePointer`.

## Code Organization

Use a JNI-specific package root with concept grouping parallel to Java FFM. The
main layers are:

```text
internal javacpp layer
  Generated JavaCPP declarations and presets for the public C headers.

internal support layer
  Small handwritten adapters that materialize JavaCPP structs, copy snapshots,
  translate statuses and diagnostics, load native libraries, and manage
  callback lifetimes. Public handles call generated JavaCPP C declarations
  directly and use these adapters only for scoped values, copied results, and
  callback ownership.

public binding layer
  Low-level Java API with stable names, ownership rules, diagnostics, and
  lifetime control.
```

Generate broad C/JNI coverage from the public C headers with JavaCPP. Handwrite
only the adapters that preserve Java API shape, value copying, ownership, and
error behavior.

## Public Types and Errors

Long-lived native objects use the shared `Handle` suffix and implement
`AutoCloseable`. Descriptor classes, records, enums, events, JSON trees, and
render target types mirror Java FFM unless JNI needs a concrete memory
difference.

`NativePointer` is a borrowed opaque address value. JNI converts it to and from
native pointer values internally. The public type grants no memory access and
transfers no ownership.

Native calls translate non-OK C statuses to unchecked `MaplibreException`
subclasses carrying `MaplibreStatus`, the raw status code, and the copied
thread-local diagnostic. The JNI support layer reads diagnostics on the same
native thread immediately after the failing C call. C++ exceptions and panics
are contained by the C ABI. Java callbacks catch Java exceptions and errors
before returning through a JavaCPP callback path.

## Handles and Threading

Handle state lives in Java wrappers. A successful `close()` releases the native
handle once; later closes no-op. Failed native destruction leaves the native
handle live so the caller can retry on the correct owner thread. Cleaners may
report leaks, but they do not destroy thread-affine native objects because the
JVM runs them on arbitrary threads.

Child handles retain their Java parent wrappers while native validity depends on
that parent. `MapProjectionHandle` follows the shared exception: it owns a
standalone transform snapshot after creation and does not retain the source
`MapHandle` for native validity.

Ordinary JNI calls execute on the Java thread that invoked them. The binding
does not dispatch to another thread or introduce an async model. Native
`MLN_STATUS_WRONG_THREAD` becomes `WrongThreadException`. Host applications and
higher-level adapters choose Android UI-thread routing, executors, or coroutine
confinement while preserving native thread identity across related calls.

JavaCPP attaches native-created callback threads before invoking Java callback
objects. Binding-owned callback state remains live for the C owner scope and is
released when the owner clears, replaces, or destroys the callback.

## JNI Memory and Strings

JavaCPP generated code owns the JNI frame mechanics for direct C calls. The
handwritten adapters keep native snapshots, result handles, and list handles
internal: they copy contents into Java records or lists and release native
handles in cleanup paths.

Public Java strings are Unicode strings. The C API expects standard UTF-8. JNI
support converts Java strings to UTF-8 bytes, rejects embedded NUL for
null-terminated C inputs, and passes explicit byte lengths for string-view
fields. Avoid modified UTF-8 for C API text.

Arrays and buffers copy at the boundary unless an API explicitly accepts a
caller-owned direct buffer for a scoped native operation. Direct buffer access
is limited to the call or documented borrow window.

## Callbacks

Callback lifetimes follow the owner scope defined by the C API. Java callback
objects are held strongly by Java state for process-global logging callbacks,
runtime-scoped resource callbacks, and map/style-scoped custom geometry
callbacks. Replacing a callback installs the new native descriptor before
releasing old state when the C API requires the previous callback to remain
active on failure.

Callback trampolines copy borrowed request fields and event data before Java can
retain them, invoke Java through JavaCPP's callback machinery, and convert
thrown Java exceptions or errors to the C callback's documented result. Java
exceptions never unwind through C frames.

Resource provider callbacks copy requests into Java `ResourceRequest` values.
`ResourceRequestHandle` owns the native request reference while Java handles it,
enforces one-shot completion, and releases the reference exactly once.
Completion may occur during the callback or later from another thread when the C
API allows it. Custom geometry callbacks are map/style scoped, catch callback
failures, and keep callback state live for in-flight calls.

## Render Targets

Render APIs mirror Java FFM. Surface and texture descriptors use `NativePointer`
for host-owned backend objects. Public ownership stays explicit and borrowed.

`RenderSessionHandle` represents one attached target for one map and keeps the
map wrapper alive. Session-owned texture targets expose explicit frame handles.
Frame accessors return copied metadata and scoped `NativePointer` values, reject
use after close, and require callers to close frames before resize, render
update, detach, or session destruction.
