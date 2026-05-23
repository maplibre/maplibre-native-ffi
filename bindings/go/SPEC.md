# Go Binding Implementation Map

## Audience and documentation role

Audience: contributors implementing and reviewing the Go binding. Category:
reference with a short explanatory map. This document names the concrete files,
packages, tasks, and coverage targets for the binding. The convention documents
remain the source of design rules.

## Normative references

The implementation follows these documents. This spec links to them instead of
restating their rules.

- [Concepts](../../docs/src/content/docs/concepts.md): runtime, map, render
  session, events, and ownership boundaries.
- [C API conventions](../../docs/src/content/docs/development/c-conventions.md):
  status, diagnostics, callbacks, ABI ownership, and thread-affinity contract.
- [Binding conventions](../../docs/src/content/docs/development/bindings.md):
  shared handle, type, callback, rendering, and testing rules.
- [Go binding conventions](../../docs/src/content/docs/development/bindings-go.md):
  cgo architecture, Go handle policy, OS-thread affinity, memory, callbacks, and
  render target rules.
- [Rust binding conventions](../../docs/src/content/docs/development/bindings-rust.md):
  existing low-level binding coverage and safe wrapper inventory.
- [Java FFM binding conventions](../../docs/src/content/docs/development/bindings-java-ffm.md):
  direct-C binding structure, public concept coverage, and generated-C boundary
  separation.

When this spec and a convention document appear to overlap, the convention
contains the rule and this spec names the concrete Go implementation points. The
public C headers are the ABI source. Existing Rust and Java FFM bindings are
coverage references; Go imports the C API directly through cgo and does not use
the Rust bridge layer.

## Scope

`bindings/go` is the low-level Go binding over the public MapLibre Native C API.
It exposes one public Go package, `maplibre`, backed by private `internal/*`
packages. The public package stays close to the C API's runtime, map, render
session, event, resource, query, style, and rendering model while adapting
ownership, diagnostics, callbacks, copied values, slices, and backend-native
handles to Go.

Go applications call methods on explicit `*Handle` structs and close them with
`Close() error`. The binding does not dispatch ordinary calls to another thread.
Callers that need deterministic owner-thread affinity use `runtime.LockOSThread`
or a future small owner-loop helper above the ordinary handle methods.
Higher-level goroutine schedulers, UI integration, async resource pipelines, and
render abstractions belong above this package.

## Current implementation

`bindings/go` contains the public `maplibre` package, focused Go tests, and
private `internal/*` packages for cgo calls, callbacks, handle state, memory,
status, and copied-structure helpers. The exact file list may change as the
binding evolves; the package inventory below defines the required areas.

The implementation covers the spec inventory:

- `maplibre.CVersion()` calls `mln_c_version()` through private cgo code.
- `maplibre.SupportedRenderBackends()` preserves backend mask bits in
  `RenderBackendMask`.
- `maplibre.CurrentNetworkStatus()` and `maplibre.SetNetworkStatus()` cross the
  real C ABI and translate native status failures into Go errors.
- `Error` wraps stable sentinel categories so `errors.Is` works and callers can
  inspect raw native status and copied diagnostics.
- `NativePointer` is a borrowed opaque address value with no memory access in
  public API.
- `internal/status.CheckCall` locks the current OS thread around a fallible C
  call and immediate diagnostic capture.
- Runtime, map, projection, render, texture, query, style, resource, offline,
  callback, JSON, GeoJSON, camera, viewport, and coordinate conversion APIs have
  public Go wrappers backed by private cgo calls.

## Build artifacts and tasks

| Artifact         | Path            | Contents                                                                  |
| ---------------- | --------------- | ------------------------------------------------------------------------- |
| Go module        | `bindings/go`   | Public Go package, private cgo/support packages, and tests.               |
| cgo layer        | `internal/capi` | Private direct calls to `include/maplibre_native_c.h`.                    |
| support packages | `internal/*`    | Status conversion, handle state, memory helpers, callback state, readers. |

Implemented tasks:

| Task                           | Required behavior                                      |
| ------------------------------ | ------------------------------------------------------ |
| `mise run //bindings/go:build` | Run `go test ./...` against the real native C library. |
| `mise run //bindings/go:test`  | Run the Go tests against the real native C library.    |
| `mise run //bindings/go:ci`    | Check `gofmt`, run tests, and run `go vet ./...`.      |

The mise tasks provide `CGO_CFLAGS`, `CGO_LDFLAGS`, and an rpath for the native
library in `MLN_FFI_BUILD_DIR`. Package consumers provide the platform C
library; packaging and native library distribution stay outside this binding
spec.

## Package responsibilities

### `maplibre`

The public package owns Go API policy:

- explicit `*Handle` structs with `Close() error`;
- Go values, descriptors, slices, enums, bit masks, callbacks, and errors;
- `Error` plus stable category sentinels such as `ErrInvalidArgument` and
  `ErrWrongThread`;
- `NativePointer` as a borrowed opaque backend address;
- public methods that preserve caller execution on the current goroutine and OS
  thread.

Public signatures do not expose `C.*` types, `unsafe.Pointer`, raw field masks,
callback trampolines, or internal package types. Unsafe backend interop stays
limited to APIs whose C contract accepts opaque backend handles.

### `internal/capi`

The cgo package owns most raw C interaction:

- direct wrappers for ordinary `mln_*` functions;
- raw C status constants and enum/mask constants needed by support code;
- narrow wrappers that copy primitive outputs into Go-owned storage;
- no public package product outside the module's `internal` boundary.

`internal/callback` is the deliberate exception for callback-owner APIs and
trampolines whose C function pointer types, `runtime/cgo.Handle` tokens, and
release hooks need to live beside the exported trampoline functions.

### `internal/status`

The status package owns same-thread status handling. Fallible public APIs call
`status.CheckCall`, which locks the current OS thread, invokes the cgo wrapper,
then copies `mln_thread_last_error_message()` before unlocking when the status
is non-OK.

### Other internal packages

| Package             | Purpose                                                                         |
| ------------------- | ------------------------------------------------------------------------------- |
| `internal/handle`   | Close-once native handle state, parent retention bookkeeping, leak reporting.   |
| `internal/memory`   | cgo memory helpers for UTF-8, string views, slices, pinned data, C allocation.  |
| `internal/structs`  | Descriptor materializers and copied result/list/snapshot readers.               |
| `internal/callback` | cgo trampolines, `runtime/cgo.Handle` tokens, panic recovery, callback release. |

## Go public API inventory

Create or complete these public source areas. File names may split further when
the split improves locality, but the single public package stays stable.

| Go area         | Public surface                                                                                                                     |
| --------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `maplibre.go`   | Process-global entry points: ABI version, supported backends, coordinate projection helpers.                                       |
| `errors.go`     | `Error`, raw status inspection, copied diagnostics, and stable error sentinels.                                                    |
| `runtime.go`    | `RuntimeHandle`, `RuntimeOptions`, event polling, network status, resource provider/transform state, offline ops.                  |
| `map.go`        | `MapHandle`, `MapOptions`, map lifecycle, style loading, debug, custom geometry state.                                             |
| `camera.go`     | Camera, animation, bounds, viewport, tile, and projection-mode descriptors and operations.                                         |
| `projection.go` | `MapProjectionHandle` and coordinate conversion helpers.                                                                           |
| `geometry.go`   | Lat/lng, screen, tile, vector, bounds, quaternion, JSON, GeoJSON, and feature value types.                                         |
| `query.go`      | Rendered/source query descriptors, query geometries, queried features, extension results.                                          |
| `render.go`     | `RenderSessionHandle`, render modes, render target descriptors, backend context/native handle descriptors, images, texture frames. |
| `resource.go`   | Resource request, response, transform, provider decision, one-shot request handle.                                                 |
| `style.go`      | Source, layer, image, light, property, filter, and custom geometry source APIs.                                                    |
| `logging.go`    | `LogCallback`, `LogEvent`, `LogRecord`, `LogSeverity`, `LogSeverityMask`, process-global callback registration.                    |
| `handles.go`    | Shared public handle documentation or small helper interfaces when needed.                                                         |

JSON and GeoJSON values copy into owned Go value trees. Public JSON objects use
`map[string]any`, so duplicate object keys collapse and member iteration order
is not stable; scalar conversions preserve integer width where the Go value
carries that width.

## Public type map

| C or shared concept                 | Go type shape                                                                                                                                                                                                                                    |
| ----------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `mln_runtime*`                      | `type RuntimeHandle struct` with private state and `Close() error`; retains runtime-owned callback state.                                                                                                                                        |
| `mln_map*`                          | `type MapHandle struct`; retains `*RuntimeHandle`.                                                                                                                                                                                               |
| `mln_map_projection*`               | `type MapProjectionHandle struct`; standalone snapshot after creation.                                                                                                                                                                           |
| `mln_render_session*`               | `type RenderSessionHandle struct`; retains `*MapHandle`.                                                                                                                                                                                         |
| `mln_resource_request_handle*`      | `type ResourceRequestHandle struct`; synchronized one-shot completion, releasable from C-permitted threads.                                                                                                                                      |
| `mln_offline_operation_id`          | `type OfflineOperationHandle[T] struct` or equivalent operation token; `Take` consumes completed results, `Discard` releases runtime-owned operation state, failed native calls leave the token retryable.                                       |
| Offline region snapshots/lists      | Copied `OfflineRegionID`, `OfflineRegionInfo`, and status values; internal guards destroy native snapshot/list handles after copying.                                                                                                            |
| Session-owned texture frame values  | `type MetalOwnedTextureFrameHandle` and `type VulkanOwnedTextureFrameHandle` or equivalent wrappers; close on the session owner thread, release before resize/render update/detach/session close, and expose frame-scoped native pointer access. |
| C option structs                    | Go descriptor structs; materializers set `size`, masks, pointers, and nested storage.                                                                                                                                                            |
| C field masks                       | Pointers, explicit presence methods, or small option wrappers; C masks stay internal.                                                                                                                                                            |
| Closed enum domains                 | Named Go integer types with constants and explicit raw conversion helpers.                                                                                                                                                                       |
| Drift-prone output domains          | Named Go types preserving unknown raw values for diagnostics.                                                                                                                                                                                    |
| C bit masks                         | Named Go mask types with helper methods.                                                                                                                                                                                                         |
| Native result/list/snapshot handles | Internal guards that copy into Go values before release.                                                                                                                                                                                         |
| Opaque backend `void*` fields       | `NativePointer`; no ownership or memory access.                                                                                                                                                                                                  |
| CPU images and resource bytes       | `[]byte` for copied payloads; readback also supports caller-owned reusable byte slices.                                                                                                                                                          |

## Internal implementation inventory

Implemented support areas under `internal/*`:

| File or package                        | Contents                                                                                      |
| -------------------------------------- | --------------------------------------------------------------------------------------------- |
| `internal/capi`                        | Private cgo wrappers over imported C symbols, C descriptor materializers, and copied readers. |
| `internal/status/status.go`            | Status checking, diagnostic capture, native failure payloads.                                 |
| `internal/handle`                      | Pointer storage, released state, parent retention hooks, close-once behavior, leak reporting. |
| `internal/memory/strings.go`           | UTF-8 and string-view storage, embedded-NUL rejection for C string inputs.                    |
| `internal/memory/scope.go`             | Scoped temporary storage helpers for C allocation and `runtime.Pinner` lifetimes.             |
| `internal/structs/doc.go`              | Internal boundary documentation for copied descriptor/result helpers.                         |
| `internal/callback/logging.go`         | Process-global logging trampoline and callback state.                                         |
| `internal/callback/resource.go`        | Runtime resource transform/provider trampolines and request handle state.                     |
| `internal/callback/custom_geometry.go` | Map/style-scoped custom geometry trampolines and delayed release.                             |

Public-package files own semantic Go descriptors and delegate C layout details
to `internal/capi`. Session-owned texture frame active-state checks live beside
the render session wrappers in `render.go`.

## C API coverage map

Every public C function listed here has a Go implementation. Reviewers compare
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

## Testing map

Go tests exercise the public Go API against the real C library. They focus on
Go-owned behavior:

- `errors.Is` category mapping and `errors.As` access to raw status and copied
  diagnostics;
- immediate same-thread diagnostic capture for native failures;
- explicit `Close() error` idempotence and failed-close retry behavior;
- parent retention while child handles are live;
- embedded-NUL rejection and scoped UTF-8/string-view storage;
- cgo pointer-rule helpers, `runtime.Pinner` lifetimes, and `runtime.KeepAlive`
  for wrappers, callback state, pinned buffers, parent handles, and slices;
- callback panic recovery and exactly-once callback-state release;
- copied runtime events and query/style result payloads;
- owner-thread wrong-thread status propagation;
- resource request one-shot completion;
- frame handle active-state invalidation and nested acquisition rejection;
- render readback into caller-owned `[]byte` storage.

Tests cover the proof slice and focused regressions for each API area without
retesting all native C validation rules.

## Implementation milestones

- [x] Keep the proof slice green: ABI version, supported backends, network
      status, `Error`, and `NativePointer`.
- [x] Implement shared support helpers: UTF-8/string storage, temporary memory,
      descriptor materialization, copied-result guards, cgo pointer helpers, and
      native handle state leak reporting.
- [x] Implement `RuntimeHandle`, `MapHandle`, `MapProjectionHandle`, and
      close-once lifecycle with parent retention.
- [x] Implement process-global logging and callback trampoline retention.
- [x] Implement runtime creation, pumping, event polling, resource transforms,
      and resource providers.
- [x] Implement map creation, style loading, camera descriptors, and camera
      APIs.
- [x] Implement `MapProjectionHandle` and process-global projection helpers.
- [x] Implement render sessions, Metal/Vulkan surface descriptors, texture
      targets, readback, and frame handles.
- [x] Implement query, style, JSON, GeoJSON, image, and custom geometry APIs.
- [x] Implement offline operation handles and copied offline results.
- [x] Add CI task coverage for supported Go variants. Package/native library
      distribution remains out of scope.
- [x] Mark all coverage items complete before changing the binding from draft to
      ready for review.

## Completion checklist

- [x] Public package `maplibre` exposes no `C.*` symbols, raw C structs, field
      masks, callback trampolines, or internal package types.
- [x] Every long-lived native object has a `*Handle` struct with `Close() error`
      and documented owner-thread behavior.
- [x] Every C function in the coverage map has a Go implementation or a recorded
      unsupported reason.
- [x] Native failures map to `Error` with stable sentinels, raw status, and
      copied diagnostics.
- [x] Go descriptors own semantic fields; internal materializers write C `size`
      and mask fields.
- [x] cgo pointer-rule helpers document and test every retained, pinned, and
      call-scoped pointer shape.
- [x] Finalizers report leaked thread-affine handles instead of destroying them,
      and cgo calls use `runtime.KeepAlive` for wrappers, callback state, pinned
      buffers, retained parents, and slices that must remain reachable until the
      native call returns.
- [x] Callback state releases exactly once after its C owner scope and active
      upcalls finish.
- [x] Session-owned texture frame values close on the session owner thread,
      reject use after frame close, and close before resize, render update,
      detach, or session destruction.
- [x] `mise run //bindings/go:build` passes.
- [x] `mise run //bindings/go:test` passes.
- [x] `mise run //bindings/go:ci` passes.
