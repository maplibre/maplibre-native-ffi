# Java JNI Binding Specification

## Audience and category

This specification is for contributors implementing and reviewing the Java JNI
binding. It is reference material: it describes the target machinery,
invariants, layout, and completion criteria for the implementation.

## Scope

The Java JNI binding exposes the MapLibre Native FFI C API to Java runtimes
where Java FFM is unavailable or unsuitable, including Android. The binding
preserves the same runtime, map, render session, event, resource, callback,
status, and handle model as the C API and the Java FFM binding.

The JNI binding consists of two artifacts:

- a Java artifact with public Java types, `native` declarations, loader code,
  handle wrappers, descriptors, events, and tests;
- a Rust `cdylib` artifact that implements the JNI bridge with `jni-rs` and
  delegates shared C ABI adaptation to `maplibre-native-core`.

The public Java API uses the package root `org.maplibre.nativejni` and the Java
module `org.maplibre.nativejni`. Public concept packages mirror Java FFM:

```text
org.maplibre.nativejni
org.maplibre.nativejni.camera
org.maplibre.nativejni.error
org.maplibre.nativejni.geo
org.maplibre.nativejni.json
org.maplibre.nativejni.log
org.maplibre.nativejni.map
org.maplibre.nativejni.offline
org.maplibre.nativejni.query
org.maplibre.nativejni.render
org.maplibre.nativejni.resource
org.maplibre.nativejni.runtime
org.maplibre.nativejni.style
```

Internal JNI, loader, status, lifecycle, callback, and descriptor code remains
under `org.maplibre.nativejni.internal.*` and is not exported from the module.

## Current scaffold

```text
bindings/java-jni/
  SPEC.md
  build.gradle.kts
  mise.toml
  native/
    Cargo.toml
    src/lib.rs
  src/main/java/module-info.java
  src/main/java/org/maplibre/nativejni/Maplibre.java
  src/main/java/org/maplibre/nativejni/error/*.java
  src/main/java/org/maplibre/nativejni/render/NativePointer.java
  src/main/java/org/maplibre/nativejni/internal/bridge/NativeBridge.java
  src/main/java/org/maplibre/nativejni/internal/loader/NativeLibrary.java
```

The scaffold includes a first native-method slice: `Maplibre.cVersion()` loads
`maplibre-native-jni`, registers `NativeBridge.cVersion()` from `JNI_OnLoad`,
and calls `mln_c_version()` through the Rust bridge.

## Build and packaging

The Gradle project `:bindings:java-jni` builds the Java artifact. The Cargo
package `maplibre-native-jni` builds the JNI bridge library.

Required task behavior:

- `mise run //bindings/java-jni:build` builds Java sources and runs Java checks.
- `./gradlew :bindings:java-jni:javadoc` validates public Javadocs.
- `mise run //bindings/java-jni:native:build` builds the Rust JNI bridge after
  the C library exists.
- Release packaging includes the Java jar, sources jar, Javadocs jar, and one or
  more platform JNI bridge libraries.
- Android packaging produces AAR-compatible native library layout under
  `jni/<abi>/` or the Gradle/Android plugin equivalent.
- JVM packaging supports classifiers or documented native-library installation
  for macOS, Linux, and Windows host triples that the C API supports.

Native library lookup order:

1. exact JNI bridge file path from `org.maplibre.nativejni.library.path`;
2. exact JNI bridge file path from `MAPLIBRE_NATIVE_JNI_LIBRARY_PATH`;
3. `System.loadLibrary("maplibre-native-jni")` through the runtime library path.

The JNI bridge is responsible for locating or linking `maplibre-native-c` in the
same distribution. A failed load surfaces as the ordinary JVM load error with
the configured path or library name visible.

## JNI bridge rules

The Rust bridge owns all raw JNI details:

- `JNI_OnLoad` stores the `JavaVM` and registers native methods explicitly.
- Registration uses stable generated method tables. Broad API coverage does not
  rely on JNI name-mangled lookup.
- Cached classes, method IDs, field IDs, and constructors are validated during
  load or during the owning feature's first registration step.
- Bridge code catches Rust panics before returning through JNI.
- Bridge code either leaves a pending Java exception for Java callers or clears
  it before returning to C according to the native callback contract.
- MapLibre-created native threads attach to the JVM before Java callbacks and
  detach only when the bridge performed the attachment.
- Java-created threads are never detached by the bridge.

The bridge may call `maplibre-native-sys` directly only for JNI trampoline glue
or when `maplibre-native-core` lacks a needed adapter. Repeated direct `sys`
sequences move into `maplibre-native-core`.

## Public Java API rules

Public names stay parallel to Java FFM where practical. Long-lived native
objects use the shared `Handle` suffix and implement `AutoCloseable`:

```text
RuntimeHandle
MapHandle
MapProjectionHandle
RenderSessionHandle
OfflineOperationHandle
ResourceRequestHandle
```

Descriptor classes, records, enums, events, JSON trees, and render target types
mirror Java FFM unless JNI requires a concrete representation difference.

Kotlin Multiplatform shared declarations may wrap this Java JNI artifact as a
JVM or Android actual implementation. The Java JNI public surface therefore
stays alignable with Java FFM and Kotlin common names, copied values, exception
semantics, and ownership rules. Coroutine, Android UI-thread, and higher-level
scheduler policy remain above this low-level binding.

Public APIs do not expose `JNIEnv`, `JavaVM`, `jclass`, `jobject`, raw JNI
references, Rust `jni` crate types, generated method tables, or raw `long`
handles. Raw native addresses appear only as `NativePointer`, a borrowed opaque
value that grants no memory access and transfers no ownership.

## Handles and lifecycle

Each public handle stores or references private state containing:

- the native pointer;
- live or closed state;
- the parent wrapper required for native validity;
- callback state owned by the handle;
- optional leak context.

`close()` releases the native handle once. Later `close()` calls no-op. If a
native destroy function returns a non-OK status, `close()` throws the mapped
exception and leaves the handle live when retrying is valid.

Cleaners report leaks and release Java-side state. They do not destroy
thread-affine native handles because cleaners may run on arbitrary JVM threads.

Child handles retain parents while native validity depends on the parent.
`MapProjectionHandle` is the shared exception: after creation it owns a
standalone projection snapshot and does not retain the source `MapHandle` for
native validity.

Owner-thread-affine calls run on the Java thread that invoked the method. The
binding does not dispatch to another thread, executor, Android looper, or
coroutine context. Native `MLN_STATUS_WRONG_THREAD` becomes
`WrongThreadException` with the copied native diagnostic.

## Status, diagnostics, and exceptions

Status-returning native calls map to unchecked `MaplibreException` subclasses:

```text
MLN_STATUS_INVALID_ARGUMENT -> InvalidArgumentException
MLN_STATUS_INVALID_STATE    -> InvalidStateException
MLN_STATUS_WRONG_THREAD     -> WrongThreadException
MLN_STATUS_UNSUPPORTED      -> UnsupportedFeatureException
MLN_STATUS_NATIVE_ERROR     -> NativeErrorException
unknown status              -> MaplibreException with UNKNOWN status
```

Every exception carries:

- `MaplibreStatus status()`;
- `int nativeStatusCode()`;
- `String diagnostic()` copied immediately on the same native thread after the
  failing C call.

The bridge validates Java-owned state before crossing into C: closed wrappers,
active frame scopes, callback-scope borrows, one-shot request completion,
embedded NUL in null-terminated strings, descriptor depth, and buffer shape. The
C API validates native handles, native lifecycle state, thread affinity, enum
values, dimensions, masks, and MapLibre-specific rules.

## Type mapping

- C option structs become mutable Java descriptor classes. Fluent setters mark
  field presence; clearers remove field presence; accessors report values and
  presence.
- C `size` fields, masks, native defaults, temporary arrays, and string views
  are internal materialization details.
- Copied values are Java records where stable value equality is useful.
- Closed C enum domains map to Java enums with explicit native-value conversion.
- Growable output domains include `UNKNOWN` or preserve the raw native value.
- C bit masks become `EnumSet<T>` or purpose-built public mask values.
- JSON and GeoJSON use Java-owned value trees that preserve integer width,
  member order, and duplicate keys.
- Native result, snapshot, and list handles stay internal and release in cleanup
  paths after copying.
- Public strings are converted to standard UTF-8. Null-terminated C inputs
  reject embedded NUL. Explicit-length C string views use byte length.
- Arrays and buffers copy at the boundary unless an API explicitly accepts a
  direct buffer for a documented scoped native operation.

## Callbacks

Callback state is scoped to the C API owner that can invoke it:

- logging callbacks are process-global;
- resource transforms and providers are runtime-scoped;
- custom geometry source callbacks are map/style-scoped;
- handled resource requests own one native request reference.

The Rust bridge stores Java callback objects as global references. It deletes
global references exactly once after native code can no longer invoke them.
Local references stay within one native call or callback frame; loops use local
frames.

Callbacks may arrive on MapLibre worker, network, logging, or render-related
threads. Trampolines attach native threads to the JVM, copy borrowed C fields
before Java can retain them, invoke Java, catch Java exceptions, and convert
failures to the C callback's documented result. Exceptions never unwind through
C or Rust frames.

Resource provider callbacks create copied Java `ResourceRequest` values.
`ResourceRequestHandle` enforces one-shot completion and exactly-once release.
Completion may occur during the callback or later from another thread when the C
API permits it.

Resource transform callbacks keep any C-borrowed response storage alive until
native code has consumed it. Per-thread response scratch storage closes on the
next callback for that thread and during runtime teardown.

Custom geometry source callbacks track active upcalls. Replacing or removing a
custom geometry source prevents new upcalls, then delays global-reference and
native-state release until in-flight callbacks have returned. Java callbacks
that need map methods hand work back to the map owner thread before calling
thread-affine APIs.

When replacing callback state, install the replacement native descriptor first.
If installation fails, release the replacement and keep the previous callback
active.

## Runtime, events, and resources

`RuntimeHandle` owns a native runtime on the creating Java thread. `runOnce()`
pumps native work on that owner thread. Event polling returns copied Java event
objects independent of the next native poll.

Runtime event objects preserve source type, source handle identity when a live
wrapper is available, copied source metadata when useful for diagnostics, event
type, and payload. Unknown future payloads map to an unknown payload type that
preserves raw discriminants and copied diagnostic fields where available.

Resource transform callbacks copy request URLs before invoking Java. Provider
routing performs native pass-through for non-matching requests before crossing
into Java when the C API exposes routing data.

## Rendering

`RenderSessionHandle` represents one attached target for one map and keeps the
map wrapper alive. Surface and texture descriptors use `NativePointer` for
host-owned backend objects. Callers keep backend objects valid and synchronized
for the C API's documented lifetime.

Texture readback supports:

- caller-owned direct buffers for reusable storage;
- copied Java byte arrays or image records for convenience.

Session-owned texture targets expose explicit frame handles. Frame handles are
`AutoCloseable`, close on the render-session owner thread, and invalidate scoped
metadata and backend pointers after close. The binding rejects resize, another
render update, detach, or session destruction while a frame is live.

## Android requirements

Android support uses the same public Java package unless a future Android
artifact adds Android-specific adapters above this low-level layer. The JNI
bridge follows Android JNI guidance:

- cache class and method lookups after load or first use;
- use global references for long-lived callback objects;
- keep local references scoped;
- avoid modified UTF-8 for C API strings by converting through standard UTF-8
  bytes or Rust UTF-16 conversion;
- attach native threads before Java callbacks;
- keep callback work short and hand application scheduling to adapters above
  this binding.

## Generation plan

Generated assistance should cover breadth while curated public types preserve
the Java API contract.

Inputs:

- `include/maplibre_native_c.h` and domain headers;
- C API comments and status lists;
- binding conventions in `docs/src/content/docs/development/`;
- Java FFM public source shape.

Generated outputs:

- Java `native` declarations under `internal.bridge`;
- Rust JNI method tables and registration stubs;
- Rust-to-Java constructor and enum conversion helpers;
- coverage reports listing public C API functions, generated declarations, and
  implemented public wrappers.

Generated coverage becomes a CI check before the binding is considered feature
complete. The report distinguishes unsupported C API features, intentionally
internal bridge helpers, generated-but-unwrapped native methods, and completed
public wrappers.

Curated outputs:

- public Java classes, records, enums, descriptors, callbacks, and handles;
- exception taxonomy;
- lifecycle and callback state machines;
- tests and examples.

## Test requirements

JNI tests run against the real C library and JNI bridge. They focus on language
adaptation invariants:

- library loading and version query;
- status-to-exception mapping and diagnostic copying;
- close idempotence and failed-close retry behavior;
- parent retention while child handles are live;
- wrong-thread propagation;
- string validation and UTF-8 conversion;
- copied values, events, snapshots, and list results;
- direct-buffer validation and readback behavior;
- callback global-reference lifetime;
- Java exception containment inside callbacks;
- resource request one-shot completion;
- frame handle invalidation;
- resource transform response storage lifetime;
- custom geometry active-upcall teardown;
- generated coverage report completeness;
- Android class-loader and thread-attach behavior where Android tests run.

C ABI tests remain the source of truth for native behavior. JNI tests prove that
Java wrappers preserve native behavior at the Java boundary.

## Implementation milestones

1. Complete the thin bridge slice: load library, register methods, return ABI
   version, get/set network status, and map statuses to exceptions.
2. Port Java FFM public value types, enums, descriptors, and `NativePointer` to
   the JNI package root.
3. Add handle state and implement runtime/map/projection lifecycle.
4. Implement event polling and copied event payloads.
5. Implement style, camera, query, and offline APIs.
6. Implement resource transforms and resource providers with one-shot request
   completion.
7. Implement render sessions, surface and texture descriptors, readback, and
   owned texture frame handles.
8. Add generated coverage for native declarations and Rust registration tables.
9. Add JVM packaging, Android packaging, CI tasks, and documentation links.
10. Reach Java FFM API parity for supported C API features and record any
    JNI-only platform constraints in this spec.

## Completion criteria

The JNI binding is complete when:

- every supported C API feature exposed by Java FFM has an equivalent JNI public
  API or a documented platform reason for omission;
- public JNI names, descriptors, events, exceptions, and `NativePointer`
  semantics stay compatible with Java FFM;
- all native methods register explicitly from `JNI_OnLoad`;
- raw JNI and Rust bridge details remain internal;
- callback and handle lifetimes have tests for exactly-once release;
- wrong-thread, invalid-state, invalid-argument, unsupported, and native-error
  statuses map to the documented exceptions with copied diagnostics;
- JVM and Android artifacts can load the bridge and execute the test suite on
  supported platforms.
