---
title: Java JNI Binding Conventions
description: Language-specific implementation conventions for Java JNI bindings.
---

Resources:

- Tracking issue:
  [#46](https://github.com/maplibre/maplibre-native-ffi/issues/46)
- [JNI specification](https://docs.oracle.com/en/java/javase/25/docs/specs/jni/)
- [Android JNI tips](https://developer.android.com/training/articles/perf-jni)
- [Rust `jni` crate](https://docs.rs/jni/)
- [Java FFM conventions](/maplibre-native-ffi/development/bindings-java-ffm/)

## Architecture

The Java JNI binding targets Android and JVMs where FFM is unavailable. Modern
JVMs with FFM support use the
[Java FFM binding](/maplibre-native-ffi/development/bindings-java-ffm/). JNI is
a bridge path: Java declares `native` methods, and a companion native library
adapts JNI values and calls the shared native binding core.

Keep the public Java API parallel to Java FFM where practical. JNI and FFM use
compatible handle names, descriptor shapes, event values, exception taxonomy,
and `NativePointer` semantics. JNI uses its own artifact and package root.
Kotlin Multiplatform `commonMain` is the shared facade for common declarations
and actual implementations.

Build the bridge library in Rust with the `jni` crate from jni-rs over the
shared Rust adaptation crates described in the
[Rust binding conventions](/maplibre-native-ffi/development/bindings-rust/). The
shared Rust adaptation layer owns C status conversion, descriptor
materialization, copied native results, and reusable callback support. The JNI
bridge Rust code owns Java callback trampolines, class lookup, method
registration, references, exceptions, thread attachment, panic containment, and
JVM or Android entry points.

Keep raw JNI details internal. `JNIEnv`, `JavaVM`, `jclass`, `jobject`, JNI
reference handles, raw `long` native pointers, and Rust `jni` crate values do
not appear in public APIs. Public APIs use Java values, records, descriptor
classes, `AutoCloseable` handles, exceptions, callbacks, buffers, and
`NativePointer`.

## Code Organization

Use a JNI-specific package root with concept grouping parallel to Java FFM. Keep
JNI-specific declarations, method metadata, reference helpers, and registration
code under internal packages and the Rust bridge crate. Generated assistance is
useful for coverage: a java-bindgen-style tool may generate Java `native`
declarations, JNI method tables, and Rust registration stubs from public C API
metadata plus binding rules. Public Java types remain curated so naming,
lifetime, and error behavior stay aligned with Java FFM.

Use explicit native-method registration from `JNI_OnLoad` so generated method
tables and cached IDs fail fast when artifacts do not match. Avoid relying on
JNI's name-mangled lookup for broad API coverage.

## Public Types and Errors

Long-lived native objects use the shared `Handle` suffix and implement
`AutoCloseable`. Descriptor classes, records, enums, events, JSON trees, and
render target types mirror Java FFM unless JNI needs a concrete memory
difference.

`NativePointer` is a borrowed opaque address value. JNI converts it to and from
`jlong` internally. The public type grants no memory access and transfers no
ownership.

Native calls translate non-OK C statuses to unchecked `MaplibreException`
subclasses carrying `MaplibreStatus`, the raw status code, and the copied
thread-local diagnostic. The bridge reads diagnostics on the same native thread
immediately after the failing C call. Rust bridge code catches panics before
returning through JNI. Pending Java exceptions are either returned to Java or
cleared before control returns to C, according to the entry point or callback
contract.

## Handles and Threading

Handle state lives in Java wrappers and Rust bridge state. A successful
`close()` releases the native handle once; later closes no-op. Failed native
destruction leaves the native handle live so the caller can retry on the correct
owner thread. Cleaners may report leaks, but they do not destroy thread-affine
native objects because the JVM runs them on arbitrary threads.

Child handles retain their Java parent wrappers while native validity depends on
that parent. `MapProjectionHandle` follows the shared exception: it owns a
standalone transform snapshot after creation and does not retain the source
`MapHandle` for native validity.

Ordinary JNI calls execute on the Java thread that invoked them. The binding
does not dispatch to another thread or introduce an async model. Native
`MLN_STATUS_WRONG_THREAD` becomes `WrongThreadException`. Host applications and
higher-level adapters choose their own Android UI-thread routing, executors, or
coroutine confinement while preserving native thread identity across related
calls.

Native threads that call into Java must attach to the JVM first. The bridge
stores the `JavaVM` from `JNI_OnLoad`, attaches MapLibre worker, network,
logging, or render-related threads before Java callbacks, and detaches only
threads that the bridge attached. It never detaches Java-created threads.

## JNI Memory and Strings

JNI local references stay scoped to one native call or callback frame. Use local
frames for loops that create many Java objects. Store Java classes, callbacks,
and long-lived helper objects as global references; use weak global references
only for explicitly non-owning caches that tolerate collection. Delete global
references exactly once when the owner-scoped native state closes.

Public Java strings are Unicode strings. The C API expects standard UTF-8. The
bridge converts Java strings to UTF-8 bytes, rejects embedded NUL for
null-terminated C inputs, and passes explicit byte lengths for string-view
fields. Avoid `GetStringUTFChars` where modified UTF-8 would change the C
contract; use Java UTF-8 byte arrays or Rust-side UTF-16 conversion.

Arrays and buffers copy at the boundary unless an API explicitly accepts a
caller-owned direct buffer for a scoped native operation. Direct buffer access
is limited to the call or documented borrow window. Native result, snapshot, and
list handles remain internal; the bridge copies their contents into Java records
or lists and releases native handles in cleanup paths.

## Callbacks

Callback lifetimes follow the owner scope defined by the C API. The bridge
stores Java callback objects in global references held by Rust state. Callback
trampolines copy borrowed request fields and event data before Java can retain
them, invoke Java through an attached `JNIEnv`, and convert thrown Java
exceptions to the C callback's documented result. Exceptions never cross C or
Rust frames as unwinding.

Resource provider callbacks copy requests into Java `ResourceRequest` values.
`ResourceRequestHandle` owns the native request reference while Java handles it,
enforces one-shot completion, and releases the reference exactly once.
Completion may occur during the callback or later from another thread when the C
API allows it. When replacing a callback, install the new native descriptor
before releasing old Java or Rust state; if installation fails, release the
replacement and keep the previous callback active. Custom geometry callbacks are
map/style scoped, track active upcalls, and delay global-reference release until
in-flight calls return.

## Render Targets

Render APIs mirror Java FFM. Surface and texture descriptors use `NativePointer`
for host-owned backend objects. Public ownership stays explicit and borrowed.

`RenderSessionHandle` represents one attached target for one map and keeps the
map wrapper alive. Session-owned texture targets expose explicit frame handles.
Frame accessors return copied metadata and scoped `NativePointer` values, reject
use after close, and require callers to close frames before resize, render
update, detach, or session destruction.
