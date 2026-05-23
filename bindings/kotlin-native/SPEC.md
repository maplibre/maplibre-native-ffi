# Kotlin/Native Binding Implementation Map

## Audience and documentation role

Audience: contributors implementing and reviewing the Kotlin/Native binding.
Category: explanation-backed reference. This document records the implementation
map and points to the design rules instead of restating them.

## Normative references

The implementation follows these documents. This spec links to them instead of
restating their rules.

- [Concepts](../../docs/src/content/docs/concepts.md): runtime, map, render
  session, events, and ownership boundaries.
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md):
  status, diagnostics, callbacks, ABI ownership, and thread-affinity contract.
- [Binding conventions](../../docs/src/content/docs/development/bindings.md):
  shared handle, type, callback, rendering, and testing rules.
- [Kotlin binding conventions](../../docs/src/content/docs/development/bindings-kotlin.md):
  Kotlin/Native architecture, cinterop boundaries, memory, callbacks, and tests.
- [Java FFM binding conventions](../../docs/src/content/docs/development/bindings-java-ffm.md):
  Java public API parity target where Kotlin names stay intentionally close.
- [Java JNI binding conventions](../../docs/src/content/docs/development/bindings-java-jni.md):
  future Kotlin Multiplatform actual implementation alignment target.

When this spec and a convention document overlap, the convention contains the
rule and this spec names the concrete files, functions, and tests that implement
it. The public C headers are the ABI source. The Java FFM binding is the current
public surface parity source for concept coverage and low-level naming. The Java
JNI binding is the Android/JVM parity target as it lands.

## Scope

`bindings/kotlin-native` is the low-level Kotlin/Native binding over the public
MapLibre Native C API. It uses Kotlin/Native `cinterop` against
`include/maplibre_native_c.h`, keeps generated declarations internal, and
exposes a safe low-level Kotlin API that preserves the C model.

The package root is `org.maplibre.nativeffi`. Public packages group C concepts:
`runtime`, `map`, `render`, `resource`, `style`, `geo`, `camera`, `query`,
`offline`, `log`, `json`, and `error`. Internal packages own cinterop access,
status conversion, descriptor materialization, memory helpers, callback state,
handle state, and native-library or linker integration.

Kotlin/Native owns this implementation. `nativeMain` contains the public
Kotlin/Native API, its support internals, and all `kotlinx.cinterop` usage. A
future shared Kotlin Multiplatform facade can place aliases or `expect`
declarations in `commonMain`, with actual implementations backed by Java FFM on
`jvmMain`, Java JNI on `androidMain` when Android support exists, and this
Kotlin/Native implementation on native targets. This spec therefore focuses on
nativeMain implementation while preserving API parity with the Java bindings.

## Kotlin/Native differences and omissions

The in-scope `nativeMain` binding mirrors the public C ABI and Java FFM concept
coverage. Deferred items are outside this implementation slice.

| Item                                              | Difference or omission | Reason                                                                                     | User-visible behavior                                                                                                   | Tests/docs impact                                              |
| ------------------------------------------------- | ---------------------- | ------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| Publication metadata                              | Deferred.              | Publication policy is outside this goal.                                                   | No Maven/package publication is produced by this PR.                                                                    | Future packaging docs and tests will cover it.                 |
| Native-library distribution                       | Deferred.              | Distribution policy is outside this goal.                                                  | Tests link the repository build artifact through `MLN_FFI_BUILD_DIR`; consumers provide the C library by future policy. | Build docs record the explicit local linking model.            |
| `commonMain`, `jvmMain`, and `androidMain` facade | Deferred.              | This goal implements `nativeMain`; shared aliases and Android JNI actuals are future work. | Kotlin/Native targets use this binding directly.                                                                        | The API stays alignable with Java FFM/JNI for a future facade. |
| Examples                                          | Deferred.              | Examples are outside this goal.                                                            | No Kotlin/Native examples are added by this PR.                                                                         | Binding tests cover adaptation logic instead.                  |

## Current implementation layout

```text
bindings/kotlin-native/
  SPEC.md
  build.gradle.kts
  mise.toml
  src/nativeInterop/cinterop/maplibreNativeC.def
  src/nativeMain/kotlin/org/maplibre/nativeffi/**
  src/nativeTest/kotlin/org/maplibre/nativeffi/**
```

The implementation includes the original proof slice and the in-scope binding
surface:

- Gradle configures a host Kotlin/Native target.
- `cinterop` imports the public umbrella header into
  `org.maplibre.nativeffi.internal.c`.
- `Maplibre.cVersion()` calls `mln_c_version()` through the internal cinterop
  package.
- Public Kotlin packages cover runtime, map, render, resource, style, geo,
  camera, query, offline, log, JSON, and error concepts.
- Internal packages isolate cinterop, status conversion, memory helpers,
  descriptor materialization, callback state, and handle lifecycle.

## Build artifacts and tasks

Implement these artifacts:

| Artifact                    | Path                       | Contents                                                                      |
| --------------------------- | -------------------------- | ----------------------------------------------------------------------------- |
| Kotlin/Native Gradle module | `bindings/kotlin-native`   | `nativeMain` public API, Kotlin/Native internals, cinterop config, tests.     |
| Kotlin/Native cinterop klib | Gradle build output        | Generated declarations for the public C ABI, internal to the binding.         |
| Native test executable      | Gradle build output        | Kotlin/Native tests linked to the C library for the host target.              |
| Published package metadata  | Deferred outside this goal | Maven publication or Kotlin package metadata after artifact policy is chosen. |

Implement these tasks:

| Task                                                              | Required behavior                                        |
| ----------------------------------------------------------------- | -------------------------------------------------------- |
| `mise run //bindings/kotlin-native:build`                         | Build the Kotlin module and run Kotlin checks.           |
| `./gradlew :bindings:kotlin-native:build`                         | Build the host Kotlin/Native target.                     |
| `./gradlew :bindings:kotlin-native:cinteropMaplibreNativeC<Host>` | Regenerate cinterop declarations for the host target.    |
| `./gradlew :bindings:kotlin-native:allTests`                      | Run Kotlin/Native tests once linked test support exists. |

Native library resolution stays explicit. The binding links or loads the C
library using the repository build artifact selected by `MLN_FFI_BUILD_DIR` when
running tests. Package consumers provide the platform C library according to the
future distribution policy.

## Package map

Create these public packages:

```text
org.maplibre.nativeffi
org.maplibre.nativeffi.camera
org.maplibre.nativeffi.error
org.maplibre.nativeffi.geo
org.maplibre.nativeffi.json
org.maplibre.nativeffi.log
org.maplibre.nativeffi.map
org.maplibre.nativeffi.offline
org.maplibre.nativeffi.query
org.maplibre.nativeffi.render
org.maplibre.nativeffi.resource
org.maplibre.nativeffi.runtime
org.maplibre.nativeffi.style
```

Create these internal packages. They remain implementation details:

```text
org.maplibre.nativeffi.internal.c
org.maplibre.nativeffi.internal.callback
org.maplibre.nativeffi.internal.lifecycle
org.maplibre.nativeffi.internal.loader
org.maplibre.nativeffi.internal.memory
org.maplibre.nativeffi.internal.status
org.maplibre.nativeffi.internal.struct
```

Add internal packages only when their names identify a concrete role.

## Public API inventory

Implement public Kotlin types that preserve the Java FFM concept inventory while
using Kotlin naming, properties, nullability, builders, and value classes where
those choices keep the low-level contract intact.

### Root

- `Maplibre`

### `camera`

- `AnimationOptions`
- `BoundOptions`
- `CameraFitOptions`
- `CameraOptions`
- `EdgeInsets`
- `FreeCameraOptions`
- `UnitBezier`

### `error`

- `InvalidArgumentException`
- `InvalidStateException`
- `MaplibreException`
- `MaplibreStatus`
- `NativeErrorException`
- `UnsupportedFeatureException`
- `WrongThreadException`

### `geo`

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

### `json`

- `JsonValue`

### `log`

- `LogCallback`
- `LogEvent`
- `LogRecord`
- `LogSeverity`

### `map`

- `ConstrainMode`
- `DebugOption`
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

### `offline`

- `OfflineRegionDefinition`
- `OfflineRegionDownloadState`
- `OfflineRegionInfo`
- `OfflineRegionStatus`

### `query`

- `FeatureExtensionResult`
- `FeatureStateSelector`
- `QueriedFeature`
- `RenderedFeatureQueryOptions`
- `RenderedQueryGeometry`
- `SourceFeatureQueryOptions`

### `render`

- `FrameScope`
- `MetalBorrowedTextureDescriptor`
- `MetalContextDescriptor`
- `MetalOwnedTextureDescriptor`
- `MetalOwnedTextureFrame`
- `MetalOwnedTextureFrameHandle`
- `MetalSurfaceDescriptor`
- `NativeBuffer`
- `NativePointer`
- `PremultipliedRgba8Image`
- `RenderBackend`
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

### `resource`

- `ResourceErrorReason`
- `ResourceKind`
- `ResourceLoadingMethod`
- `ResourcePriority`
- `ResourceProviderCallback`
- `ResourceProviderDecision`
- `ResourceRequest`
- `ResourceRequestHandle`
- `ResourceResponse`
- `ResourceResponseStatus`
- `ResourceStoragePolicy`
- `ResourceTransformCallback`
- `ResourceTransformRequest`
- `ResourceUsage`

### `runtime`

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

### `style`

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

## Internal implementation inventory

Implement these Kotlin/Native support files:

| File or package                               | Purpose                                                                                                      |
| --------------------------------------------- | ------------------------------------------------------------------------------------------------------------ |
| `internal.c`                                  | Generated cinterop declarations.                                                                             |
| `internal.callback.LogCallbackState`          | Process-global logging callback `StableRef` state.                                                           |
| `internal.callback.ResourceTransformState`    | Runtime-scoped resource transform callback state.                                                            |
| `internal.callback.ResourceProviderState`     | Runtime-scoped resource provider callback state.                                                             |
| `internal.callback.CustomGeometrySourceState` | Map/style-scoped custom geometry callback state.                                                             |
| `internal.lifecycle.HandleState`              | Native pointer, closed state, parent retention, and leak context.                                            |
| `internal.loader`                             | Package reserved for future loader policy; current native tests link through Gradle and `MLN_FFI_BUILD_DIR`. |
| `internal.memory.MemoryUtil`                  | `memScoped`, `nativeHeap`, `ByteArray.usePinned`, UTF-8, string-view, array, and out-parameter helpers.      |
| `NativePointer` plus struct helpers           | Conversion between public opaque addresses and internal cinterop pointers.                                   |
| `internal.status.Status`                      | C status conversion, immediate diagnostic copying, and exception construction.                               |
| `internal.struct.CoreStructs`                 | Core copied values and temporary descriptor inputs.                                                          |
| `internal.struct.MapStructs`                  | Map, camera, bounds, geometry, JSON, and GeoJSON materialization.                                            |
| `internal.struct.QueryStructs`                | Query descriptors and copied query result readers.                                                           |
| `internal.struct.RenderStructs`               | Render descriptors, frames, image info, and native buffers.                                                  |
| `internal.struct.ResourceStructs`             | Resource request, response, and transform conversion.                                                        |
| `internal.struct.RuntimeStructs`              | Runtime options, events, and offline operation data.                                                         |
| `internal.struct.StyleStructs`                | Style source, image, layer, and custom geometry conversion.                                                  |
| `internal.struct.ValueStructs`                | JSON value-tree conversion and native snapshot copying.                                                      |

## Cinterop coverage map

Each C API function must have a Kotlin implementation or a recorded unsupported
reason before the binding leaves draft status. Kotlin code follows the headers
for signatures, ownership, out-parameters, and status behavior; this list is a
coverage inventory.

### Base and diagnostics

- `mln_c_version`
- `mln_supported_render_backend_mask`
- `mln_thread_last_error_message`

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

### Offline

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

### Map and camera

- `mln_map_options_default`
- `mln_map_create`
- `mln_map_request_repaint`
- `mln_map_request_still_image`
- `mln_map_destroy`
- `mln_map_set_style_url`
- `mln_map_set_style_json`
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

### Projection

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

### Query

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

### Render sessions, surfaces, and textures

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

### Style

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

## Test implementation map

Kotlin/Native tests exercise the public Kotlin API against the real C library.
They focus on adaptation invariants that cinterop cannot express.

Implemented host tests:

| Test                                        | Coverage                                                                                                      |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| `error.MaplibreExceptionTest`               | Stable Kotlin exception taxonomy.                                                                             |
| `internal.status.StatusTest`                | Status conversion and immediate diagnostic copying.                                                           |
| `internal.struct.MapStructsTest`            | Map, camera, viewport, bounds, and tile descriptor materialization.                                           |
| `internal.struct.ValueStructsTest`          | JSON, geometry, GeoJSON, and copied snapshot materialization.                                                 |
| `internal.struct.QueryStructsTest`          | Query descriptors plus copied query and feature-extension results.                                            |
| `internal.struct.RenderStructsTest`         | Metal/Vulkan descriptor pointer opacity and render image/frame readers.                                       |
| `internal.struct.RuntimeOfflineStructsTest` | Offline region snapshots, status, and list copying.                                                           |
| `json.JsonValueTest`                        | Kotlin JSON value invariants.                                                                                 |
| `geo.GeometryTest`                          | Geometry value invariants and collection depth validation.                                                    |
| `log.LogCallbackStateTest`                  | Log callback state, callback exception containment, and callback clearing.                                    |
| `map.CustomGeometrySourceStateTest`         | Custom geometry callback lifetime and exception containment.                                                  |
| `map.MapCameraControlsTest`                 | Camera, fit, viewport, coordinate, projection, and map-control wrappers.                                      |
| `map.MapHandleTest`                         | Map lifecycle, parent runtime retention, style setup, and close behavior.                                     |
| `map.MapProjectionHandleTest`               | Projection lifecycle and coordinate conversion wrappers.                                                      |
| `map.StyleHandleTest`                       | Style source, layer, image, tile source, image source, and specialized-layer wrappers.                        |
| `render.FrameScopeTest`                     | Frame-scope invalidation for owned texture frame views.                                                       |
| `render.NativeBufferTest`                   | Explicit native buffer capacity, close, and zero-length behavior.                                             |
| `resource.ResourceProviderStateTest`        | Resource provider callback state and one-shot request completion.                                             |
| `resource.ResourceTransformStateTest`       | Resource transform callback state and exception containment.                                                  |
| `runtime.RuntimeHandleTest`                 | Runtime lifecycle, event polling, resource transform/provider retention, and ambient-cache operation handles. |
| `runtime.RuntimeOfflineTest`                | Runtime network state and offline operation start/take-result wrappers.                                       |

Render attachment tests stay at descriptor, buffer, and frame-scope level on the
host target because real Metal/Vulkan attachment requires host graphics objects.
The C and renderer layers own graphics backend validation; Kotlin/Native tests
cover binding-owned lifetime and materialization behavior.

## Implementation milestones

1. Keep the proof slice green: Gradle host target, cinterop, nativeMain API, and
   `Maplibre.cVersion()`.
2. Add status conversion, diagnostic capture, and Kotlin exception taxonomy.
3. Add `HandleState`, `RuntimeHandle`, `MapHandle`, and `MapProjectionHandle`
   lifecycle.
4. Implement copied values, descriptors, enum conversions, JSON, geometry, and
   GeoJSON materializers.
5. Implement runtime event polling and copied event payloads.
6. Implement style, camera, query, and offline APIs.
7. Implement logging, resource transforms, resource providers, and one-shot
   request completion.
8. Implement render sessions, surface and texture descriptors, readback,
   `NativeBuffer`, and owned texture frame handles.
9. Add host Kotlin/Native tests and local task coverage.
10. Defer package publication, native-library distribution policy, examples,
    common facade work, and Android JNI support to future PRs.
11. Mark all in-scope items complete before changing the PR from draft to ready
    for review.

## Completion checklist

- [x] All public API inventory files exist under `org.maplibre.nativeffi` or
      have recorded replacements.
- [x] All internal implementation inventory files exist or have recorded
      replacements.
- [x] Every C API function listed in the cinterop coverage map has a Kotlin
      implementation or a recorded replacement.
- [x] Public nativeMain APIs expose no `kotlinx.cinterop` types, generated C
      types, `StableRef`, `NativePtr`, `CPointer`, `COpaquePointer`, `CValue`,
      `CValuesRef`, or `NativePlacement`.
- [x] `@OptIn(ExperimentalForeignApi::class)` stays in nativeMain
      implementations and internals, not in any future common facade.
- [x] Kotlin/Native tests pass on the supported host variant.
- [x] `mise run //bindings/kotlin-native:build` passes.
- [x] `./gradlew :bindings:kotlin-native:allTests` passes with linked test
      support.
- [x] The nativeMain API remains alignable with Java FFM and Java JNI so a
      future common facade can alias or expect/actual the platform
      implementations.
