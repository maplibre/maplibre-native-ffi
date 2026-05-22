# Java JNI Binding Implementation Map

## Audience and documentation role

Audience: contributors implementing and reviewing the Java JNI binding.
Category: explanation-backed reference. This document records the implementation
map and points to the design rules instead of restating them.

## Normative references

The implementation follows these documents. This spec links to them instead of
restating their rules.

- [Concepts](../../docs/src/content/docs/concepts.md): runtime, map, render
  session, events, ownership boundaries.
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md):
  status, diagnostics, callbacks, ABI ownership, and thread-affinity contract.
- [Binding conventions](../../docs/src/content/docs/development/bindings.md):
  shared handle, type, callback, rendering, and testing rules.
- [Rust binding conventions](../../docs/src/content/docs/development/bindings-rust.md):
  `maplibre-native-core` and bridge-crate responsibility split.
- [Java JNI binding conventions](../../docs/src/content/docs/development/bindings-java-jni.md):
  JNI-specific package, bridge, reference, exception, attachment, and Android
  rules.
- [Java FFM binding conventions](../../docs/src/content/docs/development/bindings-java-ffm.md):
  public Java API parity target.
- [Kotlin binding conventions](../../docs/src/content/docs/development/bindings-kotlin.md):
  Kotlin Multiplatform facade alignment target.
- [Rust bridge binding plan](../rust/PLAN.md): bridge boundary and shared-core
  review rules.

When this spec and a convention document appear to overlap, the convention
contains the rule and this spec names the concrete files, functions, and tests
that implement it. The Java FFM implementation is the public API parity source;
the public C headers are the C ABI source.

## Scope

`bindings/java-jni` is the low-level Java JNI binding for Android and JVMs where
Java FFM is unavailable or unsuitable. It mirrors `bindings/java-ffm` for
supported C ABI features unless this file records a JNI-only difference or
omission.

Parity is by reference: Java JNI follows the Java FFM source for public class
shapes, method signatures, visibility, nullability documentation, exception
behavior, and package organization. This spec does not duplicate every Java FFM
member. It records the file inventory and the JNI-specific glue needed to reach
that parity.

The package root is `org.maplibre.nativejni`. The Java module is
`org.maplibre.nativejni`. The native bridge crate is `maplibre-native-jni`.

## JNI differences and omissions

Record JNI-only differences here. Keep the `None` row only when Java JNI
intentionally mirrors Java FFM and the public C ABI for all supported features.

| Item                                         | Difference or omission                     | Reason                                                                                                                            | User-visible behavior                                                                                                                                                          | Tests/docs impact                                                                                                          |
| -------------------------------------------- | ------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| JVM native distribution packaging            | Out of scope for this implementation pass. | Existing bindings currently build, test, and support local examples without published per-platform native distribution artifacts. | JVM users load the locally built JNI bridge through `org.maplibre.nativejni.library.path`, `MAPLIBRE_NATIVE_JNI_LIBRARY_PATH`, or `System.loadLibrary("maplibre-native-jni")`. | Local JVM build, native build, and loader tests cover the supported path.                                                  |
| Android/AAR packaging and Android load tests | Out of scope for this implementation pass. | The repository does not yet define an Android packaging target or supported Android ABI test runner for this binding.             | Android artifacts are not produced by `bindings/java-jni`; Android load behavior is not claimed for this pass.                                                                 | The completion checklist records Android packaging and load tests as unsupported until an Android packaging target exists. |

## Current scaffold

```text
bindings/java-jni/
  SPEC.md
  build.gradle.kts
  mise.toml
  native/Cargo.toml
  native/src/lib.rs
  src/main/java/module-info.java
  src/main/java/org/maplibre/nativejni/Maplibre.java
  src/main/java/org/maplibre/nativejni/error/*.java
  src/main/java/org/maplibre/nativejni/render/NativePointer.java
  src/main/java/org/maplibre/nativejni/internal/bridge/NativeBridge.java
  src/main/java/org/maplibre/nativejni/internal/loader/NativeLibrary.java
```

This abbreviated tree omits package markers and `package-info.java`
placeholders. The scaffold implements one proof slice:

- `Maplibre.loadNativeLibrary()` loads `maplibre-native-jni`.
- `JNI_OnLoad` registers `NativeBridge.cVersion()` explicitly.
- `Maplibre.cVersion()` calls `mln_c_version()` through Rust JNI code.

## Build artifacts and tasks

Implement these artifacts:

| Artifact                | Path                       | Contents                                                                         |
| ----------------------- | -------------------------- | -------------------------------------------------------------------------------- |
| Java jar                | `bindings/java-jni`        | Public Java API, internal native declarations, loader, tests.                    |
| JNI bridge              | `bindings/java-jni/native` | Rust `cdylib` using `jni-rs`, `maplibre-native-core`, and `maplibre-native-sys`. |
| JVM native distribution | Out of scope               | Use the local native bridge build and loader paths for this pass.                |
| Android distribution    | Out of scope               | AAR-compatible native library layout requires a future Android packaging target. |

Implement these tasks:

| Task                                        | Required behavior                                       |
| ------------------------------------------- | ------------------------------------------------------- |
| `mise run //bindings/java-jni:build`        | Build Java sources and run Java checks.                 |
| `mise run //bindings/java-jni:native:build` | Build `maplibre-native-jni` after the C library exists. |
| `./gradlew :bindings:java-jni:javadoc`      | Validate public Javadocs.                               |
| `cargo check -p maplibre-native-jni`        | Type-check the Rust bridge crate.                       |
| `cargo test -p maplibre-native-jni`         | Run bridge unit tests once test modules exist.          |

Native library lookup stays implemented by `internal.loader.NativeLibrary`:

1. exact JNI bridge file from `org.maplibre.nativejni.library.path`;
2. exact JNI bridge file from `MAPLIBRE_NATIVE_JNI_LIBRARY_PATH`;
3. `System.loadLibrary("maplibre-native-jni")`.

## Java package map

Create these public packages and keep `module-info.java` exports in sync:

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

Create these internal packages. They remain unexported:

```text
org.maplibre.nativejni.internal.bridge
org.maplibre.nativejni.internal.callback
org.maplibre.nativejni.internal.lifecycle
org.maplibre.nativejni.internal.loader
org.maplibre.nativejni.internal.status
org.maplibre.nativejni.internal.struct
```

Add more internal packages only when their names identify a concrete role, for
example `internal.refs` or `internal.android`.

## Java FFM source parity inventory

Port or implement these Java FFM source files under the JNI package root. Class
and member visibility mirror Java FFM, so this inventory includes
package-private support files in exported packages, such as `render.FrameScope`,
`runtime.ResourceProviderState`, and `map.CustomGeometrySourceState`. File names
stay the same unless a JNI-only difference or omission is recorded above.

### Root

- `Maplibre.java`

### `camera`

- `AnimationOptions.java`
- `BoundOptions.java`
- `CameraFitOptions.java`
- `CameraOptions.java`
- `EdgeInsets.java`
- `FreeCameraOptions.java`
- `UnitBezier.java`

### `error`

- `InvalidArgumentException.java`
- `InvalidStateException.java`
- `MaplibreException.java`
- `MaplibreStatus.java`
- `NativeErrorException.java`
- `UnsupportedFeatureException.java`
- `WrongThreadException.java`

### `geo`

- `CanonicalTileId.java`
- `Feature.java`
- `FeatureIdentifier.java`
- `GeoJson.java`
- `Geometry.java`
- `LatLng.java`
- `LatLngBounds.java`
- `ProjectedMeters.java`
- `Quaternion.java`
- `ScreenBox.java`
- `ScreenPoint.java`
- `TileId.java`
- `Vec3.java`

### `json`

- `JsonValue.java`

### `log`

- `LogCallback.java`
- `LogEvent.java`
- `LogRecord.java`
- `LogSeverity.java`

### `map`

- `ConstrainMode.java`
- `CustomGeometrySourceState.java`
- `DebugOption.java`
- `MapHandle.java`
- `MapMode.java`
- `MapOptions.java`
- `MapProjectionHandle.java`
- `NorthOrientation.java`
- `ProjectionModeOptions.java`
- `RenderingStats.java`
- `TileLodMode.java`
- `TileOperation.java`
- `TileOptions.java`
- `ViewportMode.java`
- `ViewportOptions.java`

### `offline`

- `OfflineRegionDefinition.java`
- `OfflineRegionDownloadState.java`
- `OfflineRegionInfo.java`
- `OfflineRegionStatus.java`

### `query`

- `FeatureExtensionResult.java`
- `FeatureStateSelector.java`
- `QueriedFeature.java`
- `RenderedFeatureQueryOptions.java`
- `RenderedQueryGeometry.java`
- `SourceFeatureQueryOptions.java`

### `render`

- `FrameScope.java`
- `MetalBorrowedTextureDescriptor.java`
- `MetalContextDescriptor.java`
- `MetalOwnedTextureDescriptor.java`
- `MetalOwnedTextureFrame.java`
- `MetalOwnedTextureFrameHandle.java`
- `MetalSurfaceDescriptor.java`
- `NativeBuffer.java`
- `NativePointer.java`
- `PremultipliedRgba8Image.java`
- `RenderBackend.java`
- `RenderMode.java`
- `RenderSessionHandle.java`
- `RenderTargetExtent.java`
- `TextureImageInfo.java`
- `VulkanBorrowedTextureDescriptor.java`
- `VulkanContextDescriptor.java`
- `VulkanOwnedTextureDescriptor.java`
- `VulkanOwnedTextureFrame.java`
- `VulkanOwnedTextureFrameHandle.java`
- `VulkanSurfaceDescriptor.java`

### `resource`

- `ResourceErrorReason.java`
- `ResourceKind.java`
- `ResourceLoadingMethod.java`
- `ResourcePriority.java`
- `ResourceProviderCallback.java`
- `ResourceProviderDecision.java`
- `ResourceRequest.java`
- `ResourceRequestHandle.java`
- `ResourceResponse.java`
- `ResourceResponseStatus.java`
- `ResourceStoragePolicy.java`
- `ResourceTransformCallback.java`
- `ResourceTransformRequest.java`
- `ResourceUsage.java`

### `runtime`

- `AmbientCacheOperation.java`
- `NetworkStatus.java`
- `OfflineOperationHandle.java`
- `OfflineOperationKind.java`
- `OfflineOperationResultKind.java`
- `ResourceProviderState.java`
- `RuntimeEvent.java`
- `RuntimeEventPayload.java`
- `RuntimeEventSourceType.java`
- `RuntimeEventType.java`
- `RuntimeHandle.java`
- `RuntimeOptions.java`

### `style`

- `CustomGeometrySourceCallback.java`
- `CustomGeometrySourceOptions.java`
- `LocationIndicatorImageKind.java`
- `RasterDemEncoding.java`
- `SourceInfo.java`
- `SourceType.java`
- `StyleImage.java`
- `StyleImageInfo.java`
- `StyleImageOptions.java`
- `TileScheme.java`
- `TileSourceOptions.java`
- `VectorTileEncoding.java`

## Java internal and package-private implementation inventory

Implement these JNI equivalents of the FFM internals and package-private support
state:

| JNI file                                   | Purpose                                                                                                                                                                                                                                    | FFM analogue                                |
| ------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------- |
| `internal.bridge.NativeBridge`             | Generated or curated `native` declarations.                                                                                                                                                                                                | `internal.c` jextract output.               |
| `internal.bridge.*Native`                  | Concept-specific native declarations: `BaseNative`, `LogNative`, `RuntimeNative`, `MapNative`, `CameraNative`, `ProjectionNative`, `QueryNative`, `RenderSessionNative`, `SurfaceNative`, `TextureNative`, `StyleNative`, `OfflineNative`. | `internal.c.MapLibreNativeC`.               |
| `internal.callback.LogCallbackState`       | Process-global log callback global-reference state.                                                                                                                                                                                        | `internal.callback.LogCallbackState`.       |
| `internal.callback.ResourceTransformState` | Runtime-scoped resource transform callback state.                                                                                                                                                                                          | `internal.callback.ResourceTransformState`. |
| `runtime.ResourceProviderState`            | Package-private runtime-scoped resource provider callback state.                                                                                                                                                                           | `runtime.ResourceProviderState`.            |
| `map.CustomGeometrySourceState`            | Package-private map/style-scoped custom geometry callback state.                                                                                                                                                                           | `map.CustomGeometrySourceState`.            |
| `internal.lifecycle.HandleState`           | Java-side native pointer, release state, parent retention, leak reporting.                                                                                                                                                                 | `internal.lifecycle.HandleState`.           |
| `internal.loader.NativeLibrary`            | JNI bridge library loading.                                                                                                                                                                                                                | `internal.loader.NativeLibrary`.            |
| `internal.status.Status`                   | Status-to-exception conversion and diagnostic capture.                                                                                                                                                                                     | `internal.status.Status`.                   |
| `internal.struct.CoreStructs`              | Core copied values and temporary descriptor inputs.                                                                                                                                                                                        | `internal.struct.CoreStructs`.              |
| `internal.struct.MapStructs`               | Map, camera, bounds, geometry, JSON, and GeoJSON materialization.                                                                                                                                                                          | `internal.struct.MapStructs`.               |
| `internal.struct.QueryStructs`             | Query descriptors and copied query result readers.                                                                                                                                                                                         | `internal.struct.QueryStructs`.             |
| `internal.struct.RenderStructs`            | Render descriptors, frames, image info, native buffers.                                                                                                                                                                                    | `internal.struct.RenderStructs`.            |
| `internal.struct.ResourceStructs`          | Resource request/response/transform conversion.                                                                                                                                                                                            | `internal.struct.ResourceStructs`.          |
| `internal.struct.RuntimeStructs`           | Runtime options, events, offline operation data.                                                                                                                                                                                           | `internal.struct.RuntimeStructs`.           |
| `internal.struct.StyleStructs`             | Style source, image, layer, and custom geometry conversion.                                                                                                                                                                                | `internal.struct.StyleStructs`.             |
| `internal.struct.ValueStructs`             | JSON value-tree conversion and native snapshot copying.                                                                                                                                                                                    | `internal.struct.ValueStructs`.             |

## Rust bridge crate inventory

Implement these Rust modules in `bindings/java-jni/native/src`:

| Module                            | Contents                                                                      |
| --------------------------------- | ----------------------------------------------------------------------------- |
| `lib.rs`                          | `JNI_OnLoad`, top-level registration, panic boundary.                         |
| `jvm.rs`                          | `JavaVM` storage, thread attach/detach helpers, Android class-loader support. |
| `registration.rs`                 | Class lookup and `JNINativeMethod` registration tables.                       |
| `classes.rs`                      | Cached class, constructor, method, and field IDs.                             |
| `errors.rs`                       | Java exception creation from `maplibre-native-core` errors.                   |
| `strings.rs`                      | Java string to standard UTF-8 conversion, embedded-NUL rejection.             |
| `refs.rs`                         | Local frame, global reference, weak-global reference helpers.                 |
| `handles.rs`                      | Pointer boxing/unboxing, closed-state interop, handle release helpers.        |
| `values.rs`                       | Java records/enums/JSON/geometry construction helpers.                        |
| `callbacks/logging.rs`            | Log callback trampoline and global state.                                     |
| `callbacks/resource_transform.rs` | Resource transform trampoline and response storage.                           |
| `callbacks/resource_provider.rs`  | Resource provider trampoline and request handles.                             |
| `callbacks/custom_geometry.rs`    | Custom geometry trampoline and active-upcall accounting.                      |
| `base.rs`                         | C version, supported render backends, diagnostics.                            |
| `runtime.rs`                      | Runtime, events, resource provider/transform, ambient cache.                  |
| `offline.rs`                      | Offline operation start/take/discard and copied results.                      |
| `map.rs`                          | Map lifecycle, style loading, repaint/still-image requests.                   |
| `camera.rs`                       | Camera, viewport, tile, bounds, debug, and projection-mode calls.             |
| `projection.rs`                   | `MapProjectionHandle` calls and coordinate conversions.                       |
| `query.rs`                        | Rendered/source/extension queries and copied result readers.                  |
| `render_session.rs`               | Session lifecycle, feature state, JSON snapshots.                             |
| `surface.rs`                      | Metal/Vulkan surface attachment.                                              |
| `texture.rs`                      | Metal/Vulkan texture attachment, readback, frame acquire/release.             |
| `style.rs`                        | Sources, layers, images, light, properties, filters, custom geometry.         |

Bridge code may call `maplibre-native-sys` directly only for JNI trampoline glue
or missing `maplibre-native-core` adapters. If two bridge modules need the same
`sys` sequence, move it into `maplibre-native-core`.

## Native method coverage map

The public C headers define ABI signatures, ownership, out-parameters, and
status behavior. This map is a coverage inventory, not a duplicate signature
specification. JNI native declarations and Rust bridge calls follow the headers
and the binding conventions by reference.

Each C API function listed below must have a JNI implementation or a recorded
unsupported reason before the binding leaves draft status. Reviewers may update
this list while comparing it with the headers, but this task does not require a
mechanical header-sync check or generator.

### `BaseNative`

- `mln_c_version`
- `mln_supported_render_backend_mask`
- `mln_thread_last_error_message` (`internal` through status conversion)

### `LogNative`

- `mln_log_set_callback`
- `mln_log_clear_callback`
- `mln_log_set_async_severity_mask`

### `RuntimeNative`

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

### `OfflineNative`

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

### `MapNative`

- `mln_map_options_default`
- `mln_map_create`
- `mln_map_request_repaint`
- `mln_map_request_still_image`
- `mln_map_destroy`
- `mln_map_set_style_url`
- `mln_map_set_style_json`

### `CameraNative`

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

### `ProjectionNative`

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

### `QueryNative`

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

### `RenderSessionNative`

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

### `SurfaceNative`

- `mln_metal_surface_descriptor_default`
- `mln_vulkan_surface_descriptor_default`
- `mln_metal_surface_attach`
- `mln_vulkan_surface_attach`

### `TextureNative`

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

### `StyleNative`

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

Port these Java FFM tests to `bindings/java-jni/src/test/java` and keep their
assertions focused on Java boundary behavior. Tests cover binding-owned logic:
library loading, status conversion, handle state, string and buffer validation,
callback bridging, JNI reference/thread behavior, and descriptor
materialization. They avoid exhaustive retesting of C API state, range, and
rendering semantics. Kotlin common facade compilation is the broader API parity
check when that facade is available; do not add a reflection parity test for
this task.

- `MaplibreTest`
- `internal.callback.LogCallbackStateTest`
- `internal.callback.ResourceTransformStateTest`
- `internal.loader.NativeLibraryTest`
- `internal.status.StatusAndMemoryTest`
- `internal.struct.RenderStructsTest`
- `internal.struct.ValueStructsTest`
- `map.MapHandleTest`
- `map.StyleHandleTest`
- `render.RenderSessionHandleTest`
- `render.RenderSessionQueryTest`
- `resource.ResourceRequestHandleTest`
- `runtime.ResourceProviderStateTest`
- `runtime.RuntimeHandleTest`
- `runtime.RuntimeOfflineTest`
- `test.NativeTestSupport`
- `test.RenderTargetTestSupport`

Add JNI-specific tests:

| Test                                      | Coverage                                                                    |
| ----------------------------------------- | --------------------------------------------------------------------------- |
| `internal.bridge.NativeRegistrationTest`  | `JNI_OnLoad` explicit registration and artifact mismatch failures.          |
| `internal.bridge.PanicBoundaryTest`       | Rust panic containment before JNI return.                                   |
| `internal.refs.GlobalReferenceTest`       | Callback global references release exactly once.                            |
| `internal.refs.LocalFrameTest`            | Loops creating Java objects use bounded local references.                   |
| `internal.strings.StandardUtf8Test`       | Standard UTF-8 conversion and modified-UTF-8 avoidance.                     |
| `internal.jvm.NativeThreadAttachmentTest` | Attach/detach behavior for MapLibre-created threads.                        |
| `android.AndroidLoadTest`                 | Android library loading and class-loader behavior, where Android tests run. |

## Implementation milestones

1. Replace package markers with the full public Java parity inventory.
2. Complete the bridge proof slice: ABI version, supported backends, network
   status get/set, status-to-exception conversion, and diagnostic capture.
3. Add Java native declarations and Rust registration scaffolding for all C ABI
   functions in the coverage map.
4. Implement `internal.lifecycle.HandleState` and runtime/map/projection handle
   lifecycle.
5. Implement copied values, descriptors, enum conversions, JSON, geometry, and
   GeoJSON materializers.
6. Implement runtime event polling and copied event payloads.
7. Implement style, camera, query, and offline APIs.
8. Implement resource transforms, resource providers, and one-shot request
   completion.
9. Implement render sessions, surface and texture descriptors, readback,
   `NativeBuffer`, and owned texture frame handles.
10. Verify local JVM native loading/build integration parity and record
    packaging omissions.
11. Mark all in-scope items complete before changing the PR from draft to ready
    for review.

## Completion checklist

- [ ] All Java FFM source parity inventory files exist under
      `org.maplibre.nativejni` with matching Java FFM visibility.
- [ ] All internal and package-private implementation inventory files exist or
      have recorded replacements.
- [ ] Every C API function listed in the native method coverage map has a JNI
      implementation or a recorded unsupported reason in the differences table.
- [ ] Java JNI tests pass on supported JVM host variants.
- [ ] JVM native distribution packaging and Android/AAR packaging/load tests are
      recorded as out of scope until packaging targets exist.
- [ ] `mise run //bindings/java-jni:build` passes.
- [ ] `mise run //bindings/java-jni:native:build` passes.
- [ ] `cargo test -p maplibre-native-jni` passes.
- [ ] `./gradlew :bindings:java-jni:javadoc` passes.
- [ ] Kotlin common facade compatibility has been checked against the Kotlin
      binding conventions when the facade is available.
- [ ] Local JVM native loading works through the documented loader paths.
