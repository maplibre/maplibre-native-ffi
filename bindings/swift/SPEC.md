# Swift Binding Implementation Map

## Audience and documentation role

Audience: contributors implementing and reviewing the Swift binding. Category:
reference with a short explanatory map. This document names the concrete files,
modules, tasks, and coverage targets for the binding. The convention documents
remain the source of design rules.

## Normative references

The implementation follows these documents. This spec links to them instead of
restating their rules.

- [Concepts](../../docs/src/content/docs/concepts.md): runtime, map, render
  session, events, ownership boundaries.
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md):
  status, diagnostics, callbacks, ABI ownership, and thread-affinity contract.
- [Binding conventions](../../docs/src/content/docs/development/bindings.md):
  shared handle, type, callback, rendering, and testing rules.
- [Swift binding conventions](../../docs/src/content/docs/development/bindings-swift.md):
  direct C import, ARC handle policy, errors, concurrency, callbacks, and render
  target rules.

When this spec and a convention document appear to overlap, the convention
contains the rule and this spec names the concrete Swift implementation points.
The public C headers are the ABI source. Existing direct-C bindings and examples
may be consulted for coverage and expected ABI behavior, but Swift imports the C
API directly and does not use a bridge crate.

## Scope

`bindings/swift` is the low-level Swift binding over the public C API. It uses
Swift's C importer directly and exposes one public Swift package product:
`MaplibreNative`.

The package stays close to the C API's runtime, map, render session, event,
resource, query, style, and rendering model. It adapts ownership, diagnostics,
callbacks, copied values, and native backend handles. Higher-level SwiftUI,
UIKit, AppKit, async, actor, and view-lifecycle adapters belong above this
package.

The public Swift module is `MaplibreNative`. The private C importer module is
`CMaplibreNativeC`. The internal support module is `MaplibreNativeSupport`. Only
`MaplibreNative` is a package product.

## Current implementation

```text
bindings/swift/
  SPEC.md
  Package.swift
  mise.toml
  Sources/CMaplibreNativeC/
    module.modulemap
  Sources/MaplibreNativeSupport/
    CAPI.swift
    LoggingCallbacks.swift
    MapStructs.swift
    NativeDescriptor.swift
    NativeHandleLeakReporter.swift
    NativeHandleState.swift
    NativeMemory.swift
    NativeResultGuard.swift
    NativeStatus.swift
    NativeString.swift
    OfflineStructs.swift
    QueryStructs.swift
    RenderStructs.swift
    ResourceCallbacks.swift
    RuntimeStructs.swift
    StyleStructs.swift
    ValueStructs.swift
  Sources/MaplibreNative/
    Camera.swift
    CameraAdvanced.swift
    Geometry.swift
    Handles.swift
    Logging.swift
    Map.swift
    Maplibre.swift
    MaplibreError.swift
    NativePointer.swift
    NetworkStatus.swift
    Offline.swift
    Projection.swift
    Query.swift
    Render.swift
    RenderBackend.swift
    Resource.swift
    Runtime.swift
    Style.swift
    Values.swift
  Tests/MaplibreNativeTests/
    CameraAdvancedTests.swift
    LoggingTests.swift
    MapHandleTests.swift
    MaplibreTests.swift
    OfflineTests.swift
    ProjectionTests.swift
    QueryTests.swift
    RenderTests.swift
    RuntimeTests.swift
    StyleTests.swift
    SupportHelperTests.swift
    ValueTests.swift
```

The implementation covers the full C API map below. The public module exposes
Swift descriptors, copied value types, handle classes, and callback closures;
raw imported C symbols stay inside `MaplibreNativeSupport`.

## Build artifacts and tasks

| Artifact          | Path                            | Contents                                                                         |
| ----------------- | ------------------------------- | -------------------------------------------------------------------------------- |
| Swift package     | `bindings/swift`                | Public Swift binding, private C importer, internal support, tests.               |
| C importer module | `Sources/CMaplibreNativeC`      | SwiftPM system library target that imports `include/maplibre_native_c.h`.        |
| Support module    | `Sources/MaplibreNativeSupport` | C calls, status capture, handle state, descriptor materializers, callback boxes. |
| Public module     | `Sources/MaplibreNative`        | Handles, values, descriptors, errors, callbacks, and backend interop values.     |

Implemented tasks:

| Task                                                 | Required behavior                                          |
| ---------------------------------------------------- | ---------------------------------------------------------- |
| `mise run //bindings/swift:build`                    | Build the Swift package after the native C library exists. |
| `mise run //bindings/swift:test`                     | Run Swift tests against the real C library.                |
| `mise run //bindings/swift:ci`                       | Run tests and validate SwiftPM package metadata.           |
| `swift build --scratch-path .build/$MLN_FFI_VARIANT` | Build the package from `bindings/swift`.                   |
| `swift test --scratch-path .build/$MLN_FFI_VARIANT`  | Run package tests from `bindings/swift`.                   |

The Swift package links `maplibre-native-c` from `MLN_FFI_BUILD_DIR` and embeds
that directory as an rpath for local tests and examples.

## Module responsibilities

### `CMaplibreNativeC`

The C target imports the public umbrella header through SwiftPM's system library
mechanism. It has no public package product. Code outside
`MaplibreNativeSupport` does not import it.

### `MaplibreNativeSupport`

The support module owns raw C interaction:

- direct calls to imported `mln_*` functions;
- immediate thread-local diagnostic capture after non-OK statuses;
- conversion from imported C enums and structs into Swift-friendly raw values;
- native handle state and close-once helpers;
- descriptor materializers and copied-result readers;
- `@convention(c)` callback trampolines and retained Swift callback boxes.

Support declarations use the narrowest access that Swift module boundaries
allow. Symbols consumed by `MaplibreNative` may be `public`, but the package
does not vend this module as a product. Types from this module do not appear in
public `MaplibreNative` signatures.

### `MaplibreNative`

The public module owns Swift API policy:

- final `*Handle` classes with explicit `close() throws`;
- Swift value types for copied C data;
- descriptors with semantic fields and internal C materialization;
- `MaplibreError` and `throws` status reporting;
- `OptionSet`, enum, and unknown-value mappings;
- `NativePointer` as a borrowed opaque address value;
- callback protocols, closures, and resource request handle APIs;
- non-`Sendable` owner-thread handle classes and `Sendable` copied values.

## Swift public API inventory

Create or complete these public source areas. File names may split further when
the split improves locality, but module-level concepts stay stable.

| Swift area            | Public surface                                                                                                                      |
| --------------------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| `Maplibre.swift`      | Process-global entry points: ABI version, supported backends, network status, logging configuration, coordinate projection helpers. |
| `MaplibreError.swift` | `MaplibreError`, `MaplibreErrorKind`, raw status, copied diagnostic.                                                                |
| `NativePointer.swift` | Borrowed opaque backend pointer value.                                                                                              |
| `Logging.swift`       | `LogCallback`, `LogRecord`, `LogSeverity`, `LogEvent`, process-global log callback registration.                                    |
| `Runtime.swift`       | `RuntimeHandle`, `RuntimeOptions`, events, resource provider/transform state, offline operation handles.                            |
| `Map.swift`           | `MapHandle`, `MapOptions`, map lifecycle, style loading, debug, custom geometry state.                                              |
| `Camera.swift`        | Camera, animation, bounds, viewport, tile, projection-mode descriptors and map camera operations.                                   |
| `Projection.swift`    | `MapProjectionHandle` and coordinate conversion helpers.                                                                            |
| `Geometry.swift`      | Lat/lng, screen, tile, vector, bounds, quaternion, JSON, GeoJSON, and feature value types.                                          |
| `Query.swift`         | Rendered/source query descriptors, query geometries, queried features, extension results.                                           |
| `Render.swift`        | `RenderSessionHandle`, render modes, render target descriptors, native buffers, images, texture frames.                             |
| `Resource.swift`      | Resource request, response, transform, provider decision, one-shot request handle.                                                  |
| `Style.swift`         | Source, layer, image, light, property, filter, and custom geometry source APIs.                                                     |
| `Handles.swift`       | Internal base classes or boxes shared by public final handle classes.                                                               |

## Public type map

| C or shared concept                 | Swift type shape                                                                                                                      |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `mln_runtime*`                      | `final class RuntimeHandle` with `close() throws`; retains callback boxes and map registry.                                           |
| `mln_map*`                          | `final class MapHandle`; retains `RuntimeHandle`.                                                                                     |
| `mln_map_projection*`               | `final class MapProjectionHandle`; standalone snapshot after creation.                                                                |
| `mln_render_session*`               | `final class RenderSessionHandle`; retains `MapHandle`.                                                                               |
| `mln_resource_request_handle*`      | `final class ResourceRequestHandle`; `Sendable` only if synchronized and completion is C-permitted from any thread.                   |
| Session-owned texture frame handles | `final class MetalOwnedTextureFrameHandle` and `VulkanOwnedTextureFrameHandle`; scoped view access to backend pointers.               |
| C option structs                    | Swift descriptor structs; materializers set `size`, masks, pointers, and nested storage.                                              |
| C field masks                       | Swift optionals, explicit clear methods, or small presence wrappers; C masks stay internal.                                           |
| Closed enum domains                 | Swift enums with explicit raw conversion helpers.                                                                                     |
| Drift-prone output domains          | Swift enums with `unknown(UInt32)` or `unknown(Int32)`.                                                                               |
| C bit masks                         | Swift `OptionSet` types.                                                                                                              |
| Native result/list/snapshot handles | Internal guards that copy into Swift values before release.                                                                           |
| Opaque backend `void*` fields       | `NativePointer`; no ownership or memory access.                                                                                       |
| CPU images and resource bytes       | `Data` for copied byte payloads, making Foundation part of the public surface; readback also offers explicit mutable storage helpers. |

## Internal implementation inventory

`Sources/MaplibreNativeSupport` contains the raw C boundary:

| File                             | Contents                                                                                      |
| -------------------------------- | --------------------------------------------------------------------------------------------- |
| `CAPI.swift`                     | Thin curated functions that call imported C symbols and return Swift raw values.              |
| `NativeStatus.swift`             | Status checking, diagnostic capture, native failure payloads.                                 |
| `NativeHandleState.swift`        | Pointer storage, released state, close-once behavior, and leak reporting hook.                |
| `NativeHandleLeakReporter.swift` | Leak reporting utility for native handle boxes.                                               |
| `NativeString.swift`             | UTF-8 and string-view storage, embedded-NUL rejection for C string inputs.                    |
| `NativeMemory.swift`             | Scoped temporary storage helpers for arrays, bytes, out-pointers, and descriptor graphs.      |
| `NativeDescriptor.swift`         | Scoped native pointer materialization for backend descriptors.                                |
| `NativeResultGuard.swift`        | Exactly-once release helper for result handles.                                               |
| `MapStructs.swift`               | Core coordinates plus map, camera, bounds, viewport, tile, and projection-mode materializers. |
| `RuntimeStructs.swift`           | Runtime options, copied runtime events, and offline event payloads.                           |
| `OfflineStructs.swift`           | Offline region definitions and copied region results.                                         |
| `QueryStructs.swift`             | Query descriptors, feature-state selectors, and copied query result readers.                  |
| `RenderStructs.swift`            | Render target descriptors, native buffers, texture frames, and readback helpers.              |
| `ResourceCallbacks.swift`        | Runtime resource transform/provider trampolines and request handle state.                     |
| `LoggingCallbacks.swift`         | Process-global logging trampoline and callback state.                                         |
| `StyleStructs.swift`             | Style source, image, layer, light, and custom geometry conversion and callback retention.     |
| `ValueStructs.swift`             | JSON, GeoJSON, feature, geometry, and property value conversion.                              |

## C API coverage map

Every public C function listed here has a Swift implementation in
`MaplibreNative` or an internal support implementation used by public Swift
APIs. No coverage item currently needs an unsupported-reason entry. Reviewers
compare this list with `include/maplibre_native_c/*.h` during coverage reviews.

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

### Map lifecycle and style loading

- `mln_map_options_default`
- `mln_map_create`
- `mln_map_request_repaint`
- `mln_map_request_still_image`
- `mln_map_destroy`
- `mln_map_set_style_url`
- `mln_map_set_style_json`

### Camera and map options

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

### Render session

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

### Surface targets

- `mln_metal_surface_descriptor_default`
- `mln_vulkan_surface_descriptor_default`
- `mln_metal_surface_attach`
- `mln_vulkan_surface_attach`

### Texture targets

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

## Swift example migration target

`examples/swift-map` depends on the `MaplibreNative` product and no longer
defines a raw C importer target. The example uses `RuntimeHandle`, `MapHandle`,
`RenderSessionHandle`, Swift camera descriptors, and Swift render descriptors.
AppKit, SwiftUI, Metal layer ownership, timers, and input policy remain in the
example because they stay outside the low-level binding.

## Testing map

Swift tests exercise the public Swift API against the real C library. They focus
on Swift-owned behavior:

- status-to-`MaplibreError` conversion and immediate diagnostic copying;
- explicit close idempotence and failed-close retry behavior;
- parent retention while child handles are live;
- non-`Sendable` owner-thread handles and `Sendable` copied values;
- embedded-NUL rejection and scoped UTF-8 storage;
- callback box synchronization and exactly-once release;
- copied runtime events and query/style result payloads;
- frame handle scoped-view invalidation and active-state invalidation.

The current test suite covers the proof slice, support helpers, logging,
runtime/resource callbacks, map/camera/projection controls, render targets,
query/style/value conversion, custom geometry callbacks, and offline result
copying. Render readback is exposed as caller-owned mutable storage and relies
on the C ABI tests for native pixel-copy validation. Tests focus on
binding-owned behavior instead of retesting all native C validation rules.

## Implementation milestones

1. Complete: proof slice APIs stay green: ABI version, supported backends,
   network status, `MaplibreError`, and `NativePointer`.
2. Complete: shared support helpers cover string storage, temporary memory,
   descriptor materialization, copied-result guards, and native handle state
   leak reporting.
3. Complete: process-global logging wraps C callbacks and retains callback
   state.
4. Complete: runtime creation, pumping, event polling, resource transforms, and
   resource providers are implemented.
5. Complete: map creation, style loading, camera descriptors, and the camera API
   used by `examples/swift-map` are public Swift APIs.
6. Complete: `MapProjectionHandle` and projection helper functions are
   implemented.
7. Complete: render sessions, Metal/Vulkan surface descriptors, texture targets,
   readback, and frame handles are implemented.
8. Complete: query, style, JSON, GeoJSON, image, and custom geometry APIs are
   implemented.
9. Complete: offline operation start/take/discard flows and copied offline
   results are implemented.
10. Complete: `examples/swift-map` builds against `MaplibreNative` instead of
    raw C imports.
11. Complete: the coverage map has no missing Swift implementation entries.

## Completion checklist

- [x] `MaplibreNative` exposes no imported C symbols, raw C structs, field
      masks, or callback trampolines in public API.
- [x] Every long-lived native object has a final `*Handle` class with
      `close() throws` and documented owner-thread behavior.
- [x] Every C function in the coverage map has a Swift implementation or a
      recorded unsupported reason.
- [x] Native failures map to `MaplibreError` with raw status and copied
      diagnostics.
- [x] Swift descriptors own semantic fields; support materializers write C
      `size` and mask fields.
- [x] Callback boxes release exactly once after their C owner scope and active
      upcalls finish.
- [x] Session-owned texture frame values reject use after frame close.
- [x] `examples/swift-map` builds against `MaplibreNative` instead of raw C
      imports.
- [x] `mise run //bindings/swift:build` passes.
- [x] `mise run //bindings/swift:test` passes.
- [x] `mise run //bindings/swift:ci` passes.
