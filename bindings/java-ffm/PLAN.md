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
- includes binding tests that exercise real C ABI calls.

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
  `mln_c_version()`;
- process helpers for C version, supported render backends, network status,
  logging callbacks, async log masks, and projected-meter conversion;
- runtime options, runtime/map/projection lifecycle wrappers, copied runtime
  events, live map lookup for map-originated events, and ambient cache
  operations;
- runtime resource transform and provider callbacks with copied requests,
  `ResourceResponse` materialization, and one-shot `ResourceRequestHandle`
  completion/release;
- map lifecycle, style loading, style sources/layers/images, custom geometry
  sources, repaint/still-image requests, debug helpers, camera commands,
  viewport/tile/bounds/free-camera/projection-mode options, projection helpers,
  coordinate conversions, and geometry-based camera fitting;
- render session and render target lifecycle wrappers, texture readback, native
  pointer descriptors, and callback-scoped owned texture frame access;
- feature state, rendered/source feature queries, feature extension queries, and
  query result copying;
- offline region creation, snapshots, lists, status, observation, download
  state, invalidation, deletion, metadata update, and database merge;
- JSON, geometry, GeoJSON, feature, and queried-feature value trees with
  descriptor materializers/readers and Java-side depth checks.

Java release policy: JDK 25 is the temporary development floor because jextract
25 emits `SymbolLookup.findOrThrow()`, which is unavailable under
`--release 22`. Restore JDK 22 compatibility when the generator output can
target that release while staying on the final FFM API.

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
- Use Java enums for closed input domains. Copied event and result records may
  include raw native values when they are useful for diagnostics across C ABI
  drift. Mutable descriptor snapshots can use `UNKNOWN` enum sentinels for
  forward-compatible output fields.
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

Add tests with each feature area. Prefer small tests around real C calls, and
rely on C ABI tests for native validation already covered there.

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
- tests pass through `mise run test` in supported CI variants.
