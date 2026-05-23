# C# Binding Implementation Map

## Purpose

This reference is for contributors implementing and reviewing the C#/.NET
binding. It names concrete files, projects, tasks, and coverage targets. The
convention documents remain the source of design rules.

## Normative references

The implementation follows these documents. This spec links to them instead of
restating their rules.

- [Concepts](../../docs/src/content/docs/concepts.md): runtime, map, render
  session, events, and ownership boundaries.
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md):
  status, diagnostics, callbacks, ABI ownership, and thread-affinity contract.
- [Binding conventions](../../docs/src/content/docs/development/bindings.md):
  shared handle, type, callback, rendering, and testing rules.
- [C# binding conventions](../../docs/src/content/docs/development/bindings-csharp.md):
  .NET architecture, generated interop, handle lifecycle, diagnostics, native
  memory, callbacks, and render target rules.

When this spec and a convention document appear to overlap, the convention
contains the rule and this spec names the concrete .NET implementation points.
The public C headers are the ABI source.

## Scope

`bindings/dotnet` is the low-level C# binding over the public MapLibre Native C
API. It targets `net10.0`, uses source-generated P/Invoke for raw C calls, keeps
raw declarations internal, and exposes a public .NET API that preserves the C
model.

The public assembly and root namespace are `Maplibre.Native`. Public namespaces
group C concepts: `Camera`, `Geo`, `Json`, `Log`, `Map`, `Offline`, `Query`,
`Render`, `Resource`, `Runtime`, `Style`, and `Error`. Internal namespaces own C
declarations, native library loading, status conversion, descriptor
materialization, memory helpers, callback state, and handle state.

This package remains low-level. WPF, WinUI, MAUI, Avalonia, ASP.NET, async,
thread-pool, `SynchronizationContext`, and view-lifecycle adapters belong above
this package.

## Current scaffold

```text
bindings/dotnet/
  SPEC.md
  dotnet-tools.json
  Maplibre.Native.slnx
  mise.toml
  scripts/generate-clangsharp.sh
  src/Maplibre.Native/
    Maplibre.Native.csproj
    Maplibre.cs
    NativePointer.cs
    NetworkStatus.cs
    RenderBackend.cs
    Error/
      InvalidArgumentException.cs
      InvalidStateException.cs
      MaplibreException.cs
      MaplibreStatus.cs
      NativeErrorException.cs
      UnsupportedFeatureException.cs
      WrongThreadException.cs
    Generated/
      *.g.cs
      NativeAttributes.cs
      README.md
    Internal/C/NativeMethods.cs
    Internal/Handle/NativeHandleState.cs
    Internal/Loader/NativeLibraryLoader.cs
    Internal/Memory/NativeUtf8String.cs
    Internal/Status/NativeStatus.cs
    Internal/Struct/*.cs
    Camera/*.cs
    Geo/*.cs
    Json/*.cs
    Log/*.cs
    Map/*.cs
    Offline/*.cs
    Query/*.cs
    Render/*.cs
    Resource/*.cs
    Runtime/*.cs
    Style/*.cs
  tests/Maplibre.Native.Tests/
    CustomGeometrySourceTests.cs
    GeneratedLayoutTests.cs
    GeoJsonSourceTests.cs
    MapCameraOptionsTests.cs
    MaplibreTests.cs
    NativeHandleStateTests.cs
    NativeLibraryTestSupport.cs
    NativeStatusTests.cs
    NativeUtf8StringTests.cs
    OfflineStructTests.cs
    PublicApiSurfaceTests.cs
    QueryStructTests.cs
    RenderSessionTests.cs
    ResourceProviderTests.cs
    ResourceResponseTests.cs
    ResourceTransformTests.cs
    RuntimeEventTests.cs
    RuntimeOfflineOperationTests.cs
    StyleImageTests.cs
    StyleJsonTests.cs
    StyleLayerTests.cs
    Maplibre.Native.Tests.csproj
```

The scaffold implements one proof slice:

- `Maplibre.CVersion()` calls `mln_c_version()` through a source-generated
  `LibraryImport` stub.
- `Maplibre.SupportedRenderBackends()` preserves backend mask bits in a C#
  `[Flags]` enum.
- `Maplibre.NetworkStatus()` and `Maplibre.SetNetworkStatus(...)` cross the C
  ABI and translate status failures into `MaplibreException` subclasses.
- `NativeLibraryLoader` supports an exact library path through
  `Maplibre.Native.LibraryPath`, `MAPLIBRE_NATIVE_FFI_LIBRARY_PATH`, and the
  repository `MLN_FFI_BUILD_DIR` artifact path.
- `NativeStatus` copies `mln_thread_last_error_message()` immediately after a
  non-OK status.
- `NativePointer` is a borrowed opaque address value with no memory access in
  the public API.
- `RuntimeHandle` and `MapHandle` establish the close-once owner-thread handle
  pattern over generated C declarations, including runtime event polling, map
  camera/fit/transition/bounds/free-camera/projection/viewport/tile/debug option
  calls, map coordinate conversion, projection snapshot handles, offline
  operation start/discard tokens, resource provider callbacks, resource
  transform callbacks, JSON materialization, and style source/layer JSON
  operations, style source URL/tile APIs, style source metadata/listing, style
  layer listing, style image APIs, image source APIs, typed DEM/location layer
  helpers, custom geometry source callbacks/APIs, GeoJSON/geometry
  materialization, GeoJSON source data APIs, render session lifecycle/feature
  state wrappers, offline region result readers/take-result APIs, query
  wrappers/results, render surface/texture descriptor materialization, texture
  read/frame wrappers, style JSON snapshots, layer properties, and layer
  filters.
- `NativeUtf8String` rejects embedded NUL values and owns temporary UTF-8 C
  string storage for call-boundary inputs.
- Public value, descriptor, enum, and placeholder handle types exist across the
  concept namespaces so future method slices can fill behavior without changing
  the broad API shape.
- ClangSharp-generated files in `Generated/*.g.cs` cover the public C headers.
- `GeneratedLayoutTests` verifies layout-sensitive binding facts that do not
  require the native library.
- Custom geometry source tests cover callback tile-ID copying, exception
  swallowing, descriptor materialization, and native custom source operations.
- GeoJSON source tests cover geometry/feature materialization and native GeoJSON
  source data adaptation.
- Offline struct tests cover offline region definition materialization, copied
  offline region info metadata, and offline status conversion.
- Query tests cover query geometry/options materialization and copied queried
  feature data.
- Render session tests cover surface/texture descriptor materialization, texture
  image info copying, texture frame invalidation, and feature-state selector
  materialization.
- Resource provider tests cover request copying, decision conversion, exception
  conversion, and install/replace behavior.
- Resource response tests cover byte cloning and native completion descriptor
  materialization.
- Resource transform tests cover request copying, replacement URL lifetime,
  exception conversion, and install/replace/clear behavior.
- Style image tests cover image descriptor materialization, metadata, pixel
  copying, removal, and image source coordinate/image adaptation.
- Style JSON tests cover JSON materialization, finite-number validation, style
  source/layer JSON native adaptation, style source URL/tile APIs, style source
  metadata/listing, style layer listing, style JSON snapshots, layer properties,
  and layer filters.
- Style layer tests cover typed DEM and location-indicator layer helper
  adaptation.
- Native-library tests cover the C ABI version call, projection helper
  round-tripping, runtime event polling, offline operation start/discard, map
  camera/fit/viewport/tile option round-tripping, camera transition command
  adaptation, bounds/projection/free camera adaptation, map coordinate
  conversion, projection snapshot lifecycle and coordinate conversion,
  runtime/map close behavior, map debug option round-tripping, closed-wrapper
  validation, process-global log callback installation/clearing, and native
  status diagnostic mapping when run through `mise run //bindings/dotnet:test`.
- `PublicApiSurfaceTests` keeps representative public concept types present as
  the binding surface expands.

## Build artifacts and tasks

| Artifact          | Path                            | Contents                                                                  |
| ----------------- | ------------------------------- | ------------------------------------------------------------------------- |
| .NET solution     | `bindings/dotnet`               | Binding solution and mise task root.                                      |
| Public library    | `src/Maplibre.Native`           | Public C# API, internal interop, loader, status conversion, future tests. |
| Generated C layer | `src/Maplibre.Native/Generated` | ClangSharp declarations kept internal to the assembly.                    |
| Test project      | `tests/Maplibre.Native.Tests`   | Binding tests and generated layout smoke tests.                           |

Implemented tasks:

| Task                                  | Required behavior                                                     |
| ------------------------------------- | --------------------------------------------------------------------- |
| `mise run //bindings/dotnet:generate` | Refresh ClangSharp declarations from `include/maplibre_native_c/*.h`. |
| `mise run //bindings/dotnet:build`    | Build `Maplibre.Native.slnx`.                                         |
| `mise run //bindings/dotnet:test`     | Run current .NET tests.                                               |
| `mise run //bindings/dotnet:ci`       | Run the binding's current CI check slice.                             |
| `dotnet build Maplibre.Native.slnx`   | Build the .NET solution from this folder.                             |
| `dotnet test Maplibre.Native.slnx`    | Run the .NET solution tests from this folder.                         |

Planned tasks:

| Task                              | Required behavior                                                        |
| --------------------------------- | ------------------------------------------------------------------------ |
| `mise run //bindings/dotnet:pack` | Produce the NuGet package after native artifact distribution is defined. |

Package consumers provide the platform C library according to the future native
artifact policy. Local tests load the repository build artifact selected by
`MLN_FFI_BUILD_DIR`.

## Namespace and assembly map

Create these public namespaces as concrete types land:

```text
Maplibre.Native
Maplibre.Native.Camera
Maplibre.Native.Error
Maplibre.Native.Geo
Maplibre.Native.Json
Maplibre.Native.Log
Maplibre.Native.Map
Maplibre.Native.Offline
Maplibre.Native.Query
Maplibre.Native.Render
Maplibre.Native.Resource
Maplibre.Native.Runtime
Maplibre.Native.Style
```

Create these internal namespaces. They remain implementation details:

```text
Maplibre.Native.Internal.C
Maplibre.Native.Internal.Callback
Maplibre.Native.Internal.Handle
Maplibre.Native.Internal.Loader
Maplibre.Native.Internal.Memory
Maplibre.Native.Internal.Status
Maplibre.Native.Internal.Struct
```

Add internal namespaces only when their names identify a concrete role.

## C# implementation points

- Keep raw ABI declarations below the public API in `Internal.C` and future
  `Generated` files.
- Centralize status conversion and copy diagnostics immediately in
  `NativeStatus`.
- Track close-once native state separately from public handle policy so handles
  can support fallible `Close()` and non-throwing `Dispose()`.
- Copy borrowed native data before the native borrow window ends. C# readers use
  `try/finally` and private guards for native result handles.
- Use the shared C API concepts with .NET names: `Handle` suffixes,
  `NativePointer`, copied values, descriptors, and stable exception categories.
- Run low-level binding calls on the calling thread. C# reports
  `WrongThreadException` and adds no dispatcher.

## Status and exception map

Status-returning C calls pass through `NativeStatus.Check(...)`. On non-OK
status, `NativeStatus` copies `mln_thread_last_error_message()` on the same
thread before creating the public exception.

| C status                      | Raw value           | `MaplibreStatus`  | Public exception              |
| ----------------------------- | ------------------- | ----------------- | ----------------------------- |
| `MLN_STATUS_OK`               | `0`                 | `Ok`              | none                          |
| `MLN_STATUS_INVALID_ARGUMENT` | `-1`                | `InvalidArgument` | `InvalidArgumentException`    |
| `MLN_STATUS_INVALID_STATE`    | `-2`                | `InvalidState`    | `InvalidStateException`       |
| `MLN_STATUS_WRONG_THREAD`     | `-3`                | `WrongThread`     | `WrongThreadException`        |
| `MLN_STATUS_UNSUPPORTED`      | `-4`                | `Unsupported`     | `UnsupportedFeatureException` |
| `MLN_STATUS_NATIVE_ERROR`     | `-5`                | `NativeError`     | `NativeErrorException`        |
| future non-OK status          | preserved raw value | `Unknown`         | `MaplibreException`           |

Binding-owned validation failures that do not come from C use the matching
`MaplibreStatus` when one applies, a null raw status, and a concrete diagnostic.

## .NET differences and omissions

Record C#-specific differences here. Keep the `None` row only when the binding
intentionally mirrors the public C ABI coverage for all supported features.

| Item                   | Difference or omission                                                                                                 | Reason                                                                                                     | User-visible behavior                                                                                   | Tests/docs impact                                    |
| ---------------------- | ---------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------- | ---------------------------------------------------- |
| Finalizers             | Thread-affine handles use leak reporting, not finalizer destruction.                                                   | The .NET finalizer and `SafeHandle.ReleaseHandle()` may run on arbitrary threads.                          | `Close()` reports native destruction errors; `Dispose()` performs best-effort cleanup without throwing. | Tests cover close retry and leak-reporting hooks.    |
| Generated C layer      | ClangSharp output uses generated `DllImport` declarations while the proof-slice handwritten calls use `LibraryImport`. | ClangSharp is the selected declaration generator; handwritten source-generated imports cover curated gaps. | Public API is unaffected. Raw declarations remain internal.                                             | Layout tests cover generated ABI shape smoke checks. |
| NuGet native artifacts | Native artifact packaging is pending.                                                                                  | Repository CI first proves binding behavior against built artifacts.                                       | Consumers load a local or system native library.                                                        | Pack and install docs land with artifact policy.     |

## Public API inventory

Implement public C# types that preserve the public C API concept inventory while
using .NET naming, properties, `IDisposable`, `ReadOnlySpan<T>`, `Span<T>`,
`Memory<T>`, records, structs, and exceptions where those choices keep the
low-level contract intact.

### Root

- `Maplibre`
- `NetworkStatus`
- `RenderBackend`
- `NativePointer`

### `Error`

- `InvalidArgumentException`
- `InvalidStateException`
- `MaplibreException`
- `MaplibreStatus`
- `NativeErrorException`
- `UnsupportedFeatureException`
- `WrongThreadException`

### `Camera`

- `AnimationOptions`
- `BoundOptions`
- `CameraFitOptions`
- `CameraOptions`
- `EdgeInsets`
- `FreeCameraOptions`
- `ProjectionModeOptions`
- `UnitBezier`

### `Geo`

- `CanonicalTileId`
- `Feature`
- `FeatureIdentifier`
- `GeoJson`
- `Geometry`
- `LatLng`
- `LatLngBounds`
- `ProjectedMeters`
- `Quaternion`
- `ScreenBox`
- `ScreenPoint`
- `TileId`
- `Vec3`

### `Json`

- `JsonValue`

### `Log`

- `LogCallback`
- `LogEvent`
- `LogRecord`
- `LogSeverity`
- `LogSeverityMask`

### `Map`

- `ConstrainMode`
- `DebugOptions`
- `MapHandle`
- `MapMode`
- `MapOptions`
- `MapProjectionHandle`
- `NorthOrientation`
- `ProjectionModeOptions`
- `RenderingStats`
- `TileLodMode`
- `TileOperation`
- `TileOptions`
- `ViewportMode`
- `ViewportOptions`

### `Offline`

- `OfflineRegionDefinition`
- `OfflineRegionDownloadState`
- `OfflineRegionInfo`
- `OfflineRegionStatus`

### `Query`

- `FeatureExtensionResult`
- `FeatureStateSelector`
- `QueriedFeature`
- `RenderedFeatureQueryOptions`
- `RenderedQueryGeometry`
- `SourceFeatureQueryOptions`

### `Render`

- `FrameNativePointer`
- `MetalBorrowedTextureDescriptor`
- `MetalContextDescriptor`
- `MetalOwnedTextureDescriptor`
- `MetalOwnedTextureFrame`
- `MetalOwnedTextureFrameHandle`
- `MetalSurfaceDescriptor`
- `NativeBuffer`
- `PremultipliedRgba8Image`
- `RenderMode`
- `RenderSessionHandle`
- `RenderTargetExtent`
- `TextureImageInfo`
- `VulkanBorrowedTextureDescriptor`
- `VulkanContextDescriptor`
- `VulkanOwnedTextureDescriptor`
- `VulkanOwnedTextureFrame`
- `VulkanOwnedTextureFrameHandle`
- `VulkanSurfaceDescriptor`

### `Resource`

- `ByteRange`
- `ResourceErrorReason`
- `ResourceKind`
- `ResourceLoadingMethod`
- `ResourcePriority`
- `ResourceProviderDecision`
- `ResourceRequest`
- `ResourceRequestHandle`
- `ResourceResponse`
- `ResourceResponseStatus`
- `ResourceStoragePolicy`
- `ResourceTransformRequest`
- `ResourceUsage`

### `Runtime`

- `AmbientCacheOperation`
- `NetworkStatus`
- `OfflineOperationHandle`
- `OfflineOperationKind`
- `OfflineOperationResultKind`
- `RuntimeEvent`
- `RuntimeEventPayload`
- `RuntimeEventSourceType`
- `RuntimeEventType`
- `RuntimeHandle`
- `RuntimeOptions`

### `Style`

- `CustomGeometrySourceCallback`
- `CustomGeometrySourceOptions`
- `LocationIndicatorImageKind`
- `RasterDemEncoding`
- `SourceInfo`
- `SourceType`
- `StyleImage`
- `StyleImageInfo`
- `StyleImageOptions`
- `TileScheme`
- `TileSourceOptions`
- `VectorTileEncoding`
- Style layer, light, property, and filter APIs following the C API coverage map
  below.

## Public type map

| C or shared concept                 | C# type shape                                                                                    |
| ----------------------------------- | ------------------------------------------------------------------------------------------------ |
| `mln_runtime*`                      | `sealed class RuntimeHandle : IDisposable` with `Close()`; owns callbacks and map registry.      |
| `mln_map*`                          | `sealed class MapHandle : IDisposable`; retains `RuntimeHandle`.                                 |
| `mln_map_projection*`               | `sealed class MapProjectionHandle : IDisposable`; standalone snapshot after creation.            |
| `mln_render_session*`               | `sealed class RenderSessionHandle : IDisposable`; retains `MapHandle`.                           |
| `mln_resource_request_handle*`      | `sealed class ResourceRequestHandle : IDisposable`; thread-safe one-shot completion.             |
| Session-owned texture frame handles | `MetalOwnedTextureFrameHandle` and `VulkanOwnedTextureFrameHandle`; frame-scoped pointer access. |
| C option structs                    | C# descriptor classes or structs; materializers set `size`, masks, pointers, and nested storage. |
| C field masks                       | Nullable fields, explicit presence flags, or clear methods; C masks stay internal.               |
| Closed enum domains                 | C# enums with explicit raw conversion helpers.                                                   |
| Drift-prone output domains          | Small wrapper values preserving unknown raw values.                                              |
| C bit masks                         | `[Flags]` enums or purpose-built immutable mask values.                                          |
| Native result/list/snapshot handles | Internal guards that copy into .NET values before release.                                       |
| Opaque backend `void*` fields       | `NativePointer`; no ownership or memory access.                                                  |
| CPU images and resource bytes       | `byte[]`, `ReadOnlyMemory<byte>`, or caller-owned `Span<byte>` according to ownership.           |

## Handle lifecycle map

All public long-lived handles use deterministic `Close()` and implement
`IDisposable`. `Close()` reports native status and leaves the handle live when
native destruction fails. `Dispose()` uses the same release path when safe,
suppresses exceptions, and marks the wrapper closed only after successful native
release. Finalizers and `SafeHandle.ReleaseHandle()` report leaks for
thread-affine handles instead of destroying them from arbitrary GC threads.

| Public wrapper           | Native state                              | Release operation                                                                   | Owner thread                  | Parent and retained state                                                                       |
| ------------------------ | ----------------------------------------- | ----------------------------------------------------------------------------------- | ----------------------------- | ----------------------------------------------------------------------------------------------- |
| `RuntimeHandle`          | `mln_runtime*`                            | `mln_runtime_destroy`                                                               | runtime owner thread          | Owns runtime-scoped callbacks, resource provider/transform state, and map registry.             |
| `MapHandle`              | `mln_map*`                                | `mln_map_destroy`                                                                   | map owner thread              | Retains `RuntimeHandle`; unregisters from runtime registry after successful close.              |
| `MapProjectionHandle`    | `mln_map_projection*`                     | `mln_map_projection_destroy`                                                        | projection owner thread       | Standalone snapshot after creation; does not retain the source `MapHandle` for native validity. |
| `RenderSessionHandle`    | `mln_render_session*`                     | `mln_render_session_destroy` or `mln_render_session_detach` followed by destroy     | session owner thread          | Retains `MapHandle`; owns active texture frame state and render target descriptor state.        |
| `ResourceRequestHandle`  | `mln_resource_request_handle*`            | `mln_resource_request_complete` or `mln_resource_request_release`                   | C-permitted completion thread | Owned by resource provider handling; enforces one-shot completion and exactly-once release.     |
| `OfflineOperationHandle` | `mln_offline_operation_id` plus runtime   | `mln_runtime_offline_operation_discard` or matching `*_take_result`                 | runtime owner thread          | Retains `RuntimeHandle`; live until result is taken or operation is discarded.                  |
| Texture frame handles    | acquired Metal/Vulkan owned texture frame | `mln_metal_owned_texture_release_frame` or `mln_vulkan_owned_texture_release_frame` | session owner thread          | Retain `RenderSessionHandle`; invalidate scoped `NativePointer` access after close.             |

Short-lived native result, list, and snapshot handles stay internal. Guards copy
their data into .NET values and release native handles in `finally`.

## Internal implementation inventory

Implement these support files under `src/Maplibre.Native/Internal`:

| Area       | Contents                                                                                              |
| ---------- | ----------------------------------------------------------------------------------------------------- |
| `C`        | Generated or curated raw C declarations, constants, layouts, opaque pointer types, and raw functions. |
| `Loader`   | Native library lookup, exact path loading, build-directory loading, and package artifact loading.     |
| `Status`   | Status mapping, diagnostic capture, and exception creation.                                           |
| `Handle`   | Pointer storage, released state, parent retention hooks, close-once behavior, and leak reporting.     |
| `Memory`   | Scoped native memory, UTF-8 strings, string views, arrays, out-pointers, and reusable buffers.        |
| `Struct`   | Descriptor materializers and copied-result readers for each concept area.                             |
| `Callback` | Static unmanaged thunks, retained callback state, active-upcall accounting, and teardown rules.       |

## Callback implementation map

Callback state is stored strongly for the owner scope defined by the C API.
Callback thunks catch managed exceptions and convert them to the C callback's
documented result. Managed exceptions never unwind through native frames.

| Callback area          | Owner scope                    | Thunk and state strategy                                                                                                                                           | Threading and replacement behavior                                                                                                      |
| ---------------------- | ------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------- |
| Logging                | process-global                 | Static unmanaged thunk where possible; otherwise a pinned delegate and retained callback box.                                                                      | State is thread-safe. Replacing or clearing the callback releases old state only after native installation succeeds.                    |
| Resource transform     | `RuntimeHandle`                | Runtime-owned callback state copies request URL before user code and keeps replacement URL storage alive until native consumes it.                                 | May run on worker or network threads. Replacement installs the new native descriptor before releasing old managed state.                |
| Resource provider      | `RuntimeHandle`                | Provider callback copies request data, creates a `ResourceRequestHandle` only for handled requests, and returns pass-through immediately for non-handled requests. | May complete during callback or later from a C-permitted thread. Request handles enforce one-shot completion and exactly-once release.  |
| Custom geometry source | `MapHandle`/style source scope | Source callback state tracks active upcalls and owns tile callback storage for the source lifetime.                                                                | User exceptions become documented C callback failures. State release waits for in-flight upcalls before freeing managed/native context. |
| Texture frame scopes   | `RenderSessionHandle`          | Frame handles expose scoped copied metadata and `NativePointer` values while active.                                                                               | Frame access rejects use after close and blocks resize, render update, detach, and session destruction while acquired.                  |

Prefer `UnmanagedCallersOnly` static thunks when the C signature and .NET call
site support unmanaged function pointers. Use delegates only for generator gaps
or callback shapes that require delegate marshaling, and retain delegate
instances for the exact native registration scope.

## Generated interop policy

ClangSharp generation owns internal declarations for public C headers:

- constants and fixed-layout structs;
- opaque handle pointer wrappers or typed native pointer aliases;
- raw `mln_*` functions expressible with source-generated `LibraryImport`;
- callback function pointer signatures and ABI-compatible descriptor structs.

Generation inputs stay narrow:

```text
include/maplibre_native_c.h
include/maplibre_native_c/**/*.h
```

Generated symbols remain internal to `Maplibre.Native`. Public APIs never expose
ClangSharp names, generated layouts, raw pointers, or generated callback types.
Handwritten P/Invokes are allowed for generator gaps and keep the same internal
shape as generated calls.

## C API coverage map

Every public C function listed here needs a C# implementation or a recorded
unsupported reason before the binding leaves draft status. Reviewers compare
this list with `include/maplibre_native_c/*.h` during coverage reviews.

### Base and diagnostics

- `mln_c_version`
- `mln_supported_render_backend_mask`
- `mln_thread_last_error_message` (internal through status conversion)

### Logging

- `mln_log_set_callback`
- `mln_log_clear_callback`
- `mln_log_set_async_severity_mask`

### Runtime and resources

- `mln_network_status_get`
- `mln_network_status_set`
- `mln_runtime_options_default`
- `mln_runtime_create`
- `mln_runtime_set_resource_provider`
- `mln_resource_request_complete`
- `mln_resource_request_cancelled`
- `mln_resource_request_release`
- `mln_runtime_set_resource_transform`
- `mln_runtime_clear_resource_transform`
- `mln_runtime_run_ambient_cache_operation_start`
- `mln_runtime_offline_operation_discard`
- `mln_runtime_destroy`
- `mln_runtime_run_once`
- `mln_runtime_poll_event`

### Offline regions

- `mln_runtime_offline_region_create_start`
- `mln_runtime_offline_region_get_start`
- `mln_runtime_offline_regions_list_start`
- `mln_runtime_offline_regions_merge_database_start`
- `mln_runtime_offline_region_update_metadata_start`
- `mln_runtime_offline_region_get_status_start`
- `mln_runtime_offline_region_set_observed_start`
- `mln_runtime_offline_region_set_download_state_start`
- `mln_runtime_offline_region_invalidate_start`
- `mln_runtime_offline_region_delete_start`
- `mln_runtime_offline_region_create_take_result`
- `mln_runtime_offline_region_get_take_result`
- `mln_runtime_offline_regions_list_take_result`
- `mln_runtime_offline_regions_merge_database_take_result`
- `mln_runtime_offline_region_update_metadata_take_result`
- `mln_runtime_offline_region_get_status_take_result`
- `mln_offline_region_snapshot_get`
- `mln_offline_region_snapshot_destroy`
- `mln_offline_region_list_count`
- `mln_offline_region_list_get`
- `mln_offline_region_list_destroy`

### Map lifecycle and style document

- `mln_map_options_default`
- `mln_map_create`
- `mln_map_request_repaint`
- `mln_map_request_still_image`
- `mln_map_destroy`
- `mln_map_set_style_url`
- `mln_map_set_style_json`

### Camera and projection

- `mln_camera_options_default`
- `mln_animation_options_default`
- `mln_camera_fit_options_default`
- `mln_bound_options_default`
- `mln_free_camera_options_default`
- `mln_projection_mode_default`
- `mln_map_viewport_options_default`
- `mln_map_tile_options_default`
- `mln_map_set_debug_options`
- `mln_map_get_debug_options`
- `mln_map_set_rendering_stats_view_enabled`
- `mln_map_get_rendering_stats_view_enabled`
- `mln_map_is_fully_loaded`
- `mln_map_dump_debug_logs`
- `mln_map_get_viewport_options`
- `mln_map_set_viewport_options`
- `mln_map_get_tile_options`
- `mln_map_set_tile_options`
- `mln_map_get_camera`
- `mln_map_jump_to`
- `mln_map_ease_to`
- `mln_map_fly_to`
- `mln_map_move_by`
- `mln_map_move_by_animated`
- `mln_map_scale_by`
- `mln_map_scale_by_animated`
- `mln_map_rotate_by`
- `mln_map_rotate_by_animated`
- `mln_map_pitch_by`
- `mln_map_pitch_by_animated`
- `mln_map_cancel_transitions`
- `mln_map_camera_for_lat_lng_bounds`
- `mln_map_camera_for_lat_lngs`
- `mln_map_camera_for_geometry`
- `mln_map_lat_lng_bounds_for_camera`
- `mln_map_lat_lng_bounds_for_camera_unwrapped`
- `mln_map_get_bounds`
- `mln_map_set_bounds`
- `mln_map_get_free_camera_options`
- `mln_map_set_free_camera_options`
- `mln_map_get_projection_mode`
- `mln_map_set_projection_mode`
- `mln_map_pixel_for_lat_lng`
- `mln_map_lat_lng_for_pixel`
- `mln_map_pixels_for_lat_lngs`
- `mln_map_lat_lngs_for_pixels`
- `mln_map_projection_create`
- `mln_map_projection_destroy`
- `mln_map_projection_get_camera`
- `mln_map_projection_set_camera`
- `mln_map_projection_set_visible_coordinates`
- `mln_map_projection_set_visible_geometry`
- `mln_map_projection_pixel_for_lat_lng`
- `mln_map_projection_lat_lng_for_pixel`
- `mln_projected_meters_for_lat_lng`
- `mln_lat_lng_for_projected_meters`

### Query and feature results

- `mln_rendered_feature_query_options_default`
- `mln_source_feature_query_options_default`
- `mln_rendered_query_geometry_point`
- `mln_rendered_query_geometry_box`
- `mln_rendered_query_geometry_line_string`
- `mln_render_session_query_rendered_features`
- `mln_render_session_query_source_features`
- `mln_render_session_query_feature_extensions`
- `mln_feature_query_result_count`
- `mln_feature_query_result_get`
- `mln_feature_query_result_destroy`
- `mln_feature_extension_result_get`
- `mln_feature_extension_result_destroy`

### Render sessions and feature state

- `mln_render_session_resize`
- `mln_render_session_render_update`
- `mln_render_session_detach`
- `mln_render_session_destroy`
- `mln_render_session_reduce_memory_use`
- `mln_render_session_clear_data`
- `mln_render_session_dump_debug_logs`
- `mln_render_session_set_feature_state`
- `mln_render_session_get_feature_state`
- `mln_render_session_remove_feature_state`
- `mln_json_snapshot_get`
- `mln_json_snapshot_destroy`

### Style and custom geometry

- `mln_style_tile_source_options_default`
- `mln_custom_geometry_source_options_default`
- `mln_premultiplied_rgba8_image_default`
- `mln_style_image_options_default`
- `mln_style_image_info_default`
- `mln_style_id_list_count`
- `mln_style_id_list_get`
- `mln_style_id_list_destroy`
- `mln_map_add_style_source_json`
- `mln_map_remove_style_source`
- `mln_map_style_source_exists`
- `mln_map_get_style_source_type`
- `mln_map_get_style_source_info`
- `mln_map_copy_style_source_attribution`
- `mln_map_list_style_source_ids`
- `mln_map_add_geojson_source_url`
- `mln_map_add_geojson_source_data`
- `mln_map_set_geojson_source_url`
- `mln_map_set_geojson_source_data`
- `mln_map_add_vector_source_url`
- `mln_map_add_vector_source_tiles`
- `mln_map_add_raster_source_url`
- `mln_map_add_raster_source_tiles`
- `mln_map_add_raster_dem_source_url`
- `mln_map_add_raster_dem_source_tiles`
- `mln_map_add_custom_geometry_source`
- `mln_map_set_custom_geometry_source_tile_data`
- `mln_map_invalidate_custom_geometry_source_tile`
- `mln_map_invalidate_custom_geometry_source_region`
- `mln_map_set_style_image`
- `mln_map_remove_style_image`
- `mln_map_style_image_exists`
- `mln_map_get_style_image_info`
- `mln_map_copy_style_image_premultiplied_rgba8`
- `mln_map_add_image_source_url`
- `mln_map_add_image_source_image`
- `mln_map_set_image_source_url`
- `mln_map_set_image_source_image`
- `mln_map_set_image_source_coordinates`
- `mln_map_get_image_source_coordinates`
- `mln_map_add_hillshade_layer`
- `mln_map_add_color_relief_layer`
- `mln_map_add_location_indicator_layer`
- `mln_map_set_location_indicator_location`
- `mln_map_set_location_indicator_bearing`
- `mln_map_set_location_indicator_accuracy_radius`
- `mln_map_set_location_indicator_image_name`
- `mln_map_add_style_layer_json`
- `mln_map_remove_style_layer`
- `mln_map_style_layer_exists`
- `mln_map_get_style_layer_type`
- `mln_map_list_style_layer_ids`
- `mln_map_move_style_layer`
- `mln_map_get_style_layer_json`
- `mln_map_set_style_light_json`
- `mln_map_set_style_light_property`
- `mln_map_get_style_light_property`
- `mln_map_set_layer_property`
- `mln_map_get_layer_property`
- `mln_map_set_layer_filter`
- `mln_map_get_layer_filter`

### Surfaces and textures

- `mln_metal_surface_descriptor_default`
- `mln_vulkan_surface_descriptor_default`
- `mln_metal_surface_attach`
- `mln_vulkan_surface_attach`
- `mln_metal_owned_texture_descriptor_default`
- `mln_metal_borrowed_texture_descriptor_default`
- `mln_vulkan_owned_texture_descriptor_default`
- `mln_vulkan_borrowed_texture_descriptor_default`
- `mln_texture_image_info_default`
- `mln_metal_owned_texture_attach`
- `mln_metal_borrowed_texture_attach`
- `mln_vulkan_owned_texture_attach`
- `mln_vulkan_borrowed_texture_attach`
- `mln_texture_read_premultiplied_rgba8`
- `mln_metal_owned_texture_acquire_frame`
- `mln_metal_owned_texture_release_frame`
- `mln_vulkan_owned_texture_acquire_frame`
- `mln_vulkan_owned_texture_release_frame`

## Testing plan

C# tests exercise the public C# API against the real native library. Add tests
in small slices that prove binding-owned behavior:

- generated interop compilation and layout smoke tests;
- ABI version and supported backend calls;
- native library resolution through exact path and build directory;
- status, diagnostic, and exception subclass mapping;
- unknown output values that preserve raw native values;
- embedded-NUL rejection for null-terminated string inputs;
- `Close()` retry behavior and `Dispose()` best-effort behavior;
- parent retention while child handles are live;
- copied event, query, style, JSON, GeoJSON, and resource data;
- callback lifetime, exception capture, and replacement ordering;
- one-shot resource request completion and exactly-once release;
- wrong-thread propagation;
- native memory guard cleanup on success and failure;
- texture frame invalidation after close.

## Draft status

The binding is scaffolded. It leaves draft status when every C API coverage item
has a C# implementation or a recorded unsupported reason, generated interop and
layout checks run in CI, and tests cover the binding-owned lifetime, status,
callback, memory, and render-frame invariants listed above.
