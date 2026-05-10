# Java FFM Binding Implementation Plan

## Resources

- [Java FFM binding conventions](../../docs/src/content/docs/development/bindings-java-ffm.md)
- [Shared binding conventions](../../docs/src/content/docs/development/bindings.md)
- [Project concepts](../../docs/src/content/docs/concepts.md)
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md)
- [Development overview](../../docs/src/content/docs/development/overview.md)
- [Java JNI binding conventions](../../docs/src/content/docs/development/bindings-java-jni.md)
- [Public C headers](../../include/)
- [Umbrella C header](../../include/maplibre_native_c.h)
- [Tracking issue #45](https://github.com/maplibre/maplibre-native-ffi/issues/45)
- [Java Foreign Function & Memory API](https://docs.oracle.com/en/java/javase/25/core/foreign-function-and-memory-api.html)
- [`jextract`](https://jdk.java.net/jextract/)

## Completion Target

Deliver a low-level Java package for modern desktop/server JVMs that:

- calls the public C ABI through the generated `jextract` layer;
- keeps FFM classes, generated layouts, arenas, method handles, and memory
  segments internal;
- exposes Java handle types, value objects, descriptors, callbacks, events, and
  exceptions that preserve the C API model;
- supports deterministic release for every long-lived native handle;
- translates C statuses and thread-local diagnostics into Java exceptions;
- includes a smoke example and binding tests that exercise real C ABI calls.

The binding targets the Java FFM path. Android and JVMs where FFM is unavailable
use the separate JNI binding path.

## Ground Rules

1. Keep the generated C layer internal. `org.maplibre.nativeffi.internal.c`
   remains an implementation detail. Public APIs expose `org.maplibre.nativeffi`
   values and handles.
2. Load the native library before the first generated downcall. Public entry
   points call the support-layer loader, which uses the existing exact-path
   lookup order from `NativeLibrary`.
3. Translate every status-returning call through one status helper. Read
   `mln_thread_last_error_message()` immediately on the same thread when a
   native call returns a non-OK status.
4. Preserve owner-thread affinity. Handle methods call native code on the
   invoking Java thread and report native `MLN_STATUS_WRONG_THREAD` as
   `WrongThreadException`.
5. Use deterministic release. Handles implement `AutoCloseable`. A successful
   close marks the wrapper released and makes later closes no-ops. Cleaner
   support reports leaks instead of destroying thread-affine handles from
   arbitrary threads.
6. Allocate native memory by lifetime. Use confined per-call arenas for
   temporary structs and strings, shared callback arenas for upcall stubs,
   object-owned arenas for object-owned native storage, and explicit native
   buffers for large reusable byte storage.
7. Validate Java-owned state in Java. Check released wrappers, one-shot request
   completion, callback-scoped frame activity, embedded NULs in null-terminated
   strings, and public buffer shape. Let the C ABI validate native handles and
   native state.

## Current Baseline

`bindings/java-ffm` already has:

- Gradle Java library setup with the `de.infolektuell.jextract` plugin;
- a generated include argfile at `src/jextract/maplibre-native-c.includes`;
- a mise task that refreshes the argfile from jextract's include report;
- `NativeLibrary`, which loads `maplibre-native-c` from an explicit property,
  environment variable, or `java.library.path`;
- a native-library smoke test that proves generated jextract calls can reach
  `mln_c_version()`.

Before public API work, resolve the Java release policy. The conventions target
JDK 22 or newer, while the current Gradle task sets `options.release` to 25.
Preferred outcome: compile the public and generated sources with `--release 22`
using a jextract version whose output stays on the final FFM API. If jextract 25
emits JDK 25-only source, keep JDK 25 as a temporary development floor and track
a follow-up to restore JDK 22 compatibility.

## Public Package Shape

Use these package boundaries:

| Package                             | Purpose                                                                                                                                       |
| ----------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| `org.maplibre.nativeffi`            | Public handles, values, descriptors, enums, callbacks, events, and exceptions.                                                                |
| `org.maplibre.nativeffi.internal`   | Native library loading, status conversion, handle state, arenas, UTF-8 helpers, native buffers, callback state, and generated-layer adapters. |
| `org.maplibre.nativeffi.internal.c` | Generated jextract declarations only.                                                                                                         |

Recommended public entry points:

- `MapLibre`: static process/global helpers such as `loadNativeLibrary`,
  `cVersion`, `supportedRenderBackends`, network status, and logging.
- `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`, `RenderSessionHandle`:
  owned native handles.
- `NativePointer`: opaque borrowed native address value used by surface and
  texture descriptors.
- `NativeBuffer`: explicit reusable off-heap byte storage for large readback and
  upload paths.
- `MapLibreException` plus stable subclasses for invalid argument, invalid
  state, wrong thread, unsupported feature, and native error statuses.

Java exceptions should be unchecked. Native calls can fail for lifecycle and
thread-affinity reasons even when method signatures are otherwise valid, and
unchecked exceptions keep the low-level API usable in `AutoCloseable` and
callback contexts. Each exception carries the native status and copied
diagnostic string.

## Value, Descriptor, And Enum Policy

- Use records for immutable scalar values: `LatLng`, `LatLngBounds`,
  `ScreenPoint`, `ScreenBox`, `EdgeInsets`, `TileId`, `RenderingStats`, and
  texture/image metadata.
- Use mutable descriptor objects for option types with field masks: camera,
  animation, bounds, viewport, tile, style image, tile source, custom geometry
  source, render target, resource response, and offline region descriptors.
  Fluent `set...()` methods return `this` and mark the corresponding field
  present. Matching `clear...()` methods mark fields absent, and `has...()`
  methods report presence. Callers can use a new descriptor when they want a
  fully empty state. Leave native `size` and `fields` materialization to
  internal boundary code.
- Initialize C `size` fields in internal materializers. Public callers set only
  semantic fields.
- Represent C bit masks as `EnumSet<T>` or small purpose-built value types.
- Use Java enums for closed input domains. For output domains that may grow,
  preserve the raw native value in the copied Java object so newer C libraries
  can be diagnosed safely.
- Model JSON, geometry, GeoJSON, features, and queried data as Java value trees.
  Define `JsonValue` as a sealed interface with immutable record variants for
  payload values and a singleton variant for JSON null. Record constructors
  validate finite doubles, reject null children, and defensively copy
  list-backed arrays and objects. Preserve object member order with a member
  record; optional map factories may sit above the low-level shape. Choose the
  public representation for unsigned 64-bit JSON numbers before stabilizing the
  API. Materialize input trees into temporary native descriptor graphs at the
  call boundary, and copy native snapshots/result views back into independent
  Java values before releasing native handles.

## Internal Support Layer

Build this support layer before broad API coverage:

1. `NativeAccess` or `NativeRuntime` loader wrapper.
   - Calls `NativeLibrary.load()` once.
   - Performs optional ABI checks with `mln_c_version()`.
   - Produces clear errors for missing `--enable-native-access` or missing
     native symbols.
2. `Status` conversion.
   - Maps native status integers to a Java status enum.
   - Calls `mln_thread_last_error_message()` immediately after failures.
   - Throws the right `MapLibreException` subclass.
3. `HandleState`.
   - Stores the native pointer segment, released flag, parent references, type
     name, and debug allocation context.
   - Offers `requireLive()` and `closeOnce()` helpers.
4. `MemoryUtil`.
   - Owns UTF-8 null-terminated strings, `mln_string_view` values, arrays,
     boolean and size out parameters, and pointer out parameters.
   - Rejects embedded NUL for null-terminated C string inputs.
   - Copies C strings and string views by byte length before arena close.
5. `DescriptorWriter` and `DescriptorReader` helpers.
   - Materialize Java descriptors into jextract structs.
   - Read output structs into Java values.
6. `CallbackScope`.
   - Uses shared arenas for upcall stubs that native code may invoke from
     worker, network, logging, or render-related threads.
   - Stores Java callback objects strongly for the native owner scope.
   - Catches `Throwable` inside every upcall and converts it to the documented C
     callback behavior.

## Milestones

### 1. Build, Generator, And Native Access

Implement:

- verify `jextract:update-includes` produces a stable argfile from
  `include/maplibre_native_c/**`;
- add a Gradle check that fails when the generated include argfile is stale;
- decide and enforce the Java release target;
- document test JVM flags for FFM native access;
- keep generated sources under Gradle output unless the project later chooses to
  check them in for platform comparison.

Done when:

- `mise run //bindings/java-ffm:jextract:update-includes` is repeatable;
- `../../gradlew :bindings:java-ffm:build` runs from a clean checkout after the
  native library is built;
- tests explain how to set `MLN_FFI_BUILD_DIR` or the explicit library path.

### 2. Exceptions, Status, Strings, And Handles

Implement:

- `MapLibreException` hierarchy and status mapping;
- immediate diagnostic capture;
- Java-side released-handle checks;
- base `AutoCloseable` handle support;
- UTF-8 helpers for C strings and `mln_string_view`;
- `NativePointer` and internal pointer conversion.

Tests:

- invalid enum/status call reports the expected exception class and diagnostic;
- released handles reject later method calls before native dispatch;
- null-terminated string helpers reject embedded NUL;
- `NativePointer` round-trips null and non-null addresses internally without
  exposing `MemorySegment` publicly.

### 3. Process Services

Implement:

- `MapLibre.cVersion()`;
- `MapLibre.supportedRenderBackends()`;
- process-global network status get/set;
- process-global logging callback and async severity mask.

Tests:

- C version and backend mask call through the public wrapper;
- network status accepts known values and rejects unknown values through status
  conversion;
- logging callback state survives until cleared or replaced, and callback
  exceptions never unwind into native code.

### 4. Runtime Handles, Events, And Resource Hooks

Implement:

- `RuntimeOptions` builder and `RuntimeHandle.create(options)`;
- `RuntimeHandle.runOnce()`, ambient cache operations, and `close()`;
- copied runtime events with typed payload records;
- live map registry on `RuntimeHandle` for map-originated events;
- resource transform callback;
- resource provider callback with copied requests and `ResourceRequestHandle`
  one-shot completion/release.

Tests:

- create and close a runtime on one thread;
- wrong-thread calls throw `WrongThreadException` with the native diagnostic;
- polling with no events returns empty without leaking arena state;
- resource request handles enforce exactly-once completion and release where a
  real provider test is practical.

### 5. Map Lifecycle, Camera, And Projection

Implement:

- `MapOptions` and `MapHandle.create(runtime, options)`;
- map close with parent runtime retention;
- style URL and style JSON loading;
- repaint and still-image requests;
- camera, animation, bounds, viewport, tile, free-camera, and projection-mode
  descriptors;
- camera commands and coordinate conversions;
- `MapProjectionHandle` snapshot helper and Mercator meter helpers.

Tests:

- runtime keeps parent validity while maps are live;
- runtime close fails while a map is live and succeeds after map close;
- map options and default descriptors materialize expected `size` and field
  masks;
- projection helpers close independently and reject use after release;
- wrong-thread map and projection calls map to `WrongThreadException`.

### 6. JSON, Geometry, GeoJSON, And Feature Values

Implement:

- sealed Java value trees for JSON and GeoJSON concepts, including a `JsonValue`
  sealed interface implemented by immutable record variants and a singleton null
  variant;
- materializers for `mln_json_value`, `mln_geometry`, `mln_feature`, and
  `mln_geojson` descriptor graphs;
- readers that copy native JSON/feature views into Java value trees;
- depth checks or clear Java errors before C rejects overly deep graphs when the
  binding can diagnose them cheaply.

Tests:

- primitive, array, and object JSON values materialize and copy back correctly;
- geometry collections, polygons, and feature identifiers materialize correctly;
- descriptor memory stays alive for the full native call and is released after
  the call.

### 7. Style Sources, Layers, Images, And Custom Geometry

Implement:

- style source JSON add/remove/exists/type/info/list APIs;
- GeoJSON, vector, raster, raster DEM, image, and custom geometry source APIs;
- style image set/remove/exists/info/copy APIs;
- layer add/remove/exists/type/list/move APIs;
- light, layer property, and layer filter APIs;
- location indicator helpers;
- custom geometry source callback state tied to `MapHandle` and source identity.

Tests:

- source and layer list handles copy IDs into Java lists and release native list
  handles;
- image copy APIs support size-query then copy flows;
- style property snapshots copy JSON before native snapshot release;
- custom geometry callbacks stay strongly reachable until the map drops their
  owner scope.

### 8. Render Sessions And Render Targets

Implement:

- `RenderSessionHandle` lifecycle, resize, render update, detach, memory
  maintenance, and debug logs;
- default owned texture target attach;
- Metal and Vulkan surface, owned texture, and borrowed texture descriptors;
- `NativePointer` conversion for backend handles;
- CPU premultiplied RGBA8 readback into `NativeBuffer` and convenience `byte[]`;
- callback-scoped frame access for Metal and Vulkan owned textures.

Tests:

- unsupported backend attaches throw `UnsupportedFeatureException` where the
  current build lacks a backend;
- session close detaches exactly once and map close requires no live session;
- readback reports required byte length when the supplied buffer is too small;
- acquired frame callbacks always release the frame after callback success or
  failure.

### 9. Feature State, Queries, Snapshots, And Offline Regions

Implement:

- feature-state selector builders;
- set/get/remove feature state;
- rendered/source feature query options and query geometry;
- feature query result copying;
- feature extension result copying;
- JSON snapshot copying;
- offline region descriptors, snapshots, lists, status, observation,
  download-state, invalidation, deletion, metadata update, and database merge.

Tests:

- snapshot/list handles release after their contents are copied;
- missing result cases return Java `Optional` or empty values consistently;
- feature query and offline APIs propagate native invalid-state and wrong-thread
  statuses through the public exception model.

### 10. Owner-Thread Helper Optional Layer

The low-level handle APIs stay direct. After direct coverage works, add a small
optional helper only if tests or examples show a clear need:

- `OwnerThread` owns one platform thread;
- it can create a runtime, run submitted tasks on that thread, pump `runOnce()`,
  and drain events;
- it returns ordinary handles and keeps direct handle APIs available.

Keep UI dispatch, coroutines, listeners, promises, and application scheduling in
adapters above this low-level binding.

### 11. Smoke Example And Packaging

Implement:

- a small Java FFM smoke example that loads the native library, creates a
  runtime and map, loads a style URL or JSON, pumps events, and closes handles;
- an optional offscreen readback example when the native backend is available;
- Gradle publication metadata for the Java artifact;
- clear native-library loading documentation.

Recommended initial packaging:

- publish the Java binding separately from native binaries;
- require callers to provide the native `maplibre-native-c` library through an
  exact path or platform library path;
- add platform classifier artifacts later when CI builds and signs native
  libraries for each supported variant.

Tests:

- example compiles as part of CI;
- smoke example exits under a short timeout;
- build output states the native library path requirement when tests are skipped
  or fail due missing native artifacts.

### 12. Documentation And Generated Reference

Implement:

- public Javadocs for ownership, owner-thread rules, callback threading, native
  pointer lifetimes, and buffer lifetimes;
- docs under `docs/src/content/docs/development/` for Java FFM contributor notes
  as implementation details evolve;
- generated or curated reference pages when the public API stabilizes.

Done when:

- public docs name every handle's close rule;
- callback docs state invocation threads and allowed native calls;
- render-target docs state borrowed backend object lifetimes;
- examples stay small and focused.

## Header Coverage Map

| C header           | Java public area                                                                                                                    |
| ------------------ | ----------------------------------------------------------------------------------------------------------------------------------- |
| `base.h`           | status enum, render backend enum set, C version, supported backend mask, handle pointer support                                     |
| `diagnostics.h`    | internal diagnostic capture in status conversion                                                                                    |
| `logging.h`        | process logging callback, severity/event enums, async severity mask                                                                 |
| `runtime.h`        | runtime options/handle, network status, events, resource transform/provider, ambient cache, offline status payloads                 |
| `map.h`            | map options/handle, shared scalar values, JSON/GeoJSON/feature descriptors, offline regions, projection mode, viewport/tile options |
| `camera.h`         | camera snapshots, camera commands, bounds, fitting, free camera, coordinate conversion                                              |
| `projection.h`     | projection handle and Mercator helper functions                                                                                     |
| `style.h`          | style sources, layers, images, custom geometry callbacks, light/layer/filter properties, style ID lists                             |
| `surface.h`        | Metal and Vulkan surface render target descriptors                                                                                  |
| `texture.h`        | owned/borrowed texture descriptors, readback, callback-scoped owned texture frames                                                  |
| `render_session.h` | render session lifecycle, render updates, feature state, JSON snapshots                                                             |
| `query.h`          | rendered/source feature queries, feature extension queries, result copying                                                          |

## Test Strategy

Add tests with each milestone. Prefer small tests around real C calls, and rely
on C ABI tests for native validation already covered there.

Required test categories:

- generated-layer smoke tests;
- native library loading and native access diagnostics;
- status-to-exception conversion;
- deterministic release and use-after-release checks;
- owner-thread error propagation;
- descriptor defaulting and field-mask materialization;
- UTF-8 and string-view copying;
- JSON, GeoJSON, event, snapshot, and query result copying;
- callback lifetime, exception containment, and one-shot completion rules;
- render target attach/readback/acquire behavior for available CI variants.

Use `mise run test` before merging broad feature work. Use
`mise run //bindings/java-ffm:build` for focused binding iteration.

## Definition Of Done

The Java FFM binding is complete when:

- every exported C ABI function has a public Java wrapper or a documented reason
  for remaining internal;
- every C-owned long-lived handle has deterministic Java release;
- public APIs expose no FFM classes or generated jextract classes;
- statuses and diagnostics become stable Java exceptions;
- callbacks hold Java state for the full native owner scope and catch all
  exceptions;
- borrowed C strings, event payloads, snapshots, lists, and query results are
  copied before their native validity window ends;
- owner-thread behavior is documented and tested;
- surface and texture APIs model backend-native handles as borrowed
  `NativePointer` values;
- tests and the smoke example pass through `mise run test` in supported CI
  variants.
