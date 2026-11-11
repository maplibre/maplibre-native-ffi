// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_HANDLES_H
#define MAPLIBRE_NATIVE_FFI_HANDLES_H

#include <gio/gio.h>

#include "maplibre-native-ffi-command-completion.h"
#include "maplibre-native-ffi-values.h"

G_BEGIN_DECLS

/**
 * MaplibreNativeFfiAcquiredFrame:
 *
 * A rendered frame that the caller holds to read or show.
 *
 * See `mln_acquired_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_ACQUIRED_FRAME \
  (maplibre_native_ffi_acquired_frame_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiAcquiredFrame, maplibre_native_ffi_acquired_frame,
  MAPLIBRE_NATIVE_FFI, ACQUIRED_FRAME, GObject
)

/**
 * maplibre_native_ffi_acquired_frame_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_acquired_frame_get_id(
  MaplibreNativeFfiAcquiredFrame* self
);

/**
 * maplibre_native_ffi_acquired_frame_discard:
 *
 * Gives up an acquired frame without saying when your GPU stops reading it.
 *
 * Because your GPU may still read the frame's texture, the session never
 * renders into that texture again.
 *
 * The object calls this when it is disposed, unless it was released or
 * discarded. Later calls fail as on any released handle.
 *
 * See `mln_acquired_frame_discard` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_acquired_frame_discard(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);

#if defined(__linux__)
/**
 * maplibre_native_ffi_acquired_frame_get_dma_buf:
 *
 * Gets the dmabuf of an acquired frame.
 *
 * Wait for `maplibre_native_ffi_acquired_frame_get_producer_sync()` before you
 * read the buffer. Once you release or discard the frame, the session may
 * render into the buffer again. You can map a buffer with
 * DRM_FORMAT_MOD_LINEAR, which has one plane, with `mmap`, and bracket your
 * reads with DMA_BUF_IOCTL_SYNC.
 *
 * The session owns the planes' file descriptors, so don't close them. To keep
 * the buffer after you release the frame, duplicate each file descriptor with
 * `dup`. A duplicate keeps the buffer's memory, but not its contents.
 *
 * See `mln_acquired_frame_get_dma_buf` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session failed, or you
 *   detached, abandoned, or disposed it.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_acquired_frame_get_dma_buf(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_acquired_frame_get_io_surface:
 *
 * Gets the IOSurface of an acquired frame.
 *
 * Wait for `maplibre_native_ffi_acquired_frame_get_producer_sync()` before you
 * read the surface. Once you release or discard the frame, the session may
 * render into the surface again. For an API that takes a `CVPixelBuffer`, wrap
 * the surface with `CVPixelBufferCreateWithIOSurface`. A reference that you
 * take with `CFRetain`, or a pixel buffer that wraps the surface, keeps the
 * surface's memory, but not its contents.
 *
 * See `mln_acquired_frame_get_io_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session failed, or you
 *   detached, abandoned, or disposed it.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiIoSurfaceFrame*
maplibre_native_ffi_acquired_frame_get_io_surface(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_acquired_frame_get_metal_texture:
 *
 * Gets the Metal texture of an acquired frame.
 *
 * See `mln_acquired_frame_get_metal_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session failed, or you
 *   detached, abandoned, or disposed it.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiMetalTextureFrame*
maplibre_native_ffi_acquired_frame_get_metal_texture(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);
#endif

/**
 * maplibre_native_ffi_acquired_frame_get_open_gl_texture:
 *
 * Gets the OpenGL texture of an acquired frame.
 *
 * See `mln_acquired_frame_get_open_gl_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session failed, or you
 *   detached, abandoned, or disposed it.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiOpenGlTextureFrame*
maplibre_native_ffi_acquired_frame_get_open_gl_texture(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);

/**
 * maplibre_native_ffi_acquired_frame_get_producer_sync:
 *
 * Reads what to wait for before you read an acquired frame's texture.
 *
 * See `mln_acquired_frame_get_producer_sync` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session failed, or you
 *   detached, abandoned, or disposed it.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiGpuSync* maplibre_native_ffi_acquired_frame_get_producer_sync(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);

/**
 * maplibre_native_ffi_acquired_frame_get_result:
 *
 * Reads the frame result of an acquired frame.
 *
 * It succeeds until you release or discard the frame, whatever the session's
 * state.
 *
 * See `mln_acquired_frame_get_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiRenderFrameResult*
maplibre_native_ffi_acquired_frame_get_result(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);

/**
 * maplibre_native_ffi_acquired_frame_get_vulkan_texture:
 *
 * Gets the Vulkan image of an acquired frame.
 *
 * See `mln_acquired_frame_get_vulkan_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session failed, or you
 *   detached, abandoned, or disposed it.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_acquired_frame_get_vulkan_texture(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
);

/**
 * maplibre_native_ffi_acquired_frame_release:
 *
 * Gives an acquired frame back to the session once you're done reading it.
 *
 * `consumer_completion` says when your GPU work that reads the texture
 * finishes. Pass `MAPLIBRE_NATIVE_FFI_GPU_SYNC_CPU_COMPLETE` when that work has
 * already finished. The session waits for that point before it renders into the
 * texture again. If that wait fails, such as for an OpenGL fence outside the
 * session's share group, the session fails.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_acquired_frame_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `MaplibreNativeFfiRenderSessionCapabilities.consumer_sync_kinds` lacks the
 *   kind of `consumer_completion`.
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_acquired_frame_release(
  MaplibreNativeFfiAcquiredFrame* self,
  const MaplibreNativeFfiGpuSync* consumer_completion, GError** error
);

/**
 * MaplibreNativeFfiFramePacer:
 *
 * Demands render sessions' frames at a display's refreshes.
 *
 * See `mln_frame_pacer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_FRAME_PACER \
  (maplibre_native_ffi_frame_pacer_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiFramePacer, maplibre_native_ffi_frame_pacer,
  MAPLIBRE_NATIVE_FFI, FRAME_PACER, GObject
)

/**
 * maplibre_native_ffi_frame_pacer_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_frame_pacer_get_id(
  MaplibreNativeFfiFramePacer* self
);

/**
 * maplibre_native_ffi_frame_pacer_release:
 *
 * Releases a frame pacer.
 *
 * Render sessions that use the pacer stay paced until you release them or set
 * another pacer.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_frame_pacer_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
void maplibre_native_ffi_frame_pacer_release(MaplibreNativeFfiFramePacer* self);

/**
 * MaplibreNativeFfiMap:
 *
 * A map, with its style and camera.
 *
 * See `mln_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP (maplibre_native_ffi_map_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMap, maplibre_native_ffi_map, MAPLIBRE_NATIVE_FFI, MAP,
  GObject
)

/**
 * maplibre_native_ffi_map_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_map_get_id(MaplibreNativeFfiMap* self);

/**
 * maplibre_native_ffi_map_add_custom_geometry_source_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds a source whose GeoJSON tiles come from callbacks.
 *
 * See `mln_map_add_custom_geometry_source` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS` when the style already has a
 *   source named `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_add_custom_geometry_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiCustomGeometrySourceOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_add_custom_geometry_source_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_add_custom_geometry_source_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_custom_geometry_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_add_custom_vector_source_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds a source whose vector tiles come from callbacks.
 *
 * See `mln_map_add_custom_vector_source` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS` when the style already has a
 *   source named `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_add_custom_vector_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiCustomVectorSourceOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_add_custom_vector_source_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_add_custom_vector_source_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_custom_vector_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_add_geo_json_source_async:
 * @source_id: the UTF-8 text
 * @options: (nullable): the value, or %NULL
 * @attribution: (nullable): the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds a GeoJSON source with no data.
 *
 * Give it data with `maplibre_native_ffi_map_set_geo_json_source_data_async()`
 * or `maplibre_native_ffi_map_set_geo_json_source_url_async()`. Its options
 * stay fixed for as long as the style holds it.
 *
 * `attribution` is the source's
 * \[attribution\](https://maplibre.org/maplibre-style-spec/sources/#attribution),
 * or empty for none.
 *
 * See `mln_map_add_geo_json_source` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when an option in `options` is
 *   out of range.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS` when the style already has a
 *   source named `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_add_geo_json_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiGeoJsonSourceOptions* options, const char* attribution,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_add_geo_json_source_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_add_geo_json_source_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_geo_json_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_add_image_source_image_async:
 * @source_id: the UTF-8 text
 * @coordinates: (array length=coordinate_count): the items
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds an image source that shows `image`.
 *
 * `coordinates` are the image's four corners, clockwise from the top left.
 *
 * See `mln_map_add_image_source_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS` when the style already has a
 *   source named `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_add_image_source_image_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLng* coordinates, gsize coordinate_count,
  MaplibreNativeFfiPremultipliedRgba8Image* image, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_add_image_source_image_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_add_image_source_image_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_image_source_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_add_style_layer_json_async:
 * @layer_json: the UTF-8 text
 * @before_layer_id: (nullable): the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds a layer to the map's style.
 *
 * `layer_json` is a \[layer\](https://maplibre.org/maplibre-style-spec/layers/)
 * object. The layer goes below the layer named `before_layer_id`, or above
 * every layer when `before_layer_id` is empty.
 *
 * See `mln_map_add_style_layer_json` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no layer named
 *   `before_layer_id`, or no source with the ID that the layer uses.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS` when the style already has a
 *   layer with the new layer's ID.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_add_style_layer_json_async(
  MaplibreNativeFfiMap* self, const char* layer_json,
  const char* before_layer_id, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_add_style_layer_json_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_add_style_layer_json_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_style_layer_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_add_style_source_json_async:
 * @source_id: the UTF-8 text
 * @source_json: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds a source to the map's style.
 *
 * `source_json` is a
 * \[source\](https://maplibre.org/maplibre-style-spec/sources/) object.
 *
 * See `mln_map_add_style_source_json` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS` when the style already has a
 *   source named `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_add_style_source_json_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* source_json,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_add_style_source_json_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_add_style_source_json_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_style_source_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_apply_camera_delta_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves the camera relative to where it is, as a gesture does.
 *
 * `delta` can pan the map's content by an offset on the screen, scale the zoom,
 * and add to the bearing and pitch. An anchor keeps one screen point in place
 * while the scale, bearing, or pitch changes. An anchor needs one of those
 * changes, and combines with an offset only when the delta has no animation.
 * The function finishes when the map applies the delta, before an animation
 * ends.
 *
 * A delta that replaces an animated one starts from where that animation left
 * the camera, so the rest of the replaced change never happens. To reach an
 * exact camera, use `maplibre_native_ffi_map_update_camera_async()`.
 *
 * A delta in the end phase can carry momentum, which keeps the camera moving
 * from the gesture's release velocities and slows it on each rendered frame.
 * The gesture lasts until the momentum ends: the snapshot keeps
 * `gesture_in_progress` set, and camera events report
 * `MAPLIBRE_NATIVE_FFI_CAMERA_CHANGE_SOURCE_MOMENTUM`. With momentum,
 * `animation` sets only `transition_id` and the end handler, which runs when
 * the gesture ends.
 *
 * A delta in the begin phase,
 * `maplibre_native_ffi_map_cancel_camera_transitions_async()`, or a
 * cancellation by that ID stops the momentum. A later command that sets a field
 * stops only the parts of the momentum that move that field. On a tilted map,
 * the pan moves the map's content the same distance on the screen in every
 * direction.
 *
 * See `mln_map_apply_camera_delta` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_apply_camera_delta_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiCameraDelta* delta,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_apply_camera_delta_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_apply_camera_delta_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_apply_camera_delta_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

#if defined(__linux__)
/**
 * maplibre_native_ffi_map_attach_dma_buf_ring:
 * @device: (nullable): the handle, or %NULL
 *
 * Creates a session that renders the map into a ring of dmabufs that the
 * library allocates.
 *
 * Acquire each frame with `maplibre_native_ffi_render_session_acquire_frame()`,
 * and get its buffer with `maplibre_native_ffi_acquired_frame_get_dma_buf()`.
 * `maplibre_native_ffi_render_session_resize_async()` gives the ring new
 * buffers of the new size.
 *
 * A Vulkan build renders with `device`, or, when `device` is null, with a
 * device that the library creates. An OpenGL build uses an EGL context of its
 * own, and `device` must be null. Attach with
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER`.
 *
 * For a reader that waits on no sync file, such as a Wayland compositor without
 * explicit synchronization, attach without
 * `MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_SYNC_FD` in
 * `MaplibreNativeFfiRenderSessionAttachOptions:producer-sync-kinds`, so that
 * each frame finishes before you acquire it. Keep each frame acquired until its
 * reader is done with the buffer. Give the ring 3 buffers, so the session can
 * render while you hold two frames.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_dma_buf_ring` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the graphics context can't
 *   render into a dmabuf of any of `formats`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the library can't create its
 *   Vulkan device or initialize an EGL display.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_dma_buf_ring(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiDmaBufRingDescriptor* descriptor,
  MaplibreNativeFfiVulkanDevice* device,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_map_attach_io_surface_ring:
 *
 * Creates a session that renders the map into a ring of IOSurfaces that the
 * library allocates.
 *
 * Acquire each frame with `maplibre_native_ffi_render_session_acquire_frame()`,
 * and get its surface with
 * `maplibre_native_ffi_acquired_frame_get_io_surface()`.
 * `maplibre_native_ffi_render_session_resize_async()` gives the ring new
 * surfaces of the new size. Attach with
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER`.
 *
 * To hand frames to a reader that can't wait on an event, such as Flutter's
 * `copyPixelBuffer`, attach without
 * `MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT` in
 * `MaplibreNativeFfiRenderSessionAttachOptions:producer-sync-kinds`, so that
 * each frame finishes before you acquire it. Release a frame with
 * `MAPLIBRE_NATIVE_FFI_GPU_SYNC_CPU_COMPLETE` only once the reader has finished
 * with its surface.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_io_surface_ring` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_supported_render_backend_mask()` lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_METAL`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when `context` names no device and
 *   the system has no Metal device.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_io_surface_ring(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiIoSurfaceRingDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_map_attach_metal_borrowed_texture:
 *
 * Creates a session that renders the map into a ring of your Metal textures.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_supported_render_backend_mask()` lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_METAL`, or when the target
 *   supports none of the `producer_sync_kinds` in `options`.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession*
maplibre_native_ffi_map_attach_metal_borrowed_texture(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_map_attach_metal_surface:
 *
 * Creates a session that renders the map to a Metal layer.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_metal_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_supported_render_backend_mask()` lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_METAL`.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_metal_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiMetalSurfaceDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);
#endif

/**
 * maplibre_native_ffi_map_attach_offscreen:
 * @device: (nullable): the handle, or %NULL
 *
 * Attaches a render session that renders the map offscreen.
 *
 * Read each image's pixels with
 * `maplibre_native_ffi_render_session_read_texture_async()`.
 *
 * The session renders on `device`. When `device` is null, the session creates a
 * device with the default options, and releases it when the session ends.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_offscreen` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when the map already has a render
 *   session.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when `device` is null and the
 *   platform can't create a device.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_offscreen(
  MaplibreNativeFfiMap* self,
  const MaplibreNativeFfiOffscreenDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options,
  MaplibreNativeFfiOffscreenDevice* device, GError** error
);

/**
 * maplibre_native_ffi_map_attach_open_gl_borrowed_texture:
 *
 * Creates a session that renders the map into a ring of your OpenGL textures.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_open_gl_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_open_gl_supported_context_provider_mask()` lacks the
 *   context's platform, or when the target supports none of the
 *   `producer_sync_kinds` in `options`.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession*
maplibre_native_ffi_map_attach_open_gl_borrowed_texture(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);

/**
 * maplibre_native_ffi_map_attach_open_gl_surface:
 *
 * Creates a session that renders the map to an OpenGL surface.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_open_gl_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_open_gl_supported_context_provider_mask()` lacks the
 *   context's platform.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_open_gl_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiOpenGlSurfaceDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);

/**
 * maplibre_native_ffi_map_attach_vulkan_borrowed_texture:
 *
 * Creates a session that renders the map into a ring of your Vulkan images.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_vulkan_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_supported_render_backend_mask()` lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_VULKAN`, or when the target
 *   supports none of the `producer_sync_kinds` in `options`.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession*
maplibre_native_ffi_map_attach_vulkan_borrowed_texture(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);

/**
 * maplibre_native_ffi_map_attach_vulkan_surface:
 *
 * Creates a session that renders the map to a Vulkan surface.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_vulkan_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_supported_render_backend_mask()` lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_VULKAN`.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_vulkan_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiVulkanSurfaceDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);

#if defined(__linux__)
/**
 * maplibre_native_ffi_map_attach_wayland_surface:
 * @device: (nullable): the handle, or %NULL
 *
 * Creates a session that presents the map to a Wayland surface, with a graphics
 * context that the library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a
 * device that the library creates. An OpenGL build uses an EGL context on the
 * display, and `device` must be null. Attach with
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER`.
 *
 * The session presents without waiting for the compositor, so demand at most
 * one frame per display refresh.
 *
 * Keep the surface until a release or replacement of the target, or a detach of
 * the session, completes. Keep the display connected until the session's detach
 * completes. In an OpenGL build, if you also use EGL on the display, initialize
 * it before you attach.
 *
 * The object is ready once `maplibre_native_ffi_render_session_ready_async`
 * finishes, which fails with the errors that the completion reports.
 *
 * See `mln_map_attach_wayland_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when you created `device` without
 *   `MaplibreNativeFfiVulkanDeviceOptions:presentation`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the GPU or the Vulkan loader
 *   can't present to Wayland.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the library can't create the
 *   graphics context or present to the surface.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_wayland_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiWaylandSurfaceDescriptor* descriptor,
  MaplibreNativeFfiVulkanDevice* device,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
);
#endif

/**
 * maplibre_native_ffi_map_begin_command_group_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Holds back the map's changes from rendering until
 * `maplibre_native_ffi_map_end_command_group_async()`, so they show in the same
 * frame.
 *
 * Groups nest, and the changes render when the outermost group ends.
 *
 * See `mln_map_begin_command_group` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_begin_command_group_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_begin_command_group_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_begin_command_group_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_begin_command_group_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_camera_for_geometry_async:
 * @geometry: the UTF-8 text
 * @fit_options: (nullable): the value, or %NULL
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Computes a camera that fits a GeoJSON geometry in the map's view.
 *
 * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
 * feature. The camera keeps the map's bearing and pitch unless `fit_options`
 * sets them, and its zoom stays within the map's zoom limits.
 *
 * See `mln_map_camera_for_geometry` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the padding in
 *   `fit_options` leaves no room for the area.
 */
void maplibre_native_ffi_map_camera_for_geometry_async(
  MaplibreNativeFfiMap* self, const char* geometry,
  MaplibreNativeFfiCameraFitOptions* fit_options, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_camera_for_geometry_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_camera_for_geometry_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_map_camera_for_geometry_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_camera_for_lat_lng_bounds_async:
 * @fit_options: (nullable): the value, or %NULL
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Computes a camera that fits `bounds` in the map's view.
 *
 * The camera keeps the map's bearing and pitch unless `fit_options` sets them,
 * and its zoom stays within the map's zoom limits.
 *
 * See `mln_map_camera_for_lat_lng_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the padding in
 *   `fit_options` leaves no room for the area.
 */
void maplibre_native_ffi_map_camera_for_lat_lng_bounds_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLngBounds* bounds,
  MaplibreNativeFfiCameraFitOptions* fit_options, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_camera_for_lat_lng_bounds_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_camera_for_lat_lng_bounds_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_map_camera_for_lat_lng_bounds_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_camera_for_lat_lngs_async:
 * @coordinates: (array length=coordinate_count): the items
 * @fit_options: (nullable): the value, or %NULL
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Computes a camera that fits `coordinates` in the map's view.
 *
 * `coordinates` must not be empty. The camera keeps the map's bearing and pitch
 * unless `fit_options` sets them, and its zoom stays within the map's zoom
 * limits.
 *
 * See `mln_map_camera_for_lat_lngs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the padding in
 *   `fit_options` leaves no room for the area.
 */
void maplibre_native_ffi_map_camera_for_lat_lngs_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLng* coordinates,
  gsize coordinate_count, MaplibreNativeFfiCameraFitOptions* fit_options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_camera_for_lat_lngs_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_camera_for_lat_lngs_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_map_camera_for_lat_lngs_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_cancel_camera_transitions_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Stops every running camera animation where it is.
 *
 * The end handler of each stopped animation reports
 * `MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CANCELLED`.
 *
 * See `mln_map_cancel_camera_transitions` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_cancel_camera_transitions_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_cancel_camera_transitions_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_cancel_camera_transitions_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_cancel_camera_transitions_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_cancel_camera_transitions_with_id_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Stops the running camera animations whose animation options set
 * `transition_id`.
 *
 * Other animations keep running. The end handler of each stopped animation
 * reports `MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CANCELLED`.
 *
 * See `mln_map_cancel_camera_transitions_with_id` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_cancel_camera_transitions_with_id_async(
  MaplibreNativeFfiMap* self, guint64 transition_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_cancel_camera_transitions_with_id_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_cancel_camera_transitions_with_id_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_cancel_camera_transitions_with_id_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_dispose:
 *
 * Releases the map in any state, and finishes in the background.
 *
 * Use it from destructors and finalizers. Otherwise, use
 * `maplibre_native_ffi_map_release_async()`.
 *
 * The object calls this when it is disposed, unless its handle was released.
 * Later calls do nothing.
 *
 * See `mln_map_dispose` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_map_dispose(MaplibreNativeFfiMap* self);

/**
 * maplibre_native_ffi_map_dump_debug_logs_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Logs debugging information about the map's style and sources.
 *
 * See `mln_map_dump_debug_logs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_dump_debug_logs_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_dump_debug_logs_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_dump_debug_logs_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_dump_debug_logs_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_end_command_group_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Ends the group that `maplibre_native_ffi_map_begin_command_group_async()`
 * began.
 *
 * See `mln_map_end_command_group` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_end_command_group_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_end_command_group_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_end_command_group_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_end_command_group_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_fit_camera_to_geometry_async:
 * @geometry: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves the camera to fit a GeoJSON geometry in the map's view, at once or with
 * an animation.
 *
 * `geometry` is a GeoJSON geometry object with at least one coordinate, not a
 * feature. The camera is the one that
 * `maplibre_native_ffi_map_camera_for_geometry_async()` computes after the
 * commands sent before this one apply. The function finishes when the map
 * applies the change, before an animation ends.
 *
 * See `mln_map_fit_camera_to_geometry` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the padding in `update`
 *   leaves no room for the area. The camera doesn't move.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_fit_camera_to_geometry_async(
  MaplibreNativeFfiMap* self, const char* geometry,
  MaplibreNativeFfiCameraFitUpdate* update, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_fit_camera_to_geometry_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_fit_camera_to_geometry_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_fit_camera_to_geometry_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves the camera to fit `bounds` in the map's view, at once or with an
 * animation.
 *
 * The camera is the one that
 * `maplibre_native_ffi_map_camera_for_lat_lng_bounds_async()` computes after
 * the commands sent before this one apply. The function finishes when the map
 * applies the change, before an animation ends.
 *
 * See `mln_map_fit_camera_to_lat_lng_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the padding in `update`
 *   leaves no room for the area. The camera doesn't move.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLngBounds* bounds,
  MaplibreNativeFfiCameraFitUpdate* update, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_fit_camera_to_lat_lngs_async:
 * @coordinates: (array length=coordinate_count): the items
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves the camera to fit `coordinates` in the map's view, at once or with an
 * animation.
 *
 * `coordinates` must not be empty. The camera is the one that
 * `maplibre_native_ffi_map_camera_for_lat_lngs_async()` computes after the
 * commands sent before this one apply. The function finishes when the map
 * applies the change, before an animation ends.
 *
 * See `mln_map_fit_camera_to_lat_lngs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the padding in `update`
 *   leaves no room for the area. The camera doesn't move.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_fit_camera_to_lat_lngs_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLng* coordinates,
  gsize coordinate_count, MaplibreNativeFfiCameraFitUpdate* update,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_fit_camera_to_lat_lngs_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_fit_camera_to_lat_lngs_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_fit_camera_to_lat_lngs_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_camera_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the map's camera.
 *
 * The query reads the camera after the map applies the commands sent before it.
 *
 * See `mln_map_get_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
void maplibre_native_ffi_map_get_camera_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_camera_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_camera_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiCameraQueryResult* maplibre_native_ffi_map_get_camera_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_feature_state_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the state of a feature, as a JSON object.
 *
 * The object is empty when the feature has no state, including after its source
 * leaves the style or a style loads, as
 * `maplibre_native_ffi_map_set_feature_state_async()` describes.
 *
 * See `mln_map_get_feature_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_map_get_feature_state_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFeatureStateSelector* selector,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_feature_state_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_feature_state_async` started.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_map_get_feature_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_global_state_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the global state of the map's style as a JSON object.
 *
 * See `mln_map_get_global_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_global_state_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_global_state_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_global_state_async` started.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_map_get_global_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_image_source_coordinates_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the corners of an image source's image, clockwise from the top left.
 *
 * See `mln_map_get_image_source_coordinates` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't an image source.
 */
void maplibre_native_ffi_map_get_image_source_coordinates_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_image_source_coordinates_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_image_source_coordinates_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiLatLng): the items
 */
GPtrArray* maplibre_native_ffi_map_get_image_source_coordinates_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_loaded_style_json_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the JSON of the style that the map loaded.
 *
 * The JSON doesn't include the changes made to the style since it loaded.
 *
 * See `mln_map_get_loaded_style_json` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_map_get_loaded_style_json_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_loaded_style_json_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_loaded_style_json_async` started.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_map_get_loaded_style_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_snapshot:
 *
 * Copies the map's state.
 *
 * The copy doesn't wait for pending commands. To read the camera after they
 * apply, use `maplibre_native_ffi_map_get_camera_async()`.
 *
 * See `mln_map_get_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` until
 *   `maplibre_native_ffi_runtime_create_map()` completes with `0`.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiMapSnapshot* maplibre_native_ffi_map_get_snapshot(
  MaplibreNativeFfiMap* self, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_image_async:
 * @image_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads an image of the map's style.
 *
 * Completes with no value when the style has no image named `image_id`.
 *
 * See `mln_map_get_style_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_style_image_async(
  MaplibreNativeFfiMap* self, const char* image_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_image_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_image_async` started.
 *
 * Returns: (transfer full) (nullable): the value
 */
MaplibreNativeFfiStyleImageInfo* maplibre_native_ffi_map_get_style_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_layer_async:
 * @layer_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads a layer of the map's style.
 *
 * Completes with no value when the style has no layer named `layer_id`.
 *
 * See `mln_map_get_style_layer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_style_layer_async(
  MaplibreNativeFfiMap* self, const char* layer_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_layer_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_layer_async` started.
 *
 * Returns: (transfer full) (nullable): the value
 */
MaplibreNativeFfiStyleLayerInfo* maplibre_native_ffi_map_get_style_layer_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_layer_json_async:
 * @layer_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads a layer of the map's style as a
 * \[layer\](https://maplibre.org/maplibre-style-spec/layers/) object.
 *
 * Completes with no value when the style has no layer named `layer_id`.
 *
 * See `mln_map_get_style_layer_json` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_style_layer_json_async(
  MaplibreNativeFfiMap* self, const char* layer_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_layer_json_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_layer_json_async` started.
 *
 * Returns: (transfer full) (nullable): the text
 */
char* maplibre_native_ffi_map_get_style_layer_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_layer_property_async:
 * @layer_id: the UTF-8 text
 * @property_name: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads a property of a layer in the map's style.
 *
 * The value can differ in form from the value that was set: a color set as
 * `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.
 *
 * Completes with no value when the layer doesn't set the property.
 *
 * See `mln_map_get_style_layer_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no layer named
 *   `layer_id`.
 */
void maplibre_native_ffi_map_get_style_layer_property_async(
  MaplibreNativeFfiMap* self, const char* layer_id, const char* property_name,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_layer_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_layer_property_async` started.
 *
 * Returns: (transfer full) (nullable): the text
 */
char* maplibre_native_ffi_map_get_style_layer_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_light_property_async:
 * @property_name: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads a property of the light of the map's style.
 *
 * The value can differ in form from the value that was set: a color set as
 * `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.
 *
 * Completes with no value when the light doesn't set the property.
 *
 * See `mln_map_get_style_light_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_style_light_property_async(
  MaplibreNativeFfiMap* self, const char* property_name,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_light_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_light_property_async` started.
 *
 * Returns: (transfer full) (nullable): the text
 */
char* maplibre_native_ffi_map_get_style_light_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_root_property_async:
 * @property_name: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads a \[root property\](https://maplibre.org/maplibre-style-spec/root/) of
 * the map's style.
 *
 * The value can differ in form from the value that was set: a color set as
 * `"#ff0000"` reads back as `["rgba", 255, 0, 0, 1]`.
 *
 * Completes with no value when neither the style nor the library sets the
 * property. The library can give a property a value of its own, such as a
 * default `transition`.
 *
 * See `mln_map_get_style_root_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `property_name` isn't a
 *   root property, or is `sources` or `layers`.
 */
void maplibre_native_ffi_map_get_style_root_property_async(
  MaplibreNativeFfiMap* self, const char* property_name,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_root_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_root_property_async` started.
 *
 * Returns: (transfer full) (nullable): the text
 */
char* maplibre_native_ffi_map_get_style_root_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_source_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads a source of the map's style.
 *
 * A tile source with a `url` has an empty attribution until its TileJSON loads,
 * which raises `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SOURCE_CHANGED`.
 *
 * Completes with no value when the style has no source named `source_id`.
 *
 * See `mln_map_get_style_source` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_style_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_source_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_source_async` started.
 *
 * Returns: (transfer full) (nullable): the value
 */
MaplibreNativeFfiStyleSourceInfo*
maplibre_native_ffi_map_get_style_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_transition_options_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the default transition of the map's style.
 *
 * See `mln_map_get_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_get_style_transition_options_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_transition_options_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_transition_options_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_map_get_style_transition_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_get_style_url_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the URL of the map's style.
 *
 * The URL is empty when the style came from
 * `maplibre_native_ffi_map_set_style_json_async()`.
 *
 * See `mln_map_get_style_url` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_map_get_style_url_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_get_style_url_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_get_style_url_async` started.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_map_get_style_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_invalidate_custom_geometry_source_region_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes a custom geometry source ask again for the tiles in `bounds` that the
 * map still needs.
 *
 * See `mln_map_invalidate_custom_geometry_source_region` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom geometry source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_invalidate_custom_geometry_source_region_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLngBounds* bounds, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_invalidate_custom_geometry_source_region_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_invalidate_custom_geometry_source_region_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_geometry_source_region_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes a custom geometry source ask for a tile again when the map needs it.
 *
 * See `mln_map_invalidate_custom_geometry_source_tile` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom geometry source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_invalidate_custom_vector_source_region_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes a custom vector source ask again for the tiles in `bounds` that the map
 * still needs.
 *
 * See `mln_map_invalidate_custom_vector_source_region` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom vector source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_invalidate_custom_vector_source_region_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLngBounds* bounds, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_invalidate_custom_vector_source_region_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_invalidate_custom_vector_source_region_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_vector_source_region_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_invalidate_custom_vector_source_tile_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes a custom vector source ask for a tile again when the map needs it.
 *
 * See `mln_map_invalidate_custom_vector_source_tile` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom vector source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_invalidate_custom_vector_source_tile_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_invalidate_custom_vector_source_tile_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_invalidate_custom_vector_source_tile_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_vector_source_tile_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_lat_lng_bounds_for_camera_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Computes the area that the map would show with `camera`.
 *
 * Camera fields that `camera` doesn't set come from the map's camera. The
 * longitudes can fall outside -180 to 180, so a view across the antimeridian
 * gives one continuous range.
 *
 * See `mln_map_lat_lng_bounds_for_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 */
void maplibre_native_ffi_map_lat_lng_bounds_for_camera_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiCameraOptions* camera,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_lat_lng_bounds_for_camera_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_lat_lng_bounds_for_camera_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_map_lat_lng_bounds_for_camera_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_list_style_layers_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Lists the layers of the map's style, from bottom to top.
 *
 * See `mln_map_list_style_layers` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_list_style_layers_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_list_style_layers_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_list_style_layers_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiStyleLayerInfo): the items
 */
GPtrArray* maplibre_native_ffi_map_list_style_layers_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_list_style_sources_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Lists the sources of the map's style.
 *
 * See `mln_map_list_style_sources` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
void maplibre_native_ffi_map_list_style_sources_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_list_style_sources_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_list_style_sources_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiStyleSourceInfo): the items
 */
GPtrArray* maplibre_native_ffi_map_list_style_sources_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_move_style_layer_async:
 * @layer_id: the UTF-8 text
 * @before_layer_id: (nullable): the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves a layer of the map's style below the layer named `before_layer_id`, or
 * above every layer when `before_layer_id` is empty.
 *
 * See `mln_map_move_style_layer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no layer named
 *   `layer_id` or `before_layer_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_move_style_layer_async(
  MaplibreNativeFfiMap* self, const char* layer_id, const char* before_layer_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_move_style_layer_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_move_style_layer_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_move_style_layer_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_release_async:
 * @callback: (scope async) (nullable): the callback to call when the release finishes
 * @user_data: the data to pass to `callback`
 *
 * Releases the map.
 *
 * Detach the map's render session first.
 *
 * See `mln_map_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_map_release_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_release_finish:
 * @result: the result that the callback received
 *
 * Finishes a release that `maplibre_native_ffi_map_release_async` started.
 *
 * Returns: %TRUE when the handle retired, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_map_release_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_remove_feature_state_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Removes the feature state that `selector` selects.
 *
 * See `mln_map_remove_feature_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_remove_feature_state_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFeatureStateSelector* selector,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_remove_feature_state_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_remove_feature_state_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_feature_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_remove_style_image_async:
 * @image_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Removes an image from the map's style.
 *
 * See `mln_map_remove_style_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no image named
 *   `image_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_remove_style_image_async(
  MaplibreNativeFfiMap* self, const char* image_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_remove_style_image_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_remove_style_image_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_style_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_remove_style_layer_async:
 * @layer_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Removes a layer from the map's style.
 *
 * See `mln_map_remove_style_layer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no layer named
 *   `layer_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_remove_style_layer_async(
  MaplibreNativeFfiMap* self, const char* layer_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_remove_style_layer_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_remove_style_layer_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_style_layer_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_remove_style_source_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Removes a source from the map's style.
 *
 * See `mln_map_remove_style_source` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when a layer still uses the
 *   source, or while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_remove_style_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_remove_style_source_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_remove_style_source_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_style_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_request_repaint_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Renders a new frame of a continuous map, even when nothing changed.
 *
 * See `mln_map_request_repaint` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_request_repaint_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_request_repaint_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_request_repaint_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_request_repaint_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_request_still_image_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Renders a still image of a static or tile map.
 *
 * The map's render session draws the image once every resource that the image
 * needs has loaded.
 *
 * See `mln_map_request_still_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_CANCELLED` when the map is released before the
 *   image is done.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the style or a resource that
 *   the image needs fails to load.
 */
void maplibre_native_ffi_map_request_still_image_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_request_still_image_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_request_still_image_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_map_request_still_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_resize_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets the map's size.
 *
 * A map with a render session takes its size from the session, so resize the
 * session with `maplibre_native_ffi_render_session_resize_async()` instead.
 *
 * See `mln_map_resize` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when the map has a render
 *   session.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when a render session attached to
 *   the map before the resize applied.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_resize_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLogicalSize* logical_size,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_resize_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_resize_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion* maplibre_native_ffi_map_resize_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_bounds_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Limits the camera's center to an area, and its zoom and pitch to ranges.
 *
 * Limits that `options` doesn't set keep their current values. When the camera
 * is outside the new limits, it moves inside them.
 *
 * See `mln_map_set_bounds` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_bounds_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiBoundOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_bounds_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_bounds_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion* maplibre_native_ffi_map_set_bounds_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async:
 * @source_id: the UTF-8 text
 * @data: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets the data of a tile in a custom geometry source.
 *
 * `data` is a GeoJSON feature collection, feature, or geometry. The source
 * ignores data for a tile that it hasn't asked for, or has since cancelled.
 *
 * See `mln_map_set_custom_geometry_source_tile_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom geometry source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, const char* data,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_custom_geometry_source_tile_data_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_geometry_source_tile_data_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async:
 * @source_id: the UTF-8 text
 * @message: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reports that a tile of a custom geometry source failed to load.
 *
 * See `mln_map_set_custom_geometry_source_tile_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom geometry source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, const char* message,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_custom_geometry_source_tile_error_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_geometry_source_tile_error_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_custom_vector_source_tile_data_async:
 * @source_id: the UTF-8 text
 * @data: the bytes
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets the data of a tile in a custom vector source.
 *
 * `data` is the tile, encoded as `encoding`. The source ignores data for a tile
 * that it hasn't asked for, or has since cancelled.
 *
 * See `mln_map_set_custom_vector_source_tile_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom vector source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_custom_vector_source_tile_data_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id,
  MaplibreNativeFfiStyleVectorTileEncoding encoding, GBytes* data,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_custom_vector_source_tile_data_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_custom_vector_source_tile_data_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_vector_source_tile_data_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_custom_vector_source_tile_error_async:
 * @source_id: the UTF-8 text
 * @message: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reports that a tile of a custom vector source failed to load.
 *
 * See `mln_map_set_custom_vector_source_tile_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a custom vector source.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_custom_vector_source_tile_error_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, const char* message,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_custom_vector_source_tile_error_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_custom_vector_source_tile_error_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_vector_source_tile_error_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_debug_flags_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Draws debugging overlays, such as tile borders, on the map.
 *
 * `flags` replaces the overlays set before.
 *
 * See `mln_map_set_debug_flags` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_debug_flags_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiMapDebugFlag flags,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_debug_flags_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_debug_flags_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_debug_flags_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_event_mask_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Selects the events that the map reports.
 *
 * See `mln_map_set_event_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_event_mask_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiRuntimeEventFlag mask,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_event_mask_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_event_mask_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_event_mask_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_feature_state_async:
 * @state: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Merges `state` into the state of a feature.
 *
 * `state` is a JSON object. Its keys replace the same keys in the feature's
 * state, and the other keys stay.
 *
 * The map keeps the state until it is removed, its source leaves the style, or
 * a style loads. `maplibre_native_ffi_map_remove_style_source_async()` removes
 * the state of the source's features, and a style that loads removes the state
 * of every source, so a later source with the same ID starts with no state. To
 * set state for a new style's sources, wait until
 * `maplibre_native_ffi_map_set_style_url_async()` or
 * `maplibre_native_ffi_map_set_style_json_async()` completes. The style load
 * removes state set before then.
 *
 * See `mln_map_set_feature_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_feature_state_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFeatureStateSelector* selector,
  const char* state, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_feature_state_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_feature_state_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_feature_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_free_camera_options_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Places the camera at a position and orientation in space.
 *
 * The map adjusts them to a camera that it can show. It stops running camera
 * animations first, and their end handlers report
 * `MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CANCELLED`.
 *
 * See `mln_map_set_free_camera_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_free_camera_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFreeCameraOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_free_camera_options_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_free_camera_options_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_free_camera_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_geo_json_source_data_async:
 * @source_id: the UTF-8 text
 * @data: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the data of a GeoJSON source.
 *
 * `data` is a GeoJSON feature collection, feature, or geometry. The library
 * prepares it for the source with the source's options, off the threads that
 * the caller and the runtime use, and the command completes once the source
 * holds it. Later commands don't wait for it. While the source is still
 * preparing earlier data, only the newest data waits to follow it.
 *
 * With synchronous tiling on, as
 * `maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_async()`
 * describes, the command prepares `data` before later commands run, so the data
 * shows in the next frame.
 *
 * See `mln_map_set_geo_json_source_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a GeoJSON source, when `data` isn't valid GeoJSON, or
 *   when the source clusters and `data` isn't a feature collection of points.
 *   The source keeps its data.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_CANCELLED` when newer data for the source
 *   replaces `data` while it waits, or when the source's URL, its removal, a
 *   style load, or the map's release replaces the source's data before `data`
 *   is ready.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_geo_json_source_data_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* data,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_geo_json_source_data_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_geo_json_source_data_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_geo_json_source_data_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets whether a GeoJSON source builds its tiles while the map renders.
 *
 * Building tiles while the map renders, instead of in the background, makes new
 * data show in the next frame. The setting replaces the `synchronousUpdate`
 * member of the source's JSON, which defaults to false.
 *
 * See `mln_map_set_geo_json_source_synchronous_tiling` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a GeoJSON source.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_async(
  MaplibreNativeFfiMap* self, const char* source_id, gboolean enabled,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_geo_json_source_url_async:
 * @source_id: the UTF-8 text
 * @url: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes a GeoJSON source load its data from `url`.
 *
 * See `mln_map_set_geo_json_source_url` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't a GeoJSON source.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_geo_json_source_url_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* url,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_geo_json_source_url_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_geo_json_source_url_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_geo_json_source_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_global_state_property_async:
 * @property_name: the UTF-8 text
 * @value: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets a value that the style's `global-state`
 * \[expressions\](https://maplibre.org/maplibre-style-spec/expressions/) read.
 *
 * See `mln_map_set_global_state_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_global_state_property_async(
  MaplibreNativeFfiMap* self, const char* property_name, const char* value,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_global_state_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_global_state_property_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_global_state_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_image_source_coordinates_async:
 * @source_id: the UTF-8 text
 * @coordinates: (array length=coordinate_count): the items
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves the image of an image source.
 *
 * `coordinates` are the image's four corners, clockwise from the top left.
 *
 * See `mln_map_set_image_source_coordinates` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't an image source.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_image_source_coordinates_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLng* coordinates, gsize coordinate_count,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_image_source_coordinates_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_image_source_coordinates_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_image_source_coordinates_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_image_source_image_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the image of an image source.
 *
 * See `mln_map_set_image_source_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't an image source.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_image_source_image_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiPremultipliedRgba8Image* image, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_image_source_image_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_image_source_image_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_image_source_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_image_source_url_async:
 * @source_id: the UTF-8 text
 * @url: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes an image source load its image from `url`.
 *
 * See `mln_map_set_image_source_url` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source named
 *   `source_id` isn't an image source.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_image_source_url_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* url,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_image_source_url_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_image_source_url_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_image_source_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_projection_mode_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets whether the map draws in perspective or axonometric projection, and how
 * much it skews the view.
 *
 * Settings that `options` doesn't set keep their current values.
 *
 * See `mln_map_set_projection_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_projection_mode_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiProjectionModeOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_projection_mode_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_projection_mode_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_projection_mode_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_rendering_stats_view_enabled_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Shows or hides an overlay of rendering statistics on the map.
 *
 * See `mln_map_set_rendering_stats_view_enabled` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_rendering_stats_view_enabled_async(
  MaplibreNativeFfiMap* self, gboolean enabled, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_rendering_stats_view_enabled_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_rendering_stats_view_enabled_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_rendering_stats_view_enabled_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_image_async:
 * @image_id: the UTF-8 text
 * @options: (nullable): the value, or %NULL
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds an image to the map's style, or replaces the image named `image_id`.
 *
 * See `mln_map_set_style_image` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_image_async(
  MaplibreNativeFfiMap* self, const char* image_id,
  MaplibreNativeFfiPremultipliedRgba8Image* image,
  MaplibreNativeFfiStyleImageOptions* options, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_image_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_image_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_json_async:
 * @json: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Loads the map's style from `json`.
 *
 * The command completes once the style has parsed, and the map's style is then
 * loaded, as `maplibre_native_ffi_map_set_style_url_async()` describes. The
 * style's sources, sprites, and glyphs load in the background.
 *
 * See `mln_map_set_style_json` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `json` isn't a valid
 *   style. The map keeps its current style.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_json_async(
  MaplibreNativeFfiMap* self, const char* json, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_json_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_json_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_layer_property_async:
 * @layer_id: the UTF-8 text
 * @property_name: the UTF-8 text
 * @value: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets a property of a layer in the map's style.
 *
 * `property_name` is one of the layer's layout or paint
 * \[properties\](https://maplibre.org/maplibre-style-spec/layers/), or
 * `filter`, `minzoom`, `maxzoom`, `source`, or `source-layer`. A null `value`
 * resets any of these but `source` and `source-layer` to its default.
 *
 * See `mln_map_set_style_layer_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no layer named
 *   `layer_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_layer_property_async(
  MaplibreNativeFfiMap* self, const char* layer_id, const char* property_name,
  const char* value, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_layer_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_layer_property_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_layer_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_light_property_async:
 * @property_name: the UTF-8 text
 * @value: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets a property of the light of the map's style.
 *
 * See `mln_map_set_style_light_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_light_property_async(
  MaplibreNativeFfiMap* self, const char* property_name, const char* value,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_light_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_light_property_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_light_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_root_property_async:
 * @property_name: the UTF-8 text
 * @value: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets a \[root property\](https://maplibre.org/maplibre-style-spec/root/) of
 * the map's style, such as `light`, `sky`, `projection`, or `terrain`, without
 * reloading the style.
 *
 * `property_name` is a root property other than `sources` and `layers`, which
 * the source and layer calls change. A null `value` resets the property to its
 * default, as if the style didn't set it.
 *
 * See `mln_map_set_style_root_property` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `property_name` isn't a
 *   root property, is `sources` or `layers`, or when `value` isn't a valid
 *   value for it.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the library can't change the
 *   property while the style is loaded. Load a style that sets it instead.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_root_property_async(
  MaplibreNativeFfiMap* self, const char* property_name, const char* value,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_root_property_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_root_property_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_root_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_source_volatile_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets whether a source is volatile.
 *
 * The map doesn't store the tiles of a volatile source in the persistent cache.
 *
 * See `mln_map_set_style_source_volatile` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the style has no source named
 *   `source_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_source_volatile_async(
  MaplibreNativeFfiMap* self, const char* source_id, gboolean is_volatile,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_source_volatile_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_source_volatile_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_source_volatile_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_transition_options_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets the default transition of the map's style.
 *
 * See `mln_map_set_style_transition_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while the style isn't loaded, as
 *   `maplibre_native_ffi_map_set_style_url_async()` describes.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_transition_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiStyleTransitionOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_transition_options_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_transition_options_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_transition_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_style_url_async:
 * @url: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Loads the map's style from `url`.
 *
 * The command completes once the style document has loaded and parsed. The
 * style's sources, sprites, and glyphs then load in the background.
 *
 * The map's style is loaded from the time a style document parses until the
 * next call to this function. While the style isn't loaded, the commands that
 * change it complete with `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE`. To change
 * a new map's style without a style document, first load an empty style, such
 * as `{"version":8,"sources":{},"layers":[]}`.
 *
 * See `mln_map_set_style_url` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the style document fails to
 *   load or isn't a valid style.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_CANCELLED` when the map is released, or this
 *   function or `maplibre_native_ffi_map_set_style_json_async()` loads another
 *   style, before the style document parses.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_style_url_async(
  MaplibreNativeFfiMap* self, const char* url, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_style_url_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_style_url_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_tile_options_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets how the map chooses which tiles to load, and at what detail.
 *
 * Settings that `options` doesn't set keep their current values.
 *
 * See `mln_map_set_tile_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_tile_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiMapTileOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_tile_options_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_tile_options_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_tile_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_set_viewport_options_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sets how the map orients and constrains its view.
 *
 * Settings that `options` doesn't set keep their current values.
 *
 * See `mln_map_set_viewport_options` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_set_viewport_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiMapViewportOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_set_viewport_options_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_set_viewport_options_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_viewport_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_update_camera_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Moves the camera, at once or with an animation.
 *
 * Camera fields that `update` doesn't set keep their values, and the update
 * stops any running animation of the fields it sets. The function finishes when
 * the map applies the update, before an animation ends. To learn when an
 * animation ends, set an `MaplibreNativeFfiCameraTransitionHandler` in its
 * animation options.
 *
 * See `mln_map_update_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_map_update_camera_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiCameraUpdate* update,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_update_camera_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_map_update_camera_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_update_camera_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_ready_async:
 * @callback: (scope async) (nullable): the callback to call when the handle is ready
 * @user_data: the data to pass to `callback`
 *
 * Waits until the handle is ready, as the call that created it reports. A
 * handle that a callback lends is ready at once.
 *
 * Runs on the thread-default main context of the thread that started the call.
 */
void maplibre_native_ffi_map_ready_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_map_ready_finish:
 * @result: the result that the callback received
 *
 * Finishes a wait that `maplibre_native_ffi_map_ready_async` started.
 *
 * Returns: %TRUE when the handle is ready, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_map_ready_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_map_is_event_source:
 * @event: the event
 *
 * Whether a runtime event comes from this handle. A handle's ID needs all of
 * its 64 bits, so compare with this, rather than with
 * `maplibre_native_ffi_map_get_id`, where numbers are doubles, as in
 * JavaScript.
 *
 * Returns: whether the event's source is this handle
 */
gboolean maplibre_native_ffi_map_is_event_source(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiRuntimeEvent* event
);

/**
 * MaplibreNativeFfiMapProjection:
 *
 * A camera view for converting between screen points and coordinates.
 *
 * See `mln_map_projection` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_PROJECTION \
  (maplibre_native_ffi_map_projection_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiMapProjection, maplibre_native_ffi_map_projection,
  MAPLIBRE_NATIVE_FFI, MAP_PROJECTION, GObject
)

/**
 * maplibre_native_ffi_map_projection_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_map_projection_get_id(
  MaplibreNativeFfiMapProjection* self
);

/**
 * maplibre_native_ffi_map_projection_create: (constructor)
 *
 * Creates a projection that converts coordinates for one view of a map.
 *
 * Create it from the `view` of an `MaplibreNativeFfiMapSnapshot` or an
 * `MaplibreNativeFfiRenderFrameResult`. The projection doesn't follow the map,
 * so create a new one after the map's camera or size changes.
 *
 * See `mln_map_projection_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the object
 */
MaplibreNativeFfiMapProjection* maplibre_native_ffi_map_projection_create(
  const MaplibreNativeFfiMapView* view, GError** error
);

/**
 * maplibre_native_ffi_map_projection_get_camera:
 *
 * Reads the projection's camera.
 *
 * See `mln_map_projection_get_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiMapProjectionCameraResult*
maplibre_native_ffi_map_projection_get_camera(
  MaplibreNativeFfiMapProjection* self, GError** error
);

/**
 * maplibre_native_ffi_map_projection_lat_lng_for_pixel:
 *
 * Finds the coordinate at a screen point.
 *
 * The longitude stays between -180 and 180.
 *
 * See `mln_map_projection_lat_lng_for_pixel` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiLatLng* maplibre_native_ffi_map_projection_lat_lng_for_pixel(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* point, GError** error
);

/**
 * maplibre_native_ffi_map_projection_lat_lng_for_pixel_unwrapped:
 *
 * Finds the coordinate at a screen point, without wrapping its longitude.
 *
 * The longitude can fall outside -180 to 180, to show which copy of the world
 * the point is in.
 *
 * See `mln_map_projection_lat_lng_for_pixel_unwrapped` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiLatLng*
maplibre_native_ffi_map_projection_lat_lng_for_pixel_unwrapped(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* point, GError** error
);

/**
 * maplibre_native_ffi_map_projection_lat_lngs_for_pixels:
 * @points: (array length=point_count): the items
 *
 * Finds the coordinates at several screen points, as
 * `maplibre_native_ffi_map_projection_lat_lng_for_pixel()` does.
 *
 * See `mln_map_projection_lat_lngs_for_pixels` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiLatLng): the items
 */
GPtrArray* maplibre_native_ffi_map_projection_lat_lngs_for_pixels(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* points, gsize point_count, GError** error
);

/**
 * maplibre_native_ffi_map_projection_lat_lngs_for_pixels_unwrapped:
 * @points: (array length=point_count): the items
 *
 * Finds the coordinates at several screen points, as
 * `maplibre_native_ffi_map_projection_lat_lng_for_pixel_unwrapped()` does.
 *
 * See `mln_map_projection_lat_lngs_for_pixels_unwrapped` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiLatLng): the items
 */
GPtrArray* maplibre_native_ffi_map_projection_lat_lngs_for_pixels_unwrapped(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* points, gsize point_count, GError** error
);

/**
 * maplibre_native_ffi_map_projection_meters_per_pixel_at_latitude:
 *
 * Computes how many meters one screen pixel covers at `latitude`, at the
 * projection's zoom.
 *
 * The result ignores the camera's pitch.
 *
 * See `mln_map_projection_meters_per_pixel_at_latitude` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: the value
 */
gdouble maplibre_native_ffi_map_projection_meters_per_pixel_at_latitude(
  MaplibreNativeFfiMapProjection* self, gdouble latitude, GError** error
);

/**
 * maplibre_native_ffi_map_projection_pixel_for_lat_lng:
 *
 * Finds the screen point of a coordinate.
 *
 * The point is in the copy of the world nearest the camera's center.
 *
 * See `mln_map_projection_pixel_for_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiScreenPoint*
maplibre_native_ffi_map_projection_pixel_for_lat_lng(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiLatLng* coordinate, GError** error
);

/**
 * maplibre_native_ffi_map_projection_pixels_for_lat_lngs:
 * @coordinates: (array length=coordinate_count): the items
 *
 * Finds the screen points of several coordinates, as
 * `maplibre_native_ffi_map_projection_pixel_for_lat_lng()` does.
 *
 * See `mln_map_projection_pixels_for_lat_lngs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiScreenPoint): the items
 */
GPtrArray* maplibre_native_ffi_map_projection_pixels_for_lat_lngs(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiLatLng* coordinates, gsize coordinate_count,
  GError** error
);

/**
 * maplibre_native_ffi_map_projection_release:
 *
 * Releases a projection.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_map_projection_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 */
void maplibre_native_ffi_map_projection_release(
  MaplibreNativeFfiMapProjection* self
);

/**
 * maplibre_native_ffi_map_projection_set_camera:
 *
 * Moves the projection's camera, without changing the map.
 *
 * Camera fields that `camera` doesn't set keep their values.
 *
 * See `mln_map_projection_set_camera` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_map_projection_set_camera(
  MaplibreNativeFfiMapProjection* self, MaplibreNativeFfiCameraOptions* camera,
  GError** error
);

/**
 * MaplibreNativeFfiOffscreenDevice:
 *
 * A GPU device that the library creates to render maps offscreen.
 *
 * See `mln_offscreen_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFSCREEN_DEVICE \
  (maplibre_native_ffi_offscreen_device_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiOffscreenDevice, maplibre_native_ffi_offscreen_device,
  MAPLIBRE_NATIVE_FFI, OFFSCREEN_DEVICE, GObject
)

/**
 * maplibre_native_ffi_offscreen_device_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_offscreen_device_get_id(
  MaplibreNativeFfiOffscreenDevice* self
);

/**
 * maplibre_native_ffi_offscreen_device_create: (constructor)
 * @options: (nullable): the value, or %NULL
 *
 * Creates a GPU device for rendering maps offscreen.
 *
 * `maplibre_native_ffi_map_attach_offscreen()` creates a device of its own when
 * it gets none. Create one to share it among the sessions on many maps, or to
 * pick a GPU.
 *
 * In a browser WebGPU build, attach without a device instead.
 *
 * See `mln_offscreen_device_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` in a browser WebGPU build, or when
 *   the device can't render with `client_api`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when `adapter_index` names no GPU,
 *   or the platform can't create the device.
 *
 * Returns: (transfer full): the object
 */
MaplibreNativeFfiOffscreenDevice* maplibre_native_ffi_offscreen_device_create(
  MaplibreNativeFfiOffscreenDeviceOptions* options, GError** error
);

/**
 * maplibre_native_ffi_offscreen_device_release:
 *
 * Releases an offscreen device.
 *
 * Render sessions on the device keep working until they detach.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_offscreen_device_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
 */
void maplibre_native_ffi_offscreen_device_release(
  MaplibreNativeFfiOffscreenDevice* self
);

/**
 * MaplibreNativeFfiRenderSession:
 *
 * A render session draws a map into a surface or a texture.
 *
 * See `mln_render_session` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION \
  (maplibre_native_ffi_render_session_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiRenderSession, maplibre_native_ffi_render_session,
  MAPLIBRE_NATIVE_FFI, RENDER_SESSION, GObject
)

/**
 * maplibre_native_ffi_render_session_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_render_session_get_id(
  MaplibreNativeFfiRenderSession* self
);

/**
 * maplibre_native_ffi_render_session_abandon:
 *
 * Stops the session right away, without running its pending graphics work.
 *
 * Use it when the target's graphics objects are going away and a detach can't
 * finish first. Pending work completes with
 * `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST`, and pending frame demands finish
 * with `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_TARGET_NOT_READY`. Once the call
 * returns, the library no longer uses your graphics objects, and you can attach
 * another session to the map. Release the session afterward.
 *
 * Abandon keeps the session's OpenGL and WebGPU objects until the process
 * exits, and reports how many it kept. Detach the session instead to free them.
 *
 * See `mln_render_session_abandon` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiRenderAbandonResult*
maplibre_native_ffi_render_session_abandon(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_acquire_frame:
 *
 * Takes the newest rendered frame from the session's texture ring, if one is
 * ready.
 *
 * Older frames that you haven't acquired are dropped, and the session can
 * render over a frame before you acquire it. While you hold a frame in every
 * texture of the ring, new frames wait until you release one. When no frame is
 * ready, it succeeds and writes the null handle.
 *
 * See `mln_render_session_acquire_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the session lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` or
 *   `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR`, as
 *   `MaplibreNativeFfiRenderSessionSnapshot.failure_status` reports, when the
 *   session has failed.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when you detached, abandoned,
 *   released, or disposed the session.
 *
 * Returns: (transfer full) (nullable): the object
 */
MaplibreNativeFfiAcquiredFrame*
maplibre_native_ffi_render_session_acquire_frame(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_barrier_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Ends the session's pending frame demands and waits for its earlier work.
 *
 * Frame demands that haven't started, including those that wait for a map
 * update, finish with `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_NO_UPDATE`.
 *
 * See `mln_render_session_barrier` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the barrier runs.
 */
void maplibre_native_ffi_render_session_barrier_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_barrier_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_barrier_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_barrier_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_clear_data_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Clears the data that the session's renderer holds, such as loaded tiles, and
 * draws the map again.
 *
 * See `mln_render_session_clear_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the work runs.
 */
void maplibre_native_ffi_render_session_clear_data_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_clear_data_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_clear_data_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_clear_data_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_demand_frame:
 *
 * Asks the session to render a frame.
 *
 * The frame shows the map commands that you sent before the call, except the
 * changes that an open command group holds back.
 *
 * You can call it once the attach call returns. While the attach runs, the
 * demand waits for it to complete. Every demand finishes with one frame result
 * that carries its token, and
 * `maplibre_native_ffi_render_session_drain_frame_results()` takes it. A demand
 * without `MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_FORCE` replaces each pending demand
 * with the same flags.
 *
 * See `mln_render_session_demand_frame` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` or
 *   `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR`, as
 *   `MaplibreNativeFfiRenderSessionSnapshot.failure_status` reports, when the
 *   session has failed.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when you detached, abandoned,
 *   released, or disposed the session, or discarded a frame from every texture
 *   of its ring.
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_demand_frame(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiFrameDemand* demand, GError** error
);

/**
 * maplibre_native_ffi_render_session_detach_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Detaches the session from its map and frees its graphics objects.
 *
 * Release every acquired frame first. Pending frame demands finish with
 * `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_TARGET_NOT_READY`. Once the detach
 * completes, the library no longer uses your graphics objects, and you can
 * attach another session to the map. Release the session afterward.
 *
 * See `mln_render_session_detach` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the detach runs.
 */
void maplibre_native_ffi_render_session_detach_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_detach_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_detach_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_detach_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_dispose:
 *
 * Releases the session in any state, and finishes in the background.
 *
 * Use it from destructors and finalizers, and
 * `maplibre_native_ffi_render_session_release_async()` elsewhere. It ends the
 * session as a release does, except that it abandons a session whose frames you
 * hold, which ends those frames. Keep servicing a session driven by
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD` until
 * `maplibre_native_ffi_render_session_service_driver_work()` no longer accepts
 * it.
 *
 * The object calls this when it is disposed, unless its handle was released.
 * Later calls do nothing.
 *
 * See `mln_render_session_dispose` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
void maplibre_native_ffi_render_session_dispose(
  MaplibreNativeFfiRenderSession* self
);

/**
 * maplibre_native_ffi_render_session_drain_frame_results:
 *
 * Takes the frame results that the session has queued.
 *
 * When no result is queued, it succeeds and writes the null handle and an empty
 * view.
 *
 * See `mln_render_session_drain_frame_results` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiRenderFrameResult): the items, or %NULL with `error` set
 */
GPtrArray* maplibre_native_ffi_render_session_drain_frame_results(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_dump_debug_logs_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Logs debugging information about the session's renderer.
 *
 * See `mln_render_session_dump_debug_logs` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the work runs.
 */
void maplibre_native_ffi_render_session_dump_debug_logs_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_dump_debug_logs_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_dump_debug_logs_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_dump_debug_logs_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_get_capabilities:
 *
 * Reads what the session supports.
 *
 * See `mln_render_session_get_capabilities` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiRenderSessionCapabilities*
maplibre_native_ffi_render_session_get_capabilities(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_get_cluster_children_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the children of a cluster in a clustered GeoJSON source.
 *
 * `cluster_id` is the "cluster_id" property of a cluster feature. The result is
 * a GeoJSON feature collection of the clusters and points that the cluster
 * splits into at the next zoom level.
 *
 * The query reads the source's data from the session's most recent render
 * attempt, so a change to the data shows once the session tries to render
 * again.
 *
 * A cluster ID belongs to one version of the source's data. After the data
 * changes, feature queries can return IDs from the earlier version until the
 * source's tiles reload, and such an ID completes with
 * `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` or names a different cluster. Query the
 * features again for current IDs.
 *
 * See `mln_render_session_get_cluster_children` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the session's most recent render
 *   attempt had no source named `source_id`, or the source has no cluster
 *   `cluster_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source isn't a
 *   GeoJSON source with clustering turned on.
 */
void maplibre_native_ffi_render_session_get_cluster_children_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  guint32 cluster_id, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_get_cluster_children_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_get_cluster_children_async` started.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_render_session_get_cluster_children_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_get_cluster_expansion_zoom_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the zoom level at which a cluster of a clustered GeoJSON source splits.
 *
 * `cluster_id` is the "cluster_id" property of a cluster feature. At the
 * result's zoom level, the cluster splits into two or more clusters or points.
 *
 * A cluster ID can go out of date as
 * `maplibre_native_ffi_render_session_get_cluster_children_async()` describes.
 *
 * See `mln_render_session_get_cluster_expansion_zoom` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the session's most recent render
 *   attempt had no source named `source_id`, or the source has no cluster
 *   `cluster_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source isn't a
 *   GeoJSON source with clustering turned on.
 */
void maplibre_native_ffi_render_session_get_cluster_expansion_zoom_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  guint32 cluster_id, GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_get_cluster_expansion_zoom_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_get_cluster_expansion_zoom_async` started.
 *
 * Returns: the result, or 0 with `error` set
 */
guint32 maplibre_native_ffi_render_session_get_cluster_expansion_zoom_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_get_cluster_leaves_async:
 * @source_id: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the points in a cluster of a clustered GeoJSON source.
 *
 * `cluster_id` is the "cluster_id" property of a cluster feature. The result is
 * a GeoJSON feature collection of the original point features. It skips the
 * first `offset` points and then holds up to `limit` of them, so a `limit` of 0
 * gives an empty collection.
 *
 * A cluster ID can go out of date as
 * `maplibre_native_ffi_render_session_get_cluster_children_async()` describes.
 *
 * See `mln_render_session_get_cluster_leaves` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the session's most recent render
 *   attempt had no source named `source_id`, or the source has no cluster
 *   `cluster_id`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the source isn't a
 *   GeoJSON source with clustering turned on.
 */
void maplibre_native_ffi_render_session_get_cluster_leaves_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  guint32 cluster_id, guint32 limit, guint32 offset,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_get_cluster_leaves_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_get_cluster_leaves_async` started.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_render_session_get_cluster_leaves_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_get_failure:
 *
 * Reads the message of the error that made the session fail.
 *
 * `MaplibreNativeFfiRenderSessionSnapshot.failure_status` reports the error's
 * status. The message is empty while the session hasn't failed, and doesn't
 * change once it fails.
 *
 * See `mln_render_session_get_failure` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_render_session_get_failure(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_get_snapshot:
 *
 * Reads the session's current state.
 *
 * See `mln_render_session_get_snapshot` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiRenderSessionSnapshot*
maplibre_native_ffi_render_session_get_snapshot(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_query_rendered_features_async:
 * @options: (nullable): the value, or %NULL
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Finds the features that the session drew in an area of the screen.
 *
 * The query reads the last frame that the session rendered. A feature that
 * several layers draw appears once for each layer.
 *
 * See `mln_render_session_query_rendered_features` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
void maplibre_native_ffi_render_session_query_rendered_features_async(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiRenderedQueryGeometry* geometry,
  MaplibreNativeFfiRenderedFeatureQueryOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_query_rendered_features_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_query_rendered_features_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiRenderedFeature): the items
 */
GPtrArray* maplibre_native_ffi_render_session_query_rendered_features_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_query_source_features_async:
 * @source_id: the UTF-8 text
 * @options: (nullable): the value, or %NULL
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Finds the features of the source named `source_id`.
 *
 * The query covers only the tiles that the source has loaded for the current
 * view. A source loads tiles only while a visible layer uses it.
 *
 * See `mln_render_session_query_source_features` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
 */
void maplibre_native_ffi_render_session_query_source_features_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  MaplibreNativeFfiSourceFeatureQueryOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_query_source_features_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_query_source_features_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiSourceFeature): the items
 */
GPtrArray* maplibre_native_ffi_render_session_query_source_features_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_read_texture_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the pixels of the last frame that the session rendered.
 *
 * See `mln_render_session_read_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the session's capabilities
 *   lack `MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_READBACK`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when the session hasn't rendered
 *   a frame since it attached, resized, or changed its target.
 */
void maplibre_native_ffi_render_session_read_texture_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_read_texture_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_read_texture_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_render_session_read_texture_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_reduce_memory_use_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Frees memory that the session's renderer can rebuild, such as caches.
 *
 * See `mln_render_session_reduce_memory_use` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the work runs.
 */
void maplibre_native_ffi_render_session_reduce_memory_use_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_reduce_memory_use_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_reduce_memory_use_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_reduce_memory_use_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_release_async:
 * @callback: (scope async) (nullable): the callback to call when the release finishes
 * @user_data: the data to pass to `callback`
 *
 * Releases the session, detaching it first when it is attached.
 *
 * Pending frame demands finish with
 * `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_TARGET_NOT_READY`. Once the release
 * completes, the library no longer uses your graphics objects, and you can
 * attach another session to the map.
 *
 * A session driven by
 * `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD` detaches at your
 * next `maplibre_native_ffi_render_session_service_driver_work()`, so keep
 * servicing it until the release completes. When the session isn't attached,
 * such as while it attaches or after it fails, or when you release or dispose
 * its map before that service, the release abandons it as
 * `maplibre_native_ffi_render_session_abandon()` does.
 *
 * See `mln_render_session_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while you hold one of the
 *   session's acquired frames. Release or discard them first.
 */
void maplibre_native_ffi_render_session_release_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_release_finish:
 * @result: the result that the callback received
 *
 * Finishes a release that `maplibre_native_ffi_render_session_release_async` started.
 *
 * Returns: %TRUE when the handle retired, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_release_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_release_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Lets go of the session's surface or textures, and keeps the rest of the
 * session.
 *
 * Use it when the platform takes a surface away, such as when your app goes to
 * the background. Release every acquired frame first. Frame demands that are
 * still pending, and later ones until you give the session a new target, finish
 * with `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_TARGET_NOT_READY`. Once the release
 * completes, you can destroy the old target.
 *
 * See `mln_render_session_release_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the session renders offscreen
 *   or into Android hardware buffers, dmabufs, DXGI shared textures, or
 *   IOSurfaces that it allocates.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the target is a WebGL canvas.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the release runs.
 */
void maplibre_native_ffi_render_session_release_target_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_release_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_release_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_release_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_resize_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Resizes the session's target and its map.
 *
 * The session renders at the map's scale factor. Release every acquired frame
 * first. You can call it once the attach call returns. While the attach runs,
 * the resize waits for it to complete.
 *
 * See `mln_render_session_resize` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the session renders into a
 *   borrowed ring. Give it new textures of the new size instead.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` while you hold one of the
 *   session's acquired frames, or when the session failed, or you detached,
 *   abandoned, released, or disposed it.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` when the session loses its target
 *   before the resize applies.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_render_session_resize_async(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiLogicalSize* logical_size,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_resize_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_resize_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_render_session_resize_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_service_driver_work:
 *
 * Runs the session's ready graphics work on your graphics thread.
 *
 * Use it with `MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD`, and
 * call it from the same thread every time. Call it each time
 * `MaplibreNativeFfiRenderSessionAttachOptions:driver-work-wake` is called,
 * even while you demand no frames, until a detach, release, or disposal of the
 * session finishes. It returns once no work is ready.
 *
 * It accepts a released or disposed session until that release or disposal
 * finishes.
 *
 * See `mln_render_session_service_driver_work` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_service_driver_work(
  MaplibreNativeFfiRenderSession* self, GError** error
);

/**
 * maplibre_native_ffi_render_session_set_frame_pacer:
 * @pacer: (nullable): the handle, or %NULL
 *
 * Sets the frame pacer that demands the session's frames.
 *
 * At the first display refresh after the session has something new to draw,
 * `pacer` demands a frame without flags and with token 0. One pacer can serve
 * many sessions. A null `pacer` stops pacing the session. You can call it once
 * the attach call returns.
 *
 * See `mln_render_session_set_frame_pacer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST` or
 *   `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR`, as
 *   `MaplibreNativeFfiRenderSessionSnapshot.failure_status` reports, when the
 *   session has failed.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when you attached the session
 *   with a redraw wake, or detached, abandoned, released, or disposed it.
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_set_frame_pacer(
  MaplibreNativeFfiRenderSession* self, MaplibreNativeFfiFramePacer* pacer,
  GError** error
);

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the Metal textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * `maplibre_native_ffi_render_session_release_target_async()`. Release every
 * acquired frame first. Once the replacement completes, the session no longer
 * uses the old textures.
 *
 * See `mln_render_session_set_metal_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the new textures have a
 *   different pixel format than the old ones. Attach a new session to change
 *   it.
 */
void maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiMetalBorrowedTextureReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean
maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_render_session_set_metal_surface_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the Metal layer that the session renders to.
 *
 * Use it to give the session a layer again after
 * `maplibre_native_ffi_render_session_release_target_async()`, to move it to
 * another layer, or to resize it. Once the replacement completes, the session
 * no longer uses the old layer.
 *
 * See `mln_render_session_set_metal_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
void maplibre_native_ffi_render_session_set_metal_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiMetalSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);
#endif

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_render_session_set_metal_surface_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_metal_surface_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_set_metal_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);
#endif

/**
 * maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the OpenGL textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * `maplibre_native_ffi_render_session_release_target_async()`. Release every
 * acquired frame first. Once the replacement completes, the session no longer
 * uses the old textures.
 *
 * See `mln_render_session_set_open_gl_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
void maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean
maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_set_open_gl_surface_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the OpenGL surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * `maplibre_native_ffi_render_session_release_target_async()`, to move it to
 * another surface, or to resize it. Once the replacement completes, the session
 * no longer uses the old surface.
 *
 * See `mln_render_session_set_open_gl_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
void maplibre_native_ffi_render_session_set_open_gl_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiOpenGlSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_set_open_gl_surface_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_open_gl_surface_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_set_open_gl_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the Vulkan images that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session images again after
 * `maplibre_native_ffi_render_session_release_target_async()`. Release every
 * acquired frame first. Once the replacement completes, the session no longer
 * uses the old images.
 *
 * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
void maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean
maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_render_session_set_vulkan_surface_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the Vulkan surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * `maplibre_native_ffi_render_session_release_target_async()`, to move it to
 * another surface, or to resize it. Once the replacement completes, the session
 * no longer uses the old surface.
 *
 * See `mln_render_session_set_vulkan_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the new surface needs a
 *   different color format or pre-rotation support than the old one. Attach a
 *   new session instead.
 */
void maplibre_native_ffi_render_session_set_vulkan_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiVulkanSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_set_vulkan_surface_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_vulkan_surface_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_set_vulkan_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

#if defined(__linux__)
/**
 * maplibre_native_ffi_render_session_set_wayland_surface_target_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the Wayland surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * `maplibre_native_ffi_render_session_release_target_async()`, to move it to
 * another surface, or to resize it. Once the replacement completes, the session
 * no longer uses the old surface.
 *
 * See `mln_render_session_set_wayland_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when, in a Vulkan build, the new
 *   surface needs a different color format than the old one. Attach a new
 *   session instead.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the library can't present to
 *   the new surface.
 */
void maplibre_native_ffi_render_session_set_wayland_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiWaylandSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
);
#endif

#if defined(__linux__)
/**
 * maplibre_native_ffi_render_session_set_wayland_surface_target_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_render_session_set_wayland_surface_target_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_set_wayland_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);
#endif

/**
 * maplibre_native_ffi_render_session_ready_async:
 * @callback: (scope async) (nullable): the callback to call when the handle is ready
 * @user_data: the data to pass to `callback`
 *
 * Waits until the handle is ready, as the call that created it reports. A
 * handle that a callback lends is ready at once.
 *
 * Runs on the thread-default main context of the thread that started the call.
 */
void maplibre_native_ffi_render_session_ready_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_render_session_ready_finish:
 * @result: the result that the callback received
 *
 * Finishes a wait that `maplibre_native_ffi_render_session_ready_async` started.
 *
 * Returns: %TRUE when the handle is ready, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_render_session_ready_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
);

/**
 * MaplibreNativeFfiResourceRequest:
 *
 * A resource load that a resource provider answers.
 *
 * See `mln_resource_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_REQUEST \
  (maplibre_native_ffi_resource_request_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiResourceRequest, maplibre_native_ffi_resource_request,
  MAPLIBRE_NATIVE_FFI, RESOURCE_REQUEST, GObject
)

/**
 * maplibre_native_ffi_resource_request_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_resource_request_get_id(
  MaplibreNativeFfiResourceRequest* self
);

/**
 * maplibre_native_ffi_resource_request_complete:
 *
 * Answers a resource request.
 *
 * A failed answer fails the request with
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER`, as
 * `maplibre_native_ffi_resource_request_release()` does.
 *
 * See `mln_resource_request_complete` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when the library cancelled the
 *   request.
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_resource_request_complete(
  MaplibreNativeFfiResourceRequest* self,
  MaplibreNativeFfiResourceResponse* response, GError** error
);

/**
 * maplibre_native_ffi_resource_request_is_cancelled:
 *
 * Reads whether the library cancelled a resource request.
 *
 * See `mln_resource_request_is_cancelled` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_resource_request_is_cancelled(
  MaplibreNativeFfiResourceRequest* self, GError** error
);

/**
 * maplibre_native_ffi_resource_request_pass_through:
 *
 * Hands a resource request back to the library to load as usual.
 *
 * A failed call fails the request with
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER`, as
 * `maplibre_native_ffi_resource_request_release()` does.
 *
 * See `mln_resource_request_pass_through` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when the library cancelled the
 *   request.
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_resource_request_pass_through(
  MaplibreNativeFfiResourceRequest* self, GError** error
);

/**
 * maplibre_native_ffi_resource_request_release:
 *
 * Releases a resource request.
 *
 * Releasing a request without an answer fails it with
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER`.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_resource_request_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_resource_request_release(
  MaplibreNativeFfiResourceRequest* self
);

/**
 * maplibre_native_ffi_resource_request_set_cancel_callback:
 *
 * Calls a callback when the library cancels a resource request.
 *
 * The callback never runs after the request is answered. A request takes one
 * cancel callback. When the library has already cancelled the request, the call
 * reports that instead and the callback never runs.
 *
 * See `mln_resource_request_set_cancel_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: the value
 */
gboolean maplibre_native_ffi_resource_request_set_cancel_callback(
  MaplibreNativeFfiResourceRequest* self,
  const MaplibreNativeFfiResourceRequestCancelHandler* handler, GError** error
);

/**
 * MaplibreNativeFfiResourceTransformRequest:
 *
 * A URL rewrite that a resource transform answers with the URL to load.
 *
 * See `mln_resource_transform_request` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_REQUEST \
  (maplibre_native_ffi_resource_transform_request_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiResourceTransformRequest,
  maplibre_native_ffi_resource_transform_request, MAPLIBRE_NATIVE_FFI,
  RESOURCE_TRANSFORM_REQUEST, GObject
)

/**
 * maplibre_native_ffi_resource_transform_request_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_resource_transform_request_get_id(
  MaplibreNativeFfiResourceTransformRequest* self
);

/**
 * maplibre_native_ffi_resource_transform_request_complete:
 * @url: the UTF-8 text
 *
 * Answers a resource transform request with the URL to load instead.
 *
 * `url` must not be empty. A failed answer fails the request with
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER`, as
 * `maplibre_native_ffi_resource_transform_request_release()` does.
 *
 * See `mln_resource_transform_request_complete` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_resource_transform_request_complete(
  MaplibreNativeFfiResourceTransformRequest* self, const char* url,
  GError** error
);

/**
 * maplibre_native_ffi_resource_transform_request_fail:
 *
 * Fails a resource transform request without sending it.
 *
 * The library retries a request that failed for a reason it retries, such as
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_CONNECTION`, and the retry calls
 * the transform again. When this call fails, the request fails with
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER`, as
 * `maplibre_native_ffi_resource_transform_request_release()` does.
 *
 * See `mln_resource_transform_request_fail` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_resource_transform_request_fail(
  MaplibreNativeFfiResourceTransformRequest* self,
  MaplibreNativeFfiResourceTransformError* error_value, GError** error
);

/**
 * maplibre_native_ffi_resource_transform_request_release:
 *
 * Releases a resource transform request.
 *
 * Releasing a request without an answer fails it with
 * `MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER`.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_resource_transform_request_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_resource_transform_request_release(
  MaplibreNativeFfiResourceTransformRequest* self
);

/**
 * MaplibreNativeFfiRuntime:
 *
 * A runtime owns maps and the network and cache settings that they share.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME \
  (maplibre_native_ffi_runtime_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiRuntime, maplibre_native_ffi_runtime, MAPLIBRE_NATIVE_FFI,
  RUNTIME, GObject
)

/**
 * maplibre_native_ffi_runtime_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_runtime_get_id(MaplibreNativeFfiRuntime* self);

/**
 * maplibre_native_ffi_runtime_clear_event_handler_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Stops sending the events of the runtime and its maps to a callback.
 *
 * See `mln_runtime_clear_event_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_clear_event_handler_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_clear_event_handler_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_clear_event_handler_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_event_handler_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_clear_http_header_transform_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Stops adding headers to the runtime's HTTP requests.
 *
 * See `mln_runtime_clear_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_clear_http_header_transform_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_clear_http_header_transform_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_clear_http_header_transform_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_http_header_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_clear_resource_provider_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Removes the runtime's resource provider, so the library loads every request
 * itself.
 *
 * See `mln_runtime_clear_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_clear_resource_provider_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_clear_resource_provider_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_clear_resource_provider_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_resource_provider_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_clear_resource_transform_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Stops rewriting the URLs of the runtime's network requests.
 *
 * See `mln_runtime_clear_resource_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_clear_resource_transform_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_clear_resource_transform_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_clear_resource_transform_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_resource_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_create: (constructor)
 * @options: (nullable): the value, or %NULL
 *
 * Creates a runtime.
 *
 * A runtime holds maps and the resource loading, cache, and offline regions
 * that they share. In a browser, create runtimes on a Web Worker, not on the
 * main thread.
 *
 * See `mln_runtime_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the cache database file
 *   that `options` names can't be opened or created.
 *
 * Returns: (transfer full): the object
 */
MaplibreNativeFfiRuntime* maplibre_native_ffi_runtime_create(
  MaplibreNativeFfiRuntimeOptions* options, GError** error
);

/**
 * maplibre_native_ffi_runtime_create_map:
 *
 * Creates a map on `runtime`.
 *
 * Asynchronous calls on the map before the completion runs wait for it, and
 * complete with `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE` when creation fails.
 *
 * The object is ready once `maplibre_native_ffi_map_ready_async` finishes,
 * which fails with the errors that the completion reports.
 *
 * See `mln_runtime_create_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the `fallback_font_faces`
 *   of `options` isn't an array of font files.
 *
 * Returns: (transfer full): the new object, or %NULL with `error` set
 */
MaplibreNativeFfiMap* maplibre_native_ffi_runtime_create_map(
  MaplibreNativeFfiRuntime* self, MaplibreNativeFfiMapOptions* options,
  GError** error
);

/**
 * maplibre_native_ffi_runtime_create_offline_region_async:
 * @metadata: the bytes
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Creates an offline region in the runtime's cache file.
 *
 * `metadata` holds any bytes to store with the region, such as its name. The
 * region doesn't download until
 * `maplibre_native_ffi_runtime_set_offline_region_download_state_async()` makes
 * its download active.
 *
 * See `mln_runtime_create_offline_region` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_runtime_create_offline_region_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiOfflineRegionDefinition* definition, GBytes* metadata,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_create_offline_region_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_create_offline_region_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_runtime_create_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_delete_offline_region_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Deletes the offline region `region_id` from the runtime's cache file.
 *
 * See `mln_runtime_delete_offline_region` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the cache file has no region
 *   `region_id`.
 */
void maplibre_native_ffi_runtime_delete_offline_region_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_delete_offline_region_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_delete_offline_region_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_delete_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_dispose:
 *
 * Releases a runtime in any state, without waiting.
 *
 * The runtime finishes its teardown after its maps are released.
 *
 * The object calls this when it is disposed, unless its handle was released.
 * Later calls do nothing.
 *
 * See `mln_runtime_dispose` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_dispose(MaplibreNativeFfiRuntime* self);

/**
 * maplibre_native_ffi_runtime_get_offline_region_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the offline region `region_id`.
 *
 * The result is null when the cache file has no region with that ID.
 *
 * See `mln_runtime_get_offline_region` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_runtime_get_offline_region_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_get_offline_region_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_get_offline_region_async` started.
 *
 * Returns: (transfer full) (nullable): the value
 */
MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_runtime_get_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_get_offline_region_status_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Reads the download progress of the offline region `region_id`.
 *
 * While the download is active, the counts cover what it has visited. Once it's
 * inactive, they cover every resource the region holds, including those that
 * only earlier versions of its style listed, so the completed counts can exceed
 * the required counts.
 *
 * See `mln_runtime_get_offline_region_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the cache file has no region
 *   `region_id`.
 */
void maplibre_native_ffi_runtime_get_offline_region_status_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_get_offline_region_status_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_get_offline_region_status_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiOfflineRegionStatus*
maplibre_native_ffi_runtime_get_offline_region_status_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_invalidate_offline_region_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Makes maps check each resource of the offline region `region_id` with the
 * server before they use it.
 *
 * The region keeps its resources. Starting its download again fetches the
 * invalidated resources for offline use. If the style or a TileJSON document is
 * then missing or invalid, the download reports an offline region response
 * error and goes on from the stored copy. Resources that the current style no
 * longer lists stay in the region until it's deleted.
 *
 * See `mln_runtime_invalidate_offline_region` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the cache file has no region
 *   `region_id`.
 */
void maplibre_native_ffi_runtime_invalidate_offline_region_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_invalidate_offline_region_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_invalidate_offline_region_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_invalidate_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_list_offline_regions_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Lists the offline regions in the runtime's cache file.
 *
 * See `mln_runtime_list_offline_regions` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
void maplibre_native_ffi_runtime_list_offline_regions_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_list_offline_regions_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_list_offline_regions_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiOfflineRegionInfo): the items
 */
GPtrArray* maplibre_native_ffi_runtime_list_offline_regions_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_merge_offline_regions_async:
 * @side_database_path: the UTF-8 text
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Copies the offline regions of another cache file into the runtime's cache
 * file.
 *
 * A region whose definition and metadata match one that the runtime's file
 * already has isn't copied again. The result lists the runtime's copy of each
 * region in the other file.
 *
 * See `mln_runtime_merge_offline_regions` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `side_database_path`
 *   isn't a readable database file.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the merge would exceed the
 *   tile limit that
 *   `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED`
 *   describes.
 */
void maplibre_native_ffi_runtime_merge_offline_regions_async(
  MaplibreNativeFfiRuntime* self, const char* side_database_path,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_merge_offline_regions_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_merge_offline_regions_async` started.
 *
 * Returns: (transfer container) (element-type MaplibreNativeFfiOfflineRegionInfo): the items
 */
GPtrArray* maplibre_native_ffi_runtime_merge_offline_regions_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_release_async:
 * @callback: (scope async) (nullable): the callback to call when the release finishes
 * @user_data: the data to pass to `callback`
 *
 * Releases a runtime.
 *
 * Release the runtime's maps first.
 *
 * See `mln_runtime_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_release_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_release_finish:
 * @result: the result that the callback received
 *
 * Finishes a release that `maplibre_native_ffi_runtime_release_async` started.
 *
 * Returns: %TRUE when the handle retired, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_release_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_run_cache_database_operation_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Runs a maintenance action on the runtime's cache database.
 *
 * See `mln_runtime_run_cache_database_operation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_run_cache_database_operation_async(
  MaplibreNativeFfiRuntime* self,
  MaplibreNativeFfiCacheDatabaseOperation operation,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_run_cache_database_operation_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_run_cache_database_operation_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_run_cache_database_operation_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_event_handler_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Sends the events of the runtime and its maps to a callback.
 *
 * The callback replaces the one set before.
 *
 * See `mln_runtime_set_event_handler` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_set_event_handler_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiRuntimeEventHandler* handler,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_event_handler_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_event_handler_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_event_handler_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_http_header_transform_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Adds headers to the runtime's HTTP requests.
 *
 * The transform replaces the one set before. In a browser or on OpenHarmony,
 * serve the requests with
 * `maplibre_native_ffi_runtime_set_resource_provider_async()` instead.
 *
 * See `mln_runtime_set_http_header_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when the library runs in a browser
 *   or on OpenHarmony.
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_set_http_header_transform_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiHttpHeaderTransform* transform,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_http_header_transform_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_http_header_transform_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_http_header_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Limits the size of the runtime's ambient cache.
 *
 * `size` is in bytes. Offline regions don't count toward it. It defaults to 50
 * MB, or to 0 in the browser when the runtime has no cache file.
 *
 * See `mln_runtime_set_maximum_ambient_cache_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
void maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_async(
  MaplibreNativeFfiRuntime* self, guint64 size, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_offline_region_download_state_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Starts or pauses the download of the offline region `region_id`.
 *
 * Setting the state that the download already has does nothing. A download
 * stops when the runtime closes.
 *
 * See `mln_runtime_set_offline_region_download_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the cache file has no region
 *   `region_id`.
 */
void maplibre_native_ffi_runtime_set_offline_region_download_state_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  MaplibreNativeFfiOfflineRegionDownloadState state,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_offline_region_download_state_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_offline_region_download_state_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_set_offline_region_download_state_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_offline_region_observed_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Selects the events that the runtime reports about the download of the offline
 * region `region_id`.
 *
 * `events` holds bits of
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS` and replaces the
 * region's previous selection. `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_NONE`
 * stops the region's events.
 *
 * See `mln_runtime_set_offline_region_observed` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the cache file has no region
 *   `region_id`.
 */
void maplibre_native_ffi_runtime_set_offline_region_observed_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  MaplibreNativeFfiRuntimeEventFlag events, GAsyncReadyCallback callback,
  gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_offline_region_observed_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_offline_region_observed_async` started.
 *
 * Returns: %TRUE when the call finished, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_runtime_set_offline_region_observed_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_resource_provider_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Serves the runtime's network requests through a callback.
 *
 * The provider receives the network requests that its routes match, and the
 * library loads the rest as usual. A provider can serve URL schemes that the
 * library can't load itself. The provider replaces the one set before.
 *
 * See `mln_runtime_set_resource_provider` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_set_resource_provider_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiResourceProvider* provider,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_resource_provider_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_resource_provider_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_resource_provider_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_set_resource_transform_async:
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Rewrites the URLs of the runtime's network requests.
 *
 * The transform replaces the one set before.
 *
 * See `mln_runtime_set_resource_transform` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Without `callback`, nothing reports whether the call succeeds.
 */
void maplibre_native_ffi_runtime_set_resource_transform_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiResourceTransform* transform,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_set_resource_transform_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_set_resource_transform_async` started.
 *
 * Returns: (transfer full): the command's completion, or %NULL with `error` set
 */
MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_resource_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_update_offline_region_metadata_async:
 * @metadata: the bytes
 * @callback: (scope async) (nullable): the callback to call when the call finishes
 * @user_data: the data to pass to `callback`
 *
 * Replaces the metadata of the offline region `region_id`.
 *
 * See `mln_runtime_update_offline_region_metadata` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when the cache file has no region
 *   `region_id`.
 */
void maplibre_native_ffi_runtime_update_offline_region_metadata_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id, GBytes* metadata,
  GAsyncReadyCallback callback, gpointer user_data
);

/**
 * maplibre_native_ffi_runtime_update_offline_region_metadata_finish:
 * @result: the result that the callback received
 *
 * Finishes a call that `maplibre_native_ffi_runtime_update_offline_region_metadata_async` started.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_runtime_update_offline_region_metadata_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
);

/**
 * maplibre_native_ffi_runtime_is_event_source:
 * @event: the event
 *
 * Whether a runtime event comes from this handle. A handle's ID needs all of
 * its 64 bits, so compare with this, rather than with
 * `maplibre_native_ffi_runtime_get_id`, where numbers are doubles, as in
 * JavaScript.
 *
 * Returns: whether the event's source is this handle
 */
gboolean maplibre_native_ffi_runtime_is_event_source(
  MaplibreNativeFfiRuntime* self, const MaplibreNativeFfiRuntimeEvent* event
);

/**
 * MaplibreNativeFfiStyleExpression:
 *
 * A parsed style expression.
 *
 * See `mln_style_expression` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION \
  (maplibre_native_ffi_style_expression_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiStyleExpression, maplibre_native_ffi_style_expression,
  MAPLIBRE_NATIVE_FFI, STYLE_EXPRESSION, GObject
)

/**
 * maplibre_native_ffi_style_expression_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_style_expression_get_id(
  MaplibreNativeFfiStyleExpression* self
);

/**
 * maplibre_native_ffi_style_expression_create: (constructor)
 * @expression: the UTF-8 text
 * @options: (nullable): the value, or %NULL
 *
 * Parses a style
 * \[expression\](https://maplibre.org/maplibre-style-spec/expressions/) to
 * evaluate without a map.
 *
 * See `mln_style_expression_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `expression` isn't a
 *   valid expression, or its result can't have the expected type.
 *
 * Returns: (transfer full): the object
 */
MaplibreNativeFfiStyleExpression* maplibre_native_ffi_style_expression_create(
  const char* expression, MaplibreNativeFfiStyleExpressionOptions* options,
  GError** error
);

/**
 * maplibre_native_ffi_style_expression_evaluate:
 * @inputs: (nullable): the value, or %NULL
 *
 * Evaluates a style expression and writes its result as JSON.
 *
 * A number in the result that isn't finite becomes null. A color is `["rgba",
 * r, g, b, a]`, with red, green, and blue from 0 to 255 and alpha from 0 to 1,
 * each exact only to single precision. Formatted text is a `["format", …]`
 * expression, an image is `{"name": …, "available": …}`, padding is `[top,
 * right, bottom, left]`, and a variable anchor offset is `[anchor, [x, y], …]`.
 *
 * See `mln_style_expression_evaluate` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when the expression fails to
 *   evaluate, such as when it reads an input that `inputs` doesn't set.
 *
 * Returns: (transfer full): the text
 */
char* maplibre_native_ffi_style_expression_evaluate(
  MaplibreNativeFfiStyleExpression* self,
  MaplibreNativeFfiStyleExpressionInputs* inputs, GError** error
);

/**
 * maplibre_native_ffi_style_expression_get_info:
 *
 * Reads what a style expression's result depends on.
 *
 * See `mln_style_expression_get_info` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiStyleExpressionInfo*
maplibre_native_ffi_style_expression_get_info(
  MaplibreNativeFfiStyleExpression* self, GError** error
);

/**
 * maplibre_native_ffi_style_expression_release:
 *
 * Releases a parsed style expression.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_style_expression_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
void maplibre_native_ffi_style_expression_release(
  MaplibreNativeFfiStyleExpression* self
);

/**
 * MaplibreNativeFfiVulkanDevice:
 *
 * A Vulkan instance, device, and queue that the library creates and owns.
 *
 * See `mln_vulkan_device` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_DEVICE \
  (maplibre_native_ffi_vulkan_device_get_type())
G_DECLARE_FINAL_TYPE(
  MaplibreNativeFfiVulkanDevice, maplibre_native_ffi_vulkan_device,
  MAPLIBRE_NATIVE_FFI, VULKAN_DEVICE, GObject
)

/**
 * maplibre_native_ffi_vulkan_device_get_id:
 *
 * The handle's ID, which runtime events name as their source.
 *
 * Returns: the ID
 */
guint64 maplibre_native_ffi_vulkan_device_get_id(
  MaplibreNativeFfiVulkanDevice* self
);

/**
 * maplibre_native_ffi_vulkan_device_create: (constructor)
 * @options: (nullable): the value, or %NULL
 *
 * Creates a Vulkan instance, device, and graphics queue for render sessions.
 *
 * Attach sessions with the context from
 * `maplibre_native_ffi_vulkan_device_get_context()`, in place of a device of
 * your own. Without `device_uuid` or `device_luid`, it prefers a discrete GPU.
 * The device enables the external memory and semaphore extensions that the GPU
 * offers.
 *
 * See `mln_vulkan_device_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED` when
 *   `maplibre_native_ffi_supported_render_backend_mask()` lacks
 *   `MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_VULKAN`, or when `presentation` is
 *   true and the Vulkan loader or the GPU can't present.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND` when no GPU matches `device_uuid` and
 *   `device_luid`.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when there is no Vulkan loader, no
 *   GPU has a graphics queue, or Vulkan can't create the instance or the
 *   device.
 *
 * Returns: (transfer full): the object
 */
MaplibreNativeFfiVulkanDevice* maplibre_native_ffi_vulkan_device_create(
  MaplibreNativeFfiVulkanDeviceOptions* options, GError** error
);

/**
 * maplibre_native_ffi_vulkan_device_get_context:
 *
 * Gets the context to attach render sessions to the device with.
 *
 * Pass it as the context of `maplibre_native_ffi_map_attach_vulkan_surface()`
 * or `maplibre_native_ffi_map_attach_vulkan_borrowed_texture()`. The sessions
 * on the device share its graphics queue. To submit to the queue yourself, set
 * `MaplibreNativeFfiRenderSessionAttachOptions:queue-lock` on each session.
 *
 * See `mln_vulkan_device_get_context` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_get_context(
  MaplibreNativeFfiVulkanDevice* self, GError** error
);

/**
 * maplibre_native_ffi_vulkan_device_release:
 *
 * Releases a Vulkan device.
 *
 * Render sessions that attached with the device's context keep using it until
 * they detach or their release finishes.
 *
 * The object calls this when it is disposed, unless it was released. Later
 * calls fail as on any released handle.
 *
 * See `mln_vulkan_device_release` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
void maplibre_native_ffi_vulkan_device_release(
  MaplibreNativeFfiVulkanDevice* self
);

#if defined(__APPLE__)
/**
 * maplibre_native_ffi_apple_frame_pacer_create:
 * @options: (nullable): the value, or %NULL
 *
 * Creates a frame pacer that follows the display through a `CADisplayLink`.
 *
 * You can call it on any thread. The pacer needs no run loop of yours. On
 * macOS, it follows the main display, the one with the menu bar.
 *
 * With `MaplibreNativeFfiFramePacerOptions:maximum-frames-per-second`, the
 * pacer also asks the system for that refresh rate. On iPhone, a rate above 60
 * needs `CADisableMinimumFrameDurationOnPhone` in the app's `Info.plist`.
 *
 * See `mln_apple_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR` when the system has no display to
 *   follow.
 *
 * Returns: (transfer full): the object
 */
MaplibreNativeFfiFramePacer* maplibre_native_ffi_apple_frame_pacer_create(
  MaplibreNativeFfiFramePacerOptions* options, GError** error
);
#endif

/**
 * maplibre_native_ffi_c_version:
 *
 * Returns the API epoch of the library.
 *
 * Headers and a library with different epochs aren't compatible.
 *
 * See `mln_c_version` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 *
 * Returns: the result
 */
guint32 maplibre_native_ffi_c_version(void);

/**
 * maplibre_native_ffi_camera_for_tile:
 *
 * Computes the camera that shows exactly one tile on a square map.
 *
 * Jump a map in `MAPLIBRE_NATIVE_FFI_MAP_MODE_TILE` to the camera to render
 * `tile`, where `logical_size` is the map's size. The camera centers the tile,
 * zooms until the tile fills the map, and looks straight down with north up and
 * no padding. The map keeps its camera within its zoom limits, so for a deep
 * tile, raise the maximum zoom past the camera's with
 * `maplibre_native_ffi_map_set_bounds_async()`.
 *
 * See `mln_camera_for_tile` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
 *
 * It fails with these errors:
 *
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `logical_size` isn't
 *   square, or is wider than 512 pixels. For denser tiles, raise the map's
 *   scale factor instead.
 * - `MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT` when `tile.x` or `tile.y`
 *   isn't less than 2 to the power `tile.z`, or when the tile can't fill the
 *   map at a zoom from 0 to 25.5, as for tile 0/0/0 on a 256-pixel map.
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_for_tile(
  const MaplibreNativeFfiCanonicalTileId* tile,
  const MaplibreNativeFfiLogicalSize* logical_size, GError** error
);

/**
 * maplibre_native_ffi_lat_lng_for_projected_meters:
 *
 * Converts meters in the Web Mercator projection to a coordinate.
 *
 * See `mln_lat_lng_for_projected_meters` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiLatLng* maplibre_native_ffi_lat_lng_for_projected_meters(
  const MaplibreNativeFfiProjectedMeters* meters, GError** error
);

/**
 * maplibre_native_ffi_clear_log_callback:
 *
 * Sends log messages to the platform's log again, instead of a callback.
 *
 * See `mln_log_clear_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_clear_log_callback(GError** error);

/**
 * maplibre_native_ffi_set_log_async_severity_mask:
 *
 * Selects the severities that the library logs on a background thread.
 *
 * The library logs other severities on the thread that logged them. `mask`
 * applies to every runtime in the process.
 *
 * See `mln_log_set_async_severity_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_set_log_async_severity_mask(
  MaplibreNativeFfiLogSeverityFlag mask, GError** error
);

/**
 * maplibre_native_ffi_set_log_callback:
 *
 * Sends the log messages of every runtime in the process to a callback.
 *
 * The callback replaces the one set before.
 *
 * Runs on the thread-default main context of the thread that started the call.
 *
 * See `mln_log_set_callback` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_set_log_callback(
  MaplibreNativeFfiLogHandler* handler, GError** error
);

/**
 * maplibre_native_ffi_logical_extent_physical_size:
 * @out_width: (out): the value
 * @out_height: (out): the value
 *
 * Computes the size in pixels of a target with a logical size and scale factor.
 *
 * See `mln_logical_extent_physical_size` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_logical_extent_physical_size(
  const MaplibreNativeFfiLogicalExtent* extent, guint32* out_width,
  guint32* out_height, GError** error
);

/**
 * maplibre_native_ffi_get_network_status:
 *
 * Reads whether the library may use the network.
 *
 * See `mln_network_get_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: the value
 */
MaplibreNativeFfiNetworkStatus maplibre_native_ffi_get_network_status(
  GError** error
);

/**
 * maplibre_native_ffi_set_network_status:
 *
 * Tells the library whether it may use the network.
 *
 * While the status is `MAPLIBRE_NATIVE_FFI_NETWORK_STATUS_OFFLINE`, the library
 * starts no network requests. The requests that wait start when the status
 * returns to `MAPLIBRE_NATIVE_FFI_NETWORK_STATUS_ONLINE`. The status starts as
 * online and applies to every runtime in the process.
 *
 * See `mln_network_set_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 *
 * Returns: %TRUE on success, or %FALSE with `error` set
 */
gboolean maplibre_native_ffi_set_network_status(
  MaplibreNativeFfiNetworkStatus status, GError** error
);

/**
 * maplibre_native_ffi_open_gl_supported_context_provider_mask:
 *
 * Reports the OpenGL context platforms that this build of the library has.
 *
 * See `mln_open_gl_supported_context_provider_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 *
 * Returns: the result
 */
MaplibreNativeFfiOpenGlContextProviderFlag
maplibre_native_ffi_open_gl_supported_context_provider_mask(void);

/**
 * maplibre_native_ffi_get_plugin_register_function_v1:
 *
 * Returns the function that registers a plugin's style layer types.
 *
 * Pass the function to the plugin's registration entry point before loading a
 * style that uses the plugin's layer types. Plugin callbacks run on library
 * threads and can keep running while the process exits, so they must not use
 * state that the process destroys at exit.
 *
 * See `mln_plugin_get_register_function_v1` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
 *
 * Returns: the result
 */
gpointer maplibre_native_ffi_get_plugin_register_function_v1(void);

/**
 * maplibre_native_ffi_projected_meters_for_lat_lng:
 *
 * Converts a coordinate to meters in the Web Mercator projection.
 *
 * See `mln_projected_meters_for_lat_lng` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
 *
 * Returns: (transfer full): the value
 */
MaplibreNativeFfiProjectedMeters*
maplibre_native_ffi_projected_meters_for_lat_lng(
  const MaplibreNativeFfiLatLng* coordinate, GError** error
);

/**
 * maplibre_native_ffi_supported_render_backend_mask:
 *
 * Returns the render backends that this build of the library draws with.
 *
 * See `mln_supported_render_backend_mask` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 *
 * Returns: the result
 */
MaplibreNativeFfiRenderBackendFlag
maplibre_native_ffi_supported_render_backend_mask(void);

G_END_DECLS

#endif
