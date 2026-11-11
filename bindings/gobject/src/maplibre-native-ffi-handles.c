// Generated from the C headers by tools/bindgen. Do not edit.
#include <maplibre_native_c.h>
#include <maplibre_native_c/plugin.h>
#include <string.h>

#include "maplibre-native-ffi-handles.h"

#include "maplibre-native-ffi-handles-private.h"
#include "maplibre-native-ffi-private.h"
#include "maplibre-native-ffi-values-private.h"

struct _MaplibreNativeFfiAcquiredFrame {
  GObject parent_instance;
  mln_acquired_frame handle;
  mln_acquired_frame released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiFramePacer {
  GObject parent_instance;
  mln_frame_pacer handle;
  mln_frame_pacer released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiMap {
  GObject parent_instance;
  mln_map handle;
  mln_map released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiMapProjection {
  GObject parent_instance;
  mln_map_projection handle;
  mln_map_projection released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiOffscreenDevice {
  GObject parent_instance;
  mln_offscreen_device handle;
  mln_offscreen_device released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiRenderSession {
  GObject parent_instance;
  mln_render_session handle;
  mln_render_session released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiResourceRequest {
  GObject parent_instance;
  mln_resource_request handle;
  mln_resource_request released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiResourceTransformRequest {
  GObject parent_instance;
  mln_resource_transform_request handle;
  mln_resource_transform_request released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiRuntime {
  GObject parent_instance;
  mln_runtime handle;
  mln_runtime released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiStyleExpression {
  GObject parent_instance;
  mln_style_expression handle;
  mln_style_expression released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

struct _MaplibreNativeFfiVulkanDevice {
  GObject parent_instance;
  mln_vulkan_device handle;
  mln_vulkan_device released_handle;
  // The object of the handle's parent, which outlives it.
  GObject* parent;
  gboolean borrowed;
  MaplibreNativeFfiReady* ready;
};

static gpointer mln_map_camera_for_geometry_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_camera_options_from_native(result->value);
}

static gpointer mln_map_camera_for_lat_lng_bounds_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_camera_options_from_native(result->value);
}

static gpointer mln_map_camera_for_lat_lngs_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_camera_options_from_native(result->value);
}

static gpointer mln_map_get_camera_decode(const mln_completion_result* result) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_camera_query_result_from_native(result->value);
}

static gpointer mln_map_get_feature_state_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_global_state_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_image_source_coordinates_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count, (GDestroyNotify)maplibre_native_ffi_lat_lng_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items, maplibre_native_ffi_lat_lng_from_native(
               (const mln_lat_lng*)(const void*)((const char*)result->value +
                                                 index * result->value_size)
             )
    );
  }
  return items;
}

static gpointer mln_map_get_loaded_style_json_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_style_image_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_style_image_info_from_native(result->value);
}

static gpointer mln_map_get_style_layer_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_style_layer_info_from_native(result->value);
}

static gpointer mln_map_get_style_layer_json_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_style_layer_property_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_style_light_property_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_style_root_property_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_get_style_source_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_style_source_info_from_native(result->value);
}

static gpointer mln_map_get_style_transition_options_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_style_transition_options_from_native(
    result->value
  );
}

static gpointer mln_map_get_style_url_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_map_lat_lng_bounds_for_camera_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_lat_lng_bounds_from_native(result->value);
}

static gpointer mln_map_list_style_layers_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count,
    (GDestroyNotify)maplibre_native_ffi_style_layer_info_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items,
      maplibre_native_ffi_style_layer_info_from_native(
        (const mln_style_layer_info*)(const void*)((const char*)result->value +
                                                   index * result->value_size)
      )
    );
  }
  return items;
}

static gpointer mln_map_list_style_sources_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count,
    (GDestroyNotify)maplibre_native_ffi_style_source_info_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items,
      maplibre_native_ffi_style_source_info_from_native(
        (const mln_style_source_info*)(const void*)((const char*)result->value +
                                                    index * result->value_size)
      )
    );
  }
  return items;
}

static gpointer mln_render_session_get_cluster_children_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_render_session_get_cluster_expansion_zoom_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  guint32* value = g_new0(guint32, 1);
  *value = (guint32)(*(const uint32_t*)result->value);
  return value;
}

static gpointer mln_render_session_get_cluster_leaves_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  const mln_buffer_view* view = result->value;
  return maplibre_native_ffi_text_new(*view);
}

static gpointer mln_render_session_query_rendered_features_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count,
    (GDestroyNotify)maplibre_native_ffi_rendered_feature_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items,
      maplibre_native_ffi_rendered_feature_from_native(
        (const mln_rendered_feature*)(const void*)((const char*)result->value +
                                                   index * result->value_size)
      )
    );
  }
  return items;
}

static gpointer mln_render_session_query_source_features_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count,
    (GDestroyNotify)maplibre_native_ffi_source_feature_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items,
      maplibre_native_ffi_source_feature_from_native(
        (const mln_source_feature*)(const void*)((const char*)result->value +
                                                 index * result->value_size)
      )
    );
  }
  return items;
}

static gpointer mln_render_session_read_texture_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_texture_readback_result_from_native(result->value);
}

static gpointer mln_runtime_create_offline_region_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_offline_region_info_from_native(result->value);
}

static gpointer mln_runtime_get_offline_region_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_offline_region_info_from_native(result->value);
}

static gpointer mln_runtime_get_offline_region_status_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_offline_region_status_from_native(result->value);
}

static gpointer mln_runtime_list_offline_regions_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count,
    (GDestroyNotify)maplibre_native_ffi_offline_region_info_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items,
      maplibre_native_ffi_offline_region_info_from_native(
        (const mln_offline_region_info*)(const void*)((const char*)
                                                        result->value +
                                                      index *
                                                        result->value_size)
      )
    );
  }
  return items;
}

static gpointer mln_runtime_merge_offline_regions_decode(
  const mln_completion_result* result
) {
  GPtrArray* items = g_ptr_array_new_full(
    (guint)result->value_count,
    (GDestroyNotify)maplibre_native_ffi_offline_region_info_free
  );
  for (size_t index = 0; index < result->value_count; index++) {
    g_ptr_array_add(
      items,
      maplibre_native_ffi_offline_region_info_from_native(
        (const mln_offline_region_info*)(const void*)((const char*)
                                                        result->value +
                                                      index *
                                                        result->value_size)
      )
    );
  }
  return items;
}

static gpointer mln_runtime_update_offline_region_metadata_decode(
  const mln_completion_result* result
) {
  if (result->value == NULL) return NULL;
  return maplibre_native_ffi_offline_region_info_from_native(result->value);
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiAcquiredFrame, maplibre_native_ffi_acquired_frame,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_acquired_frame_object_dispose(GObject* object) {
  MaplibreNativeFfiAcquiredFrame* self =
    MAPLIBRE_NATIVE_FFI_ACQUIRED_FRAME(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_acquired_frame_discard(self->handle, NULL);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_acquired_frame_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_acquired_frame_class_init(
  MaplibreNativeFfiAcquiredFrameClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_acquired_frame_object_dispose;
}

static void maplibre_native_ffi_acquired_frame_init(
  MaplibreNativeFfiAcquiredFrame* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiAcquiredFrame* maplibre_native_ffi_acquired_frame_wrap(
  mln_acquired_frame handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiAcquiredFrame* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_ACQUIRED_FRAME, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_acquired_frame_get_id(
  MaplibreNativeFfiAcquiredFrame* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

gboolean maplibre_native_ffi_acquired_frame_discard(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_acquired_frame_discard(self->handle, &diagnostic_);
  if (status_ == MLN_STATUS_OK) self->handle = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

#if defined(__linux__)
MaplibreNativeFfiDmaBufFrame* maplibre_native_ffi_acquired_frame_get_dma_buf(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_dma_buf_frame out_frame_;
  memset(&out_frame_, 0, sizeof out_frame_);
  (&out_frame_)->size = sizeof *(&out_frame_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  void* scope_ = NULL;
  MaplibreNativeFfiDmaBufFrame* value_ = NULL;
  mln_status status_ =
    mln_acquired_frame_view_begin(self->handle, &scope_, &diagnostic_);
  if (status_ == MLN_STATUS_OK) {
    status_ =
      mln_acquired_frame_get_dma_buf(self->handle, &out_frame_, &diagnostic_);
    if (status_ == MLN_STATUS_OK) {
      value_ = maplibre_native_ffi_dma_buf_frame_from_native(&out_frame_);
    }
    mln_acquired_frame_view_end(scope_);
  }
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return value_;
}
#endif

#if defined(__APPLE__)
MaplibreNativeFfiIoSurfaceFrame*
maplibre_native_ffi_acquired_frame_get_io_surface(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_io_surface_frame out_frame_;
  memset(&out_frame_, 0, sizeof out_frame_);
  (&out_frame_)->size = sizeof *(&out_frame_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  void* scope_ = NULL;
  MaplibreNativeFfiIoSurfaceFrame* value_ = NULL;
  mln_status status_ =
    mln_acquired_frame_view_begin(self->handle, &scope_, &diagnostic_);
  if (status_ == MLN_STATUS_OK) {
    status_ = mln_acquired_frame_get_io_surface(
      self->handle, &out_frame_, &diagnostic_
    );
    if (status_ == MLN_STATUS_OK) {
      value_ = maplibre_native_ffi_io_surface_frame_from_native(&out_frame_);
    }
    mln_acquired_frame_view_end(scope_);
  }
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return value_;
}
#endif

#if defined(__APPLE__)
MaplibreNativeFfiMetalTextureFrame*
maplibre_native_ffi_acquired_frame_get_metal_texture(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_metal_texture_frame out_frame_;
  memset(&out_frame_, 0, sizeof out_frame_);
  (&out_frame_)->size = sizeof *(&out_frame_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  void* scope_ = NULL;
  MaplibreNativeFfiMetalTextureFrame* value_ = NULL;
  mln_status status_ =
    mln_acquired_frame_view_begin(self->handle, &scope_, &diagnostic_);
  if (status_ == MLN_STATUS_OK) {
    status_ = mln_acquired_frame_get_metal_texture(
      self->handle, &out_frame_, &diagnostic_
    );
    if (status_ == MLN_STATUS_OK) {
      value_ = maplibre_native_ffi_metal_texture_frame_from_native(&out_frame_);
    }
    mln_acquired_frame_view_end(scope_);
  }
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return value_;
}
#endif

MaplibreNativeFfiOpenGlTextureFrame*
maplibre_native_ffi_acquired_frame_get_open_gl_texture(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_open_gl_texture_frame out_frame_;
  memset(&out_frame_, 0, sizeof out_frame_);
  (&out_frame_)->size = sizeof *(&out_frame_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  void* scope_ = NULL;
  MaplibreNativeFfiOpenGlTextureFrame* value_ = NULL;
  mln_status status_ =
    mln_acquired_frame_view_begin(self->handle, &scope_, &diagnostic_);
  if (status_ == MLN_STATUS_OK) {
    status_ = mln_acquired_frame_get_open_gl_texture(
      self->handle, &out_frame_, &diagnostic_
    );
    if (status_ == MLN_STATUS_OK) {
      value_ =
        maplibre_native_ffi_open_gl_texture_frame_from_native(&out_frame_);
    }
    mln_acquired_frame_view_end(scope_);
  }
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return value_;
}

MaplibreNativeFfiGpuSync* maplibre_native_ffi_acquired_frame_get_producer_sync(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_gpu_sync out_sync_;
  memset(&out_sync_, 0, sizeof out_sync_);
  (&out_sync_)->size = sizeof *(&out_sync_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  void* scope_ = NULL;
  MaplibreNativeFfiGpuSync* value_ = NULL;
  mln_status status_ =
    mln_acquired_frame_view_begin(self->handle, &scope_, &diagnostic_);
  if (status_ == MLN_STATUS_OK) {
    status_ = mln_acquired_frame_get_producer_sync(
      self->handle, &out_sync_, &diagnostic_
    );
    if (status_ == MLN_STATUS_OK) {
      value_ = maplibre_native_ffi_gpu_sync_from_native(&out_sync_);
    }
    mln_acquired_frame_view_end(scope_);
  }
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return value_;
}

MaplibreNativeFfiRenderFrameResult*
maplibre_native_ffi_acquired_frame_get_result(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_render_frame_result out_result_;
  memset(&out_result_, 0, sizeof out_result_);
  (&out_result_)->size = sizeof *(&out_result_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_acquired_frame_get_result(self->handle, &out_result_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_render_frame_result_from_native(&out_result_);
}

MaplibreNativeFfiVulkanTextureFrame*
maplibre_native_ffi_acquired_frame_get_vulkan_texture(
  MaplibreNativeFfiAcquiredFrame* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_vulkan_texture_frame out_frame_;
  memset(&out_frame_, 0, sizeof out_frame_);
  (&out_frame_)->size = sizeof *(&out_frame_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  void* scope_ = NULL;
  MaplibreNativeFfiVulkanTextureFrame* value_ = NULL;
  mln_status status_ =
    mln_acquired_frame_view_begin(self->handle, &scope_, &diagnostic_);
  if (status_ == MLN_STATUS_OK) {
    status_ = mln_acquired_frame_get_vulkan_texture(
      self->handle, &out_frame_, &diagnostic_
    );
    if (status_ == MLN_STATUS_OK) {
      value_ =
        maplibre_native_ffi_vulkan_texture_frame_from_native(&out_frame_);
    }
    mln_acquired_frame_view_end(scope_);
  }
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return value_;
}

gboolean maplibre_native_ffi_acquired_frame_release(
  MaplibreNativeFfiAcquiredFrame* self,
  const MaplibreNativeFfiGpuSync* consumer_completion, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_ACQUIRED_FRAME(self), FALSE);
  g_return_val_if_fail(consumer_completion != NULL, FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_gpu_sync consumer_completion_native_;
  maplibre_native_ffi_gpu_sync_to_native(
    consumer_completion, &consumer_completion_native_, NULL
  );
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_acquired_frame_release(
    self->handle, &consumer_completion_native_, &diagnostic_
  );
  if (status_ == MLN_STATUS_OK) self->handle = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiFramePacer, maplibre_native_ffi_frame_pacer, G_TYPE_OBJECT
)

static void maplibre_native_ffi_frame_pacer_object_dispose(GObject* object) {
  MaplibreNativeFfiFramePacer* self = MAPLIBRE_NATIVE_FFI_FRAME_PACER(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_frame_pacer_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_frame_pacer_parent_class)->dispose(object);
}

static void maplibre_native_ffi_frame_pacer_class_init(
  MaplibreNativeFfiFramePacerClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_frame_pacer_object_dispose;
}

static void maplibre_native_ffi_frame_pacer_init(
  MaplibreNativeFfiFramePacer* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiFramePacer* maplibre_native_ffi_frame_pacer_wrap(
  mln_frame_pacer handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiFramePacer* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_FRAME_PACER, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_frame_pacer_get_id(
  MaplibreNativeFfiFramePacer* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_FRAME_PACER(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

void maplibre_native_ffi_frame_pacer_release(
  MaplibreNativeFfiFramePacer* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FRAME_PACER(self));
  mln_frame_pacer_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMap, maplibre_native_ffi_map, G_TYPE_OBJECT
)

static void maplibre_native_ffi_map_object_dispose(GObject* object) {
  MaplibreNativeFfiMap* self = MAPLIBRE_NATIVE_FFI_MAP(object);
  if (!self->borrowed && self->handle != MLN_HANDLE_NULL) {
    maplibre_native_ffi_map_dispose(self);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_map_parent_class)->dispose(object);
}

static void maplibre_native_ffi_map_class_init(
  MaplibreNativeFfiMapClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose = maplibre_native_ffi_map_object_dispose;
}

static void maplibre_native_ffi_map_init(MaplibreNativeFfiMap* self) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiMap* maplibre_native_ffi_map_wrap(
  mln_map handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiMap* self = g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MAP, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_map_get_id(MaplibreNativeFfiMap* self) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

void maplibre_native_ffi_map_add_custom_geometry_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiCustomGeometrySourceOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_GEOMETRY_SOURCE_OPTIONS(options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_add_custom_geometry_source_async,
    callback, user_data, TRUE
  );
  mln_custom_geometry_source_options options_native_;
  maplibre_native_ffi_custom_geometry_source_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_add_custom_geometry_source(
    self->handle, maplibre_native_ffi_text_view(source_id), &options_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_custom_geometry_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_add_custom_vector_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiCustomVectorSourceOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_CUSTOM_VECTOR_SOURCE_OPTIONS(options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_add_custom_vector_source_async,
    callback, user_data, TRUE
  );
  mln_custom_vector_source_options options_native_;
  maplibre_native_ffi_custom_vector_source_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_add_custom_vector_source(
    self->handle, maplibre_native_ffi_text_view(source_id), &options_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_custom_vector_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_add_geo_json_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiGeoJsonSourceOptions* options, const char* attribution,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_GEO_JSON_SOURCE_OPTIONS(options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_add_geo_json_source_async, callback,
    user_data, TRUE
  );
  mln_geo_json_source_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_geo_json_source_options_to_native(
      options, &options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_add_geo_json_source(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (options != NULL ? &options_native_ : NULL),
    maplibre_native_ffi_text_view(attribution), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_geo_json_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_add_image_source_image_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLng* coordinates, gsize coordinate_count,
  MaplibreNativeFfiPremultipliedRgba8Image* image, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(image));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_add_image_source_image_async,
    callback, user_data, TRUE
  );
  mln_premultiplied_rgba8_image image_native_;
  maplibre_native_ffi_premultiplied_rgba8_image_to_native(
    image, &image_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_add_image_source_image(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (const mln_lat_lng*)coordinates, (size_t)(coordinate_count), &image_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_image_source_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_add_style_layer_json_async(
  MaplibreNativeFfiMap* self, const char* layer_json,
  const char* before_layer_id, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_add_style_layer_json_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_add_style_layer_json(
    self->handle, maplibre_native_ffi_text_view(layer_json),
    maplibre_native_ffi_text_view(before_layer_id), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_style_layer_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_add_style_source_json_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* source_json,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_add_style_source_json_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_add_style_source_json(
    self->handle, maplibre_native_ffi_text_view(source_id),
    maplibre_native_ffi_text_view(source_json), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_add_style_source_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_apply_camera_delta_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiCameraDelta* delta,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_DELTA(delta));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_apply_camera_delta_async, callback,
    user_data, TRUE
  );
  mln_camera_delta delta_native_;
  maplibre_native_ffi_camera_delta_to_native(
    delta, &delta_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_apply_camera_delta(
    self->handle, &delta_native_, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_apply_camera_delta_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

#if defined(__linux__)
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_dma_buf_ring(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiDmaBufRingDescriptor* descriptor,
  MaplibreNativeFfiVulkanDevice* device,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_DMA_BUF_RING_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_dma_buf_ring_descriptor descriptor_native_;
  maplibre_native_ffi_dma_buf_ring_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_dma_buf_ring(
    self->handle, &descriptor_native_,
    (device != NULL ? device->handle : MLN_HANDLE_NULL), &options_native_,
    &out_session_, &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}
#endif

#if defined(__APPLE__)
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_io_surface_ring(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiIoSurfaceRingDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_IO_SURFACE_RING_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_io_surface_ring_descriptor descriptor_native_;
  maplibre_native_ffi_io_surface_ring_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_io_surface_ring(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}
#endif

#if defined(__APPLE__)
MaplibreNativeFfiRenderSession*
maplibre_native_ffi_map_attach_metal_borrowed_texture(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiMetalBorrowedTextureDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_metal_borrowed_texture_descriptor descriptor_native_;
  maplibre_native_ffi_metal_borrowed_texture_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_metal_borrowed_texture(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}
#endif

#if defined(__APPLE__)
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_metal_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiMetalSurfaceDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_metal_surface_descriptor descriptor_native_;
  maplibre_native_ffi_metal_surface_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_metal_surface(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}
#endif

MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_offscreen(
  MaplibreNativeFfiMap* self,
  const MaplibreNativeFfiOffscreenDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options,
  MaplibreNativeFfiOffscreenDevice* device, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(descriptor != NULL, NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_offscreen_descriptor descriptor_native_;
  maplibre_native_ffi_offscreen_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_offscreen(
    self->handle, &descriptor_native_, &options_native_,
    (device != NULL ? device->handle : MLN_HANDLE_NULL), &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}

MaplibreNativeFfiRenderSession*
maplibre_native_ffi_map_attach_open_gl_borrowed_texture(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiOpenGlBorrowedTextureDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_open_gl_borrowed_texture_descriptor descriptor_native_;
  maplibre_native_ffi_open_gl_borrowed_texture_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_open_gl_borrowed_texture(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}

MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_open_gl_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiOpenGlSurfaceDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_open_gl_surface_descriptor descriptor_native_;
  maplibre_native_ffi_open_gl_surface_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_open_gl_surface(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}

MaplibreNativeFfiRenderSession*
maplibre_native_ffi_map_attach_vulkan_borrowed_texture(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiVulkanBorrowedTextureDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_vulkan_borrowed_texture_descriptor descriptor_native_;
  maplibre_native_ffi_vulkan_borrowed_texture_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_vulkan_borrowed_texture(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}

MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_vulkan_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiVulkanSurfaceDescriptor* descriptor,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_SURFACE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_vulkan_surface_descriptor descriptor_native_;
  maplibre_native_ffi_vulkan_surface_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_vulkan_surface(
    self->handle, &descriptor_native_, &options_native_, &out_session_,
    &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}

#if defined(__linux__)
MaplibreNativeFfiRenderSession* maplibre_native_ffi_map_attach_wayland_surface(
  MaplibreNativeFfiMap* self,
  MaplibreNativeFfiWaylandSurfaceDescriptor* descriptor,
  MaplibreNativeFfiVulkanDevice* device,
  MaplibreNativeFfiRenderSessionAttachOptions* options, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_DESCRIPTOR(descriptor), NULL
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION_ATTACH_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_wayland_surface_descriptor descriptor_native_;
  maplibre_native_ffi_wayland_surface_descriptor_to_native(
    descriptor, &descriptor_native_, &call_
  );
  mln_render_session_attach_options options_native_;
  maplibre_native_ffi_render_session_attach_options_to_native(
    options, &options_native_, &call_
  );
  mln_render_session out_session_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_attach_wayland_surface(
    self->handle, &descriptor_native_,
    (device != NULL ? device->handle : MLN_HANDLE_NULL), &options_native_,
    &out_session_, &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiRenderSession* object_ =
    maplibre_native_ffi_render_session_wrap(out_session_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}
#endif

void maplibre_native_ffi_map_begin_command_group_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_begin_command_group_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_begin_command_group(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_begin_command_group_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_camera_for_geometry_async(
  MaplibreNativeFfiMap* self, const char* geometry,
  MaplibreNativeFfiCameraFitOptions* fit_options, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(
    fit_options == NULL ||
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(fit_options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_camera_for_geometry_async, callback,
    user_data, FALSE
  );
  mln_camera_fit_options fit_options_native_;
  if (fit_options != NULL)
    maplibre_native_ffi_camera_fit_options_to_native(
      fit_options, &fit_options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_camera_for_geometry_decode,
        (GDestroyNotify)g_object_unref
      ))
    return;
  mln_status status_ = mln_map_camera_for_geometry(
    self->handle, maplibre_native_ffi_text_view(geometry),
    (fit_options != NULL ? &fit_options_native_ : NULL),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_map_camera_for_geometry_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_camera_for_lat_lng_bounds_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLngBounds* bounds,
  MaplibreNativeFfiCameraFitOptions* fit_options, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(bounds != NULL);
  g_return_if_fail(
    fit_options == NULL ||
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(fit_options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_camera_for_lat_lng_bounds_async,
    callback, user_data, FALSE
  );
  mln_lat_lng_bounds bounds_native_;
  maplibre_native_ffi_lat_lng_bounds_to_native(
    bounds, &bounds_native_, &async_.call
  );
  mln_camera_fit_options fit_options_native_;
  if (fit_options != NULL)
    maplibre_native_ffi_camera_fit_options_to_native(
      fit_options, &fit_options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_camera_for_lat_lng_bounds_decode,
        (GDestroyNotify)g_object_unref
      ))
    return;
  mln_status status_ = mln_map_camera_for_lat_lng_bounds(
    self->handle, bounds_native_,
    (fit_options != NULL ? &fit_options_native_ : NULL),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_map_camera_for_lat_lng_bounds_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_camera_for_lat_lngs_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLng* coordinates,
  gsize coordinate_count, MaplibreNativeFfiCameraFitOptions* fit_options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(
    fit_options == NULL ||
    MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_OPTIONS(fit_options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_camera_for_lat_lngs_async, callback,
    user_data, FALSE
  );
  mln_camera_fit_options fit_options_native_;
  if (fit_options != NULL)
    maplibre_native_ffi_camera_fit_options_to_native(
      fit_options, &fit_options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_camera_for_lat_lngs_decode,
        (GDestroyNotify)g_object_unref
      ))
    return;
  mln_status status_ = mln_map_camera_for_lat_lngs(
    self->handle, (const mln_lat_lng*)coordinates, (size_t)(coordinate_count),
    (fit_options != NULL ? &fit_options_native_ : NULL),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCameraOptions*
maplibre_native_ffi_map_camera_for_lat_lngs_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_cancel_camera_transitions_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_cancel_camera_transitions_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_cancel_camera_transitions(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_cancel_camera_transitions_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_cancel_camera_transitions_with_id_async(
  MaplibreNativeFfiMap* self, guint64 transition_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_cancel_camera_transitions_with_id_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_cancel_camera_transitions_with_id(
    self->handle, (uint64_t)(transition_id), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_cancel_camera_transitions_with_id_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_dispose(MaplibreNativeFfiMap* self) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  if (self->borrowed) return;
  mln_map handle = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (handle != MLN_HANDLE_NULL) {
    (void)mln_map_dispose(handle, NULL);
    self->released_handle = handle;
  }
}

void maplibre_native_ffi_map_dump_debug_logs_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_dump_debug_logs_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_dump_debug_logs(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_dump_debug_logs_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_end_command_group_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_end_command_group_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_end_command_group(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_end_command_group_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_fit_camera_to_geometry_async(
  MaplibreNativeFfiMap* self, const char* geometry,
  MaplibreNativeFfiCameraFitUpdate* update, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(update));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_fit_camera_to_geometry_async,
    callback, user_data, TRUE
  );
  mln_camera_fit_update update_native_;
  maplibre_native_ffi_camera_fit_update_to_native(
    update, &update_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_fit_camera_to_geometry(
    self->handle, maplibre_native_ffi_text_view(geometry), &update_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_fit_camera_to_geometry_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLngBounds* bounds,
  MaplibreNativeFfiCameraFitUpdate* update, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(bounds != NULL);
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(update));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_async,
    callback, user_data, TRUE
  );
  mln_lat_lng_bounds bounds_native_;
  maplibre_native_ffi_lat_lng_bounds_to_native(
    bounds, &bounds_native_, &async_.call
  );
  mln_camera_fit_update update_native_;
  maplibre_native_ffi_camera_fit_update_to_native(
    update, &update_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_fit_camera_to_lat_lng_bounds(
    self->handle, bounds_native_, &update_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_fit_camera_to_lat_lng_bounds_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_fit_camera_to_lat_lngs_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLatLng* coordinates,
  gsize coordinate_count, MaplibreNativeFfiCameraFitUpdate* update,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_FIT_UPDATE(update));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_fit_camera_to_lat_lngs_async,
    callback, user_data, TRUE
  );
  mln_camera_fit_update update_native_;
  maplibre_native_ffi_camera_fit_update_to_native(
    update, &update_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_fit_camera_to_lat_lngs(
    self->handle, (const mln_lat_lng*)coordinates, (size_t)(coordinate_count),
    &update_native_, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_fit_camera_to_lat_lngs_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_camera_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_camera_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_camera_decode,
        (GDestroyNotify)maplibre_native_ffi_camera_query_result_free
      ))
    return;
  mln_status status_ = mln_map_get_camera(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCameraQueryResult* maplibre_native_ffi_map_get_camera_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_feature_state_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFeatureStateSelector* selector,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(selector));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_feature_state_async, callback,
    user_data, FALSE
  );
  mln_feature_state_selector selector_native_;
  maplibre_native_ffi_feature_state_selector_to_native(
    selector, &selector_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_feature_state_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_feature_state(
    self->handle, &selector_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_feature_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_global_state_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_global_state_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_global_state_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_global_state(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_global_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_image_source_coordinates_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_image_source_coordinates_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_image_source_coordinates_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_map_get_image_source_coordinates(
    self->handle, maplibre_native_ffi_text_view(source_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_map_get_image_source_coordinates_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_loaded_style_json_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_loaded_style_json_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_loaded_style_json_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_loaded_style_json(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_loaded_style_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

MaplibreNativeFfiMapSnapshot* maplibre_native_ffi_map_get_snapshot(
  MaplibreNativeFfiMap* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_map_snapshot out_snapshot_;
  memset(&out_snapshot_, 0, sizeof out_snapshot_);
  (&out_snapshot_)->size = sizeof *(&out_snapshot_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_map_get_snapshot(self->handle, &out_snapshot_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_map_snapshot_from_native(&out_snapshot_);
}

void maplibre_native_ffi_map_get_style_image_async(
  MaplibreNativeFfiMap* self, const char* image_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_image_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_image_decode,
        (GDestroyNotify)maplibre_native_ffi_style_image_info_free
      ))
    return;
  mln_status status_ = mln_map_get_style_image(
    self->handle, maplibre_native_ffi_text_view(image_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiStyleImageInfo* maplibre_native_ffi_map_get_style_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_layer_async(
  MaplibreNativeFfiMap* self, const char* layer_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_layer_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_layer_decode,
        (GDestroyNotify)maplibre_native_ffi_style_layer_info_free
      ))
    return;
  mln_status status_ = mln_map_get_style_layer(
    self->handle, maplibre_native_ffi_text_view(layer_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiStyleLayerInfo* maplibre_native_ffi_map_get_style_layer_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_layer_json_async(
  MaplibreNativeFfiMap* self, const char* layer_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_layer_json_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_layer_json_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_style_layer_json(
    self->handle, maplibre_native_ffi_text_view(layer_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_style_layer_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_layer_property_async(
  MaplibreNativeFfiMap* self, const char* layer_id, const char* property_name,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_layer_property_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_layer_property_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_style_layer_property(
    self->handle, maplibre_native_ffi_text_view(layer_id),
    maplibre_native_ffi_text_view(property_name), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_style_layer_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_light_property_async(
  MaplibreNativeFfiMap* self, const char* property_name,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_light_property_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_light_property_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_style_light_property(
    self->handle, maplibre_native_ffi_text_view(property_name),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_style_light_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_root_property_async(
  MaplibreNativeFfiMap* self, const char* property_name,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_root_property_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_root_property_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_style_root_property(
    self->handle, maplibre_native_ffi_text_view(property_name),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_style_root_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_source_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_source_decode,
        (GDestroyNotify)maplibre_native_ffi_style_source_info_free
      ))
    return;
  mln_status status_ = mln_map_get_style_source(
    self->handle, maplibre_native_ffi_text_view(source_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiStyleSourceInfo*
maplibre_native_ffi_map_get_style_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_transition_options_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_transition_options_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_transition_options_decode,
        (GDestroyNotify)g_object_unref
      ))
    return;
  mln_status status_ = mln_map_get_style_transition_options(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiStyleTransitionOptions*
maplibre_native_ffi_map_get_style_transition_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_get_style_url_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_get_style_url_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_get_style_url_decode, g_free
      ))
    return;
  mln_status status_ = mln_map_get_style_url(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_map_get_style_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_invalidate_custom_geometry_source_region_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLngBounds* bounds, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(bounds != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_invalidate_custom_geometry_source_region_async,
    callback, user_data, TRUE
  );
  mln_lat_lng_bounds bounds_native_;
  maplibre_native_ffi_lat_lng_bounds_to_native(
    bounds, &bounds_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_invalidate_custom_geometry_source_region(
    self->handle, maplibre_native_ffi_text_view(source_id), bounds_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_geometry_source_region_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(tile_id != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_async,
    callback, user_data, TRUE
  );
  mln_canonical_tile_id tile_id_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(
    tile_id, &tile_id_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_invalidate_custom_geometry_source_tile(
    self->handle, maplibre_native_ffi_text_view(source_id), tile_id_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_geometry_source_tile_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_invalidate_custom_vector_source_region_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLngBounds* bounds, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(bounds != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_invalidate_custom_vector_source_region_async,
    callback, user_data, TRUE
  );
  mln_lat_lng_bounds bounds_native_;
  maplibre_native_ffi_lat_lng_bounds_to_native(
    bounds, &bounds_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_invalidate_custom_vector_source_region(
    self->handle, maplibre_native_ffi_text_view(source_id), bounds_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_vector_source_region_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_invalidate_custom_vector_source_tile_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(tile_id != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_invalidate_custom_vector_source_tile_async,
    callback, user_data, TRUE
  );
  mln_canonical_tile_id tile_id_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(
    tile_id, &tile_id_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_invalidate_custom_vector_source_tile(
    self->handle, maplibre_native_ffi_text_view(source_id), tile_id_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_invalidate_custom_vector_source_tile_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_lat_lng_bounds_for_camera_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiCameraOptions* camera,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(camera));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_lat_lng_bounds_for_camera_async,
    callback, user_data, FALSE
  );
  mln_camera_options camera_native_;
  maplibre_native_ffi_camera_options_to_native(
    camera, &camera_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_lat_lng_bounds_for_camera_decode,
        (GDestroyNotify)maplibre_native_ffi_lat_lng_bounds_free
      ))
    return;
  mln_status status_ = mln_map_lat_lng_bounds_for_camera(
    self->handle, &camera_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiLatLngBounds*
maplibre_native_ffi_map_lat_lng_bounds_for_camera_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_list_style_layers_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_list_style_layers_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_list_style_layers_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_map_list_style_layers(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_map_list_style_layers_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_list_style_sources_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_list_style_sources_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_map_list_style_sources_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_map_list_style_sources(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_map_list_style_sources_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_move_style_layer_async(
  MaplibreNativeFfiMap* self, const char* layer_id, const char* before_layer_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_move_style_layer_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_move_style_layer(
    self->handle, maplibre_native_ffi_text_view(layer_id),
    maplibre_native_ffi_text_view(before_layer_id), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_move_style_layer_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_release_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  if (self->borrowed) {
    GError* borrowed_ = NULL;
    maplibre_native_ffi_set_borrowed_error(&borrowed_);
    g_task_report_error(
      self, callback, user_data, maplibre_native_ffi_map_release_async,
      borrowed_
    );
    return;
  }
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_release_async, callback, user_data,
    FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_map_release(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  if (status_ == MLN_STATUS_OK) {
    self->released_handle = self->handle;
    self->handle = MLN_HANDLE_NULL;
  }
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_map_release_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_map_remove_feature_state_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFeatureStateSelector* selector,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(selector));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_remove_feature_state_async, callback,
    user_data, TRUE
  );
  mln_feature_state_selector selector_native_;
  maplibre_native_ffi_feature_state_selector_to_native(
    selector, &selector_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_remove_feature_state(
    self->handle, &selector_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_feature_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_remove_style_image_async(
  MaplibreNativeFfiMap* self, const char* image_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_remove_style_image_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_remove_style_image(
    self->handle, maplibre_native_ffi_text_view(image_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_style_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_remove_style_layer_async(
  MaplibreNativeFfiMap* self, const char* layer_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_remove_style_layer_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_remove_style_layer(
    self->handle, maplibre_native_ffi_text_view(layer_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_style_layer_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_remove_style_source_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_remove_style_source_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_remove_style_source(
    self->handle, maplibre_native_ffi_text_view(source_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_remove_style_source_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_request_repaint_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_request_repaint_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_request_repaint(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_request_repaint_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_request_still_image_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_request_still_image_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_map_request_still_image(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_map_request_still_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_map_resize_async(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiLogicalSize* logical_size,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(logical_size != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_resize_async, callback, user_data,
    TRUE
  );
  mln_logical_size logical_size_native_;
  maplibre_native_ffi_logical_size_to_native(
    logical_size, &logical_size_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_resize(
    self->handle, logical_size_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion* maplibre_native_ffi_map_resize_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_bounds_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiBoundOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_BOUND_OPTIONS(options));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_bounds_async, callback,
    user_data, TRUE
  );
  mln_bound_options options_native_;
  maplibre_native_ffi_bound_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_bounds(
    self->handle, &options_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion* maplibre_native_ffi_map_set_bounds_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, const char* data,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(tile_id != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async,
    callback, user_data, TRUE
  );
  mln_canonical_tile_id tile_id_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(
    tile_id, &tile_id_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_custom_geometry_source_tile_data(
    self->handle, maplibre_native_ffi_text_view(source_id), tile_id_native_,
    maplibre_native_ffi_text_view(data), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_geometry_source_tile_data_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, const char* message,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(tile_id != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_set_custom_geometry_source_tile_error_async,
    callback, user_data, TRUE
  );
  mln_canonical_tile_id tile_id_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(
    tile_id, &tile_id_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_custom_geometry_source_tile_error(
    self->handle, maplibre_native_ffi_text_view(source_id), tile_id_native_,
    maplibre_native_ffi_text_view(message), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_geometry_source_tile_error_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_custom_vector_source_tile_data_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id,
  MaplibreNativeFfiStyleVectorTileEncoding encoding, GBytes* data,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(tile_id != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_set_custom_vector_source_tile_data_async, callback,
    user_data, TRUE
  );
  mln_canonical_tile_id tile_id_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(
    tile_id, &tile_id_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_custom_vector_source_tile_data(
    self->handle, maplibre_native_ffi_text_view(source_id), tile_id_native_,
    (mln_style_vector_tile_encoding)(encoding),
    maplibre_native_ffi_bytes_view(data), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_vector_source_tile_data_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_custom_vector_source_tile_error_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiCanonicalTileId* tile_id, const char* message,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(tile_id != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_set_custom_vector_source_tile_error_async, callback,
    user_data, TRUE
  );
  mln_canonical_tile_id tile_id_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(
    tile_id, &tile_id_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_custom_vector_source_tile_error(
    self->handle, maplibre_native_ffi_text_view(source_id), tile_id_native_,
    maplibre_native_ffi_text_view(message), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_custom_vector_source_tile_error_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_debug_flags_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiMapDebugFlag flags,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_debug_flags_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_debug_flags(
    self->handle, (mln_map_debug_flags)(flags), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_debug_flags_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_event_mask_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiRuntimeEventFlag mask,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_event_mask_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_event_mask(
    self->handle, (mln_runtime_event_flags)(mask), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_event_mask_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_feature_state_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFeatureStateSelector* selector,
  const char* state, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FEATURE_STATE_SELECTOR(selector));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_feature_state_async, callback,
    user_data, TRUE
  );
  mln_feature_state_selector selector_native_;
  maplibre_native_ffi_feature_state_selector_to_native(
    selector, &selector_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_feature_state(
    self->handle, &selector_native_, maplibre_native_ffi_text_view(state),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_feature_state_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_free_camera_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiFreeCameraOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_FREE_CAMERA_OPTIONS(options));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_free_camera_options_async,
    callback, user_data, TRUE
  );
  mln_free_camera_options options_native_;
  maplibre_native_ffi_free_camera_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_free_camera_options(
    self->handle, &options_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_free_camera_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_geo_json_source_data_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* data,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_geo_json_source_data_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_geo_json_source_data(
    self->handle, maplibre_native_ffi_text_view(source_id),
    maplibre_native_ffi_text_view(data), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_geo_json_source_data_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_async(
  MaplibreNativeFfiMap* self, const char* source_id, gboolean enabled,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_geo_json_source_synchronous_tiling(
    self->handle, maplibre_native_ffi_text_view(source_id), (enabled) != FALSE,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_geo_json_source_synchronous_tiling_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_geo_json_source_url_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* url,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_geo_json_source_url_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_geo_json_source_url(
    self->handle, maplibre_native_ffi_text_view(source_id),
    maplibre_native_ffi_text_view(url), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_geo_json_source_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_global_state_property_async(
  MaplibreNativeFfiMap* self, const char* property_name, const char* value,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_global_state_property_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_global_state_property(
    self->handle, maplibre_native_ffi_text_view(property_name),
    maplibre_native_ffi_text_view(value), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_global_state_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_image_source_coordinates_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  const MaplibreNativeFfiLatLng* coordinates, gsize coordinate_count,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_image_source_coordinates_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_image_source_coordinates(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (const mln_lat_lng*)coordinates, (size_t)(coordinate_count),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_image_source_coordinates_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_image_source_image_async(
  MaplibreNativeFfiMap* self, const char* source_id,
  MaplibreNativeFfiPremultipliedRgba8Image* image, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(image));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_image_source_image_async,
    callback, user_data, TRUE
  );
  mln_premultiplied_rgba8_image image_native_;
  maplibre_native_ffi_premultiplied_rgba8_image_to_native(
    image, &image_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_image_source_image(
    self->handle, maplibre_native_ffi_text_view(source_id), &image_native_,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_image_source_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_image_source_url_async(
  MaplibreNativeFfiMap* self, const char* source_id, const char* url,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_image_source_url_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_image_source_url(
    self->handle, maplibre_native_ffi_text_view(source_id),
    maplibre_native_ffi_text_view(url), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_image_source_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_projection_mode_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiProjectionModeOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PROJECTION_MODE_OPTIONS(options));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_projection_mode_async, callback,
    user_data, TRUE
  );
  mln_projection_mode_options options_native_;
  maplibre_native_ffi_projection_mode_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_projection_mode(
    self->handle, &options_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_projection_mode_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_rendering_stats_view_enabled_async(
  MaplibreNativeFfiMap* self, gboolean enabled, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_map_set_rendering_stats_view_enabled_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_rendering_stats_view_enabled(
    self->handle, (enabled) != FALSE, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_rendering_stats_view_enabled_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_image_async(
  MaplibreNativeFfiMap* self, const char* image_id,
  MaplibreNativeFfiPremultipliedRgba8Image* image,
  MaplibreNativeFfiStyleImageOptions* options, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_PREMULTIPLIED_RGBA8_IMAGE(image));
  g_return_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_STYLE_IMAGE_OPTIONS(options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_image_async, callback,
    user_data, TRUE
  );
  mln_premultiplied_rgba8_image image_native_;
  maplibre_native_ffi_premultiplied_rgba8_image_to_native(
    image, &image_native_, &async_.call
  );
  mln_style_image_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_style_image_options_to_native(
      options, &options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_image(
    self->handle, maplibre_native_ffi_text_view(image_id), &image_native_,
    (options != NULL ? &options_native_ : NULL), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_image_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_json_async(
  MaplibreNativeFfiMap* self, const char* json, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_json_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_json(
    self->handle, maplibre_native_ffi_text_view(json),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_layer_property_async(
  MaplibreNativeFfiMap* self, const char* layer_id, const char* property_name,
  const char* value, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_layer_property_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_layer_property(
    self->handle, maplibre_native_ffi_text_view(layer_id),
    maplibre_native_ffi_text_view(property_name),
    maplibre_native_ffi_text_view(value), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_layer_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_light_json_async(
  MaplibreNativeFfiMap* self, const char* light_json,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_light_json_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_light_json(
    self->handle, maplibre_native_ffi_text_view(light_json),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_light_json_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_light_property_async(
  MaplibreNativeFfiMap* self, const char* property_name, const char* value,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_light_property_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_light_property(
    self->handle, maplibre_native_ffi_text_view(property_name),
    maplibre_native_ffi_text_view(value), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_light_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_root_property_async(
  MaplibreNativeFfiMap* self, const char* property_name, const char* value,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_root_property_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_root_property(
    self->handle, maplibre_native_ffi_text_view(property_name),
    maplibre_native_ffi_text_view(value), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_root_property_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_source_volatile_async(
  MaplibreNativeFfiMap* self, const char* source_id, gboolean is_volatile,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_source_volatile_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_source_volatile(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (is_volatile) != FALSE, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_source_volatile_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_transition_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiStyleTransitionOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_TRANSITION_OPTIONS(options));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_transition_options_async,
    callback, user_data, TRUE
  );
  mln_style_transition_options options_native_;
  maplibre_native_ffi_style_transition_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_transition_options(
    self->handle, &options_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_transition_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_style_url_async(
  MaplibreNativeFfiMap* self, const char* url, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_style_url_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_style_url(
    self->handle, maplibre_native_ffi_text_view(url),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_style_url_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_tile_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiMapTileOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_TILE_OPTIONS(options));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_tile_options_async, callback,
    user_data, TRUE
  );
  mln_map_tile_options options_native_;
  maplibre_native_ffi_map_tile_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_tile_options(
    self->handle, &options_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_tile_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_set_viewport_options_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiMapViewportOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_VIEWPORT_OPTIONS(options));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_set_viewport_options_async, callback,
    user_data, TRUE
  );
  mln_map_viewport_options options_native_;
  maplibre_native_ffi_map_viewport_options_to_native(
    options, &options_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_set_viewport_options(
    self->handle, &options_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_set_viewport_options_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_update_camera_async(
  MaplibreNativeFfiMap* self, MaplibreNativeFfiCameraUpdate* update,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_UPDATE(update));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_map_update_camera_async, callback,
    user_data, TRUE
  );
  mln_camera_update update_native_;
  maplibre_native_ffi_camera_update_to_native(
    update, &update_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_map_update_camera(
    self->handle, &update_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_map_update_camera_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_map_ready_async(
  MaplibreNativeFfiMap* self, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self));
  GTask* task_ = g_task_new(self, NULL, callback, user_data);
  g_task_set_source_tag(task_, maplibre_native_ffi_map_ready_async);
  maplibre_native_ffi_ready_wait(self->ready, task_);
  g_object_unref(task_);
}

gboolean maplibre_native_ffi_map_ready_finish(
  MaplibreNativeFfiMap* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

gboolean maplibre_native_ffi_map_is_event_source(
  MaplibreNativeFfiMap* self, const MaplibreNativeFfiRuntimeEvent* event
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP(self), FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  return maplibre_native_ffi_runtime_event_get_source_type(event) ==
           MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_SOURCE_MAP &&
         maplibre_native_ffi_runtime_event_get_source(event) ==
           maplibre_native_ffi_map_get_id(self);
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiMapProjection, maplibre_native_ffi_map_projection,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_map_projection_object_dispose(GObject* object) {
  MaplibreNativeFfiMapProjection* self =
    MAPLIBRE_NATIVE_FFI_MAP_PROJECTION(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_map_projection_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_map_projection_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_map_projection_class_init(
  MaplibreNativeFfiMapProjectionClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_map_projection_object_dispose;
}

static void maplibre_native_ffi_map_projection_init(
  MaplibreNativeFfiMapProjection* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiMapProjection* maplibre_native_ffi_map_projection_wrap(
  mln_map_projection handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiMapProjection* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_MAP_PROJECTION, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_map_projection_get_id(
  MaplibreNativeFfiMapProjection* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

MaplibreNativeFfiMapProjection* maplibre_native_ffi_map_projection_create(
  const MaplibreNativeFfiMapView* view, GError** error
) {
  g_return_val_if_fail(view != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_map_view view_native_;
  maplibre_native_ffi_map_view_to_native(view, &view_native_, NULL);
  mln_map_projection out_projection_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_map_projection_create(&view_native_, &out_projection_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_map_projection_wrap(
    out_projection_, NULL, FALSE, NULL
  );
}

MaplibreNativeFfiMapProjectionCameraResult*
maplibre_native_ffi_map_projection_get_camera(
  MaplibreNativeFfiMapProjection* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_map_projection_camera_result out_result_;
  memset(&out_result_, 0, sizeof out_result_);
  (&out_result_)->size = sizeof *(&out_result_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_map_projection_get_camera(self->handle, &out_result_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_map_projection_camera_result_from_native(
    &out_result_
  );
}

MaplibreNativeFfiLatLng* maplibre_native_ffi_map_projection_lat_lng_for_pixel(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* point, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(point != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_screen_point point_native_;
  maplibre_native_ffi_screen_point_to_native(point, &point_native_, NULL);
  mln_lat_lng out_coordinate_;
  memset(&out_coordinate_, 0, sizeof out_coordinate_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_lat_lng_for_pixel(
    self->handle, point_native_, &out_coordinate_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_lat_lng_from_native(&out_coordinate_);
}

MaplibreNativeFfiLatLng*
maplibre_native_ffi_map_projection_lat_lng_for_pixel_unwrapped(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* point, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(point != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_screen_point point_native_;
  maplibre_native_ffi_screen_point_to_native(point, &point_native_, NULL);
  mln_lat_lng out_coordinate_;
  memset(&out_coordinate_, 0, sizeof out_coordinate_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_lat_lng_for_pixel_unwrapped(
    self->handle, point_native_, &out_coordinate_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_lat_lng_from_native(&out_coordinate_);
}

GPtrArray* maplibre_native_ffi_map_projection_lat_lngs_for_pixels(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* points, gsize point_count, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  gsize out_coordinates_count_ = (gsize)((size_t)(point_count));
  mln_lat_lng* out_coordinates_ = maplibre_native_ffi_call_alloc(
    &call_, out_coordinates_count_, sizeof *out_coordinates_
  );
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_lat_lngs_for_pixels(
    self->handle, (const mln_screen_point*)points, (size_t)(point_count),
    out_coordinates_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
    return NULL;
  }
  GPtrArray* out_coordinates_items_ = g_ptr_array_new_full(
    (guint)out_coordinates_count_,
    (GDestroyNotify)maplibre_native_ffi_lat_lng_free
  );
  for (gsize index = 0; index < out_coordinates_count_; index++) {
    g_ptr_array_add(
      out_coordinates_items_,
      maplibre_native_ffi_lat_lng_from_native(&out_coordinates_[index])
    );
  }
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  return out_coordinates_items_;
}

GPtrArray* maplibre_native_ffi_map_projection_lat_lngs_for_pixels_unwrapped(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiScreenPoint* points, gsize point_count, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  gsize out_coordinates_count_ = (gsize)((size_t)(point_count));
  mln_lat_lng* out_coordinates_ = maplibre_native_ffi_call_alloc(
    &call_, out_coordinates_count_, sizeof *out_coordinates_
  );
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_lat_lngs_for_pixels_unwrapped(
    self->handle, (const mln_screen_point*)points, (size_t)(point_count),
    out_coordinates_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
    return NULL;
  }
  GPtrArray* out_coordinates_items_ = g_ptr_array_new_full(
    (guint)out_coordinates_count_,
    (GDestroyNotify)maplibre_native_ffi_lat_lng_free
  );
  for (gsize index = 0; index < out_coordinates_count_; index++) {
    g_ptr_array_add(
      out_coordinates_items_,
      maplibre_native_ffi_lat_lng_from_native(&out_coordinates_[index])
    );
  }
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  return out_coordinates_items_;
}

gdouble maplibre_native_ffi_map_projection_meters_per_pixel_at_latitude(
  MaplibreNativeFfiMapProjection* self, gdouble latitude, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), 0);
  g_return_val_if_fail(error == NULL || *error == NULL, 0);
  double out_meters_per_pixel_ = 0;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_meters_per_pixel_at_latitude(
    self->handle, (double)(latitude), &out_meters_per_pixel_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return 0;
  }
  return (gdouble)(out_meters_per_pixel_);
}

MaplibreNativeFfiScreenPoint*
maplibre_native_ffi_map_projection_pixel_for_lat_lng(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiLatLng* coordinate, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(coordinate != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_lat_lng coordinate_native_;
  maplibre_native_ffi_lat_lng_to_native(coordinate, &coordinate_native_, NULL);
  mln_screen_point out_point_;
  memset(&out_point_, 0, sizeof out_point_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_pixel_for_lat_lng(
    self->handle, coordinate_native_, &out_point_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_screen_point_from_native(&out_point_);
}

GPtrArray* maplibre_native_ffi_map_projection_pixels_for_lat_lngs(
  MaplibreNativeFfiMapProjection* self,
  const MaplibreNativeFfiLatLng* coordinates, gsize coordinate_count,
  GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  gsize out_points_count_ = (gsize)((size_t)(coordinate_count));
  mln_screen_point* out_points_ = maplibre_native_ffi_call_alloc(
    &call_, out_points_count_, sizeof *out_points_
  );
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_map_projection_pixels_for_lat_lngs(
    self->handle, (const mln_lat_lng*)coordinates, (size_t)(coordinate_count),
    out_points_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
    return NULL;
  }
  GPtrArray* out_points_items_ = g_ptr_array_new_full(
    (guint)out_points_count_,
    (GDestroyNotify)maplibre_native_ffi_screen_point_free
  );
  for (gsize index = 0; index < out_points_count_; index++) {
    g_ptr_array_add(
      out_points_items_,
      maplibre_native_ffi_screen_point_from_native(&out_points_[index])
    );
  }
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  return out_points_items_;
}

void maplibre_native_ffi_map_projection_release(
  MaplibreNativeFfiMapProjection* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self));
  mln_map_projection_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

gboolean maplibre_native_ffi_map_projection_set_camera(
  MaplibreNativeFfiMapProjection* self, MaplibreNativeFfiCameraOptions* camera,
  GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_PROJECTION(self), FALSE);
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_CAMERA_OPTIONS(camera), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_camera_options camera_native_;
  maplibre_native_ffi_camera_options_to_native(camera, &camera_native_, NULL);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_map_projection_set_camera(self->handle, &camera_native_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiOffscreenDevice, maplibre_native_ffi_offscreen_device,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_offscreen_device_object_dispose(
  GObject* object
) {
  MaplibreNativeFfiOffscreenDevice* self =
    MAPLIBRE_NATIVE_FFI_OFFSCREEN_DEVICE(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_offscreen_device_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_offscreen_device_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_offscreen_device_class_init(
  MaplibreNativeFfiOffscreenDeviceClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_offscreen_device_object_dispose;
}

static void maplibre_native_ffi_offscreen_device_init(
  MaplibreNativeFfiOffscreenDevice* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiOffscreenDevice* maplibre_native_ffi_offscreen_device_wrap(
  mln_offscreen_device handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiOffscreenDevice* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_OFFSCREEN_DEVICE, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_offscreen_device_get_id(
  MaplibreNativeFfiOffscreenDevice* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

MaplibreNativeFfiOffscreenDevice* maplibre_native_ffi_offscreen_device_create(
  MaplibreNativeFfiOffscreenDeviceOptions* options, GError** error
) {
  g_return_val_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE_OPTIONS(options),
    NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_offscreen_device_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_offscreen_device_options_to_native(
      options, &options_native_, NULL
    );
  mln_offscreen_device out_device_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_offscreen_device_create(
    (options != NULL ? &options_native_ : NULL), &out_device_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_offscreen_device_wrap(
    out_device_, NULL, FALSE, NULL
  );
}

void maplibre_native_ffi_offscreen_device_release(
  MaplibreNativeFfiOffscreenDevice* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_OFFSCREEN_DEVICE(self));
  mln_offscreen_device_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiRenderSession, maplibre_native_ffi_render_session,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_render_session_object_dispose(GObject* object) {
  MaplibreNativeFfiRenderSession* self =
    MAPLIBRE_NATIVE_FFI_RENDER_SESSION(object);
  if (self->handle != MLN_HANDLE_NULL) {
    maplibre_native_ffi_render_session_dispose(self);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_render_session_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_render_session_class_init(
  MaplibreNativeFfiRenderSessionClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_render_session_object_dispose;
}

static void maplibre_native_ffi_render_session_init(
  MaplibreNativeFfiRenderSession* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiRenderSession* maplibre_native_ffi_render_session_wrap(
  mln_render_session handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiRenderSession* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_render_session_get_id(
  MaplibreNativeFfiRenderSession* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

MaplibreNativeFfiRenderAbandonResult*
maplibre_native_ffi_render_session_abandon(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_render_abandon_result out_result_;
  memset(&out_result_, 0, sizeof out_result_);
  (&out_result_)->size = sizeof *(&out_result_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_render_session_abandon(self->handle, &out_result_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_render_abandon_result_from_native(&out_result_);
}

MaplibreNativeFfiAcquiredFrame*
maplibre_native_ffi_render_session_acquire_frame(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_acquired_frame out_frame_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_render_session_acquire_frame(self->handle, &out_frame_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return (
    out_frame_ != MLN_HANDLE_NULL
      ? maplibre_native_ffi_acquired_frame_wrap(out_frame_, self, FALSE, NULL)
      : NULL
  );
}

void maplibre_native_ffi_render_session_barrier_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_barrier_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_barrier(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_barrier_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_clear_data_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_clear_data_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_clear_data(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_clear_data_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

gboolean maplibre_native_ffi_render_session_demand_frame(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiFrameDemand* demand, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), FALSE);
  g_return_val_if_fail(demand != NULL, FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_frame_demand demand_native_;
  maplibre_native_ffi_frame_demand_to_native(demand, &demand_native_, NULL);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_render_session_demand_frame(
    self->handle, &demand_native_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

void maplibre_native_ffi_render_session_detach_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_detach_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_detach(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_detach_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_dispose(
  MaplibreNativeFfiRenderSession* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  mln_render_session handle = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (handle != MLN_HANDLE_NULL) {
    (void)mln_render_session_dispose(handle, NULL);
    self->released_handle = handle;
  }
}

GPtrArray* maplibre_native_ffi_render_session_drain_frame_results(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_render_frame_batch batch_ = MLN_HANDLE_NULL;
  mln_render_frame_batch_view view_;
  memset(&view_, 0, sizeof view_);
  (&view_)->size = sizeof *(&view_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_render_session_drain_frame_results(
    self->handle, &batch_, &view_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) return NULL;
  GPtrArray* items_ = g_ptr_array_new_with_free_func(
    (GDestroyNotify)maplibre_native_ffi_render_frame_result_free
  );
  if (batch_ != MLN_HANDLE_NULL) {
    for (size_t index = 0; index < view_.result_count; index++) {
      g_ptr_array_add(
        items_,
        maplibre_native_ffi_render_frame_result_from_native(
          (const mln_render_frame_result*)(const void*)((const char*)
                                                          view_.results +
                                                        index *
                                                          view_.result_size)
        )
      );
    }
    mln_render_frame_batch_release(batch_);
  }
  return items_;
}

void maplibre_native_ffi_render_session_dump_debug_logs_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_dump_debug_logs_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_dump_debug_logs(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_dump_debug_logs_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

MaplibreNativeFfiRenderSessionCapabilities*
maplibre_native_ffi_render_session_get_capabilities(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_render_session_capabilities out_capabilities_;
  memset(&out_capabilities_, 0, sizeof out_capabilities_);
  (&out_capabilities_)->size = sizeof *(&out_capabilities_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_render_session_get_capabilities(
    self->handle, &out_capabilities_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_render_session_capabilities_from_native(
    &out_capabilities_
  );
}

void maplibre_native_ffi_render_session_get_cluster_children_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  guint32 cluster_id, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_get_cluster_children_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_render_session_get_cluster_children_decode, g_free
      ))
    return;
  mln_status status_ = mln_render_session_get_cluster_children(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (uint32_t)(cluster_id), async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_render_session_get_cluster_children_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_get_cluster_expansion_zoom_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  guint32 cluster_id, GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_get_cluster_expansion_zoom_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_render_session_get_cluster_expansion_zoom_decode, g_free
      ))
    return;
  mln_status status_ = mln_render_session_get_cluster_expansion_zoom(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (uint32_t)(cluster_id), async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

guint32 maplibre_native_ffi_render_session_get_cluster_expansion_zoom_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), 0);
  guint32* value = g_task_propagate_pointer(G_TASK(result), error);
  if (value == NULL) return 0;
  guint32 copy = *value;
  g_free(value);
  return copy;
}

void maplibre_native_ffi_render_session_get_cluster_leaves_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  guint32 cluster_id, guint32 limit, guint32 offset,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_get_cluster_leaves_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_render_session_get_cluster_leaves_decode, g_free
      ))
    return;
  mln_status status_ = mln_render_session_get_cluster_leaves(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (uint32_t)(cluster_id), (uint32_t)(limit), (uint32_t)(offset),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

char* maplibre_native_ffi_render_session_get_cluster_leaves_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

char* maplibre_native_ffi_render_session_get_failure(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  size_t capacity_ = 256;
  size_t length_ = 0;
  char* text_ = NULL;
  mln_status status_;
  for (;;) {
    text_ = g_realloc(text_, capacity_);
    status_ = mln_render_session_get_failure(
      self->handle, text_, capacity_, &length_, &diagnostic_
    );
    if (status_ != MLN_STATUS_OK || length_ < capacity_) break;
    capacity_ = length_ + 1;
  }
  char* text_value_ =
    status_ == MLN_STATUS_OK ? g_strndup(text_, length_) : NULL;
  g_free(text_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return text_value_;
}

MaplibreNativeFfiRenderSessionSnapshot*
maplibre_native_ffi_render_session_get_snapshot(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_render_session_snapshot out_snapshot_;
  memset(&out_snapshot_, 0, sizeof out_snapshot_);
  (&out_snapshot_)->size = sizeof *(&out_snapshot_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_render_session_get_snapshot(self->handle, &out_snapshot_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_render_session_snapshot_from_native(
    &out_snapshot_
  );
}

void maplibre_native_ffi_render_session_query_rendered_features_async(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiRenderedQueryGeometry* geometry,
  MaplibreNativeFfiRenderedFeatureQueryOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(geometry != NULL);
  g_return_if_fail(
    options == NULL ||
    MAPLIBRE_NATIVE_FFI_IS_RENDERED_FEATURE_QUERY_OPTIONS(options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_query_rendered_features_async, callback,
    user_data, FALSE
  );
  mln_rendered_query_geometry geometry_native_;
  maplibre_native_ffi_rendered_query_geometry_to_native(
    geometry, &geometry_native_, &async_.call
  );
  mln_rendered_feature_query_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_rendered_feature_query_options_to_native(
      options, &options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_render_session_query_rendered_features_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_render_session_query_rendered_features(
    self->handle, &geometry_native_,
    (options != NULL ? &options_native_ : NULL), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_render_session_query_rendered_features_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_query_source_features_async(
  MaplibreNativeFfiRenderSession* self, const char* source_id,
  MaplibreNativeFfiSourceFeatureQueryOptions* options,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    options == NULL ||
    MAPLIBRE_NATIVE_FFI_IS_SOURCE_FEATURE_QUERY_OPTIONS(options)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_query_source_features_async, callback,
    user_data, FALSE
  );
  mln_source_feature_query_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_source_feature_query_options_to_native(
      options, &options_native_, &async_.call
    );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_render_session_query_source_features_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_render_session_query_source_features(
    self->handle, maplibre_native_ffi_text_view(source_id),
    (options != NULL ? &options_native_ : NULL), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_render_session_query_source_features_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_read_texture_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_read_texture_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_render_session_read_texture_decode,
        (GDestroyNotify)maplibre_native_ffi_texture_readback_result_free
      ))
    return;
  mln_status status_ = mln_render_session_read_texture(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiTextureReadbackResult*
maplibre_native_ffi_render_session_read_texture_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_reduce_memory_use_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_reduce_memory_use_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_reduce_memory_use(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_reduce_memory_use_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_release_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_release_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_release(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  if (status_ == MLN_STATUS_OK) {
    self->released_handle = self->handle;
    self->handle = MLN_HANDLE_NULL;
  }
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_release_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_release_target_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_release_target_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_release_target(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_release_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_resize_async(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiLogicalSize* logical_size,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(logical_size != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_render_session_resize_async, callback,
    user_data, TRUE
  );
  mln_logical_size logical_size_native_;
  maplibre_native_ffi_logical_size_to_native(
    logical_size, &logical_size_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_render_session_resize(
    self->handle, logical_size_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_render_session_resize_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

gboolean maplibre_native_ffi_render_session_service_driver_work(
  MaplibreNativeFfiRenderSession* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_render_session_service_driver_work(
    (self->handle != MLN_HANDLE_NULL ? self->handle : self->released_handle),
    &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

gboolean maplibre_native_ffi_render_session_set_frame_pacer(
  MaplibreNativeFfiRenderSession* self, MaplibreNativeFfiFramePacer* pacer,
  GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_render_session_set_frame_pacer(
    self->handle, (pacer != NULL ? pacer->handle : MLN_HANDLE_NULL),
    &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

#if defined(__APPLE__)
void maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiMetalBorrowedTextureReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_BORROWED_TEXTURE_REPLACEMENT(replacement)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_async,
    callback, user_data, FALSE
  );
  mln_metal_borrowed_texture_replacement replacement_native_;
  maplibre_native_ffi_metal_borrowed_texture_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_metal_borrowed_texture_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}
#endif

#if defined(__APPLE__)
gboolean
maplibre_native_ffi_render_session_set_metal_borrowed_texture_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}
#endif

#if defined(__APPLE__)
void maplibre_native_ffi_render_session_set_metal_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiMetalSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_METAL_SURFACE_REPLACEMENT(replacement)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_metal_surface_target_async, callback,
    user_data, FALSE
  );
  mln_metal_surface_replacement replacement_native_;
  maplibre_native_ffi_metal_surface_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_metal_surface_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}
#endif

#if defined(__APPLE__)
gboolean maplibre_native_ffi_render_session_set_metal_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}
#endif

void maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiOpenGlBorrowedTextureReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_BORROWED_TEXTURE_REPLACEMENT(replacement)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_async,
    callback, user_data, FALSE
  );
  mln_open_gl_borrowed_texture_replacement replacement_native_;
  maplibre_native_ffi_open_gl_borrowed_texture_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_open_gl_borrowed_texture_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean
maplibre_native_ffi_render_session_set_open_gl_borrowed_texture_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_set_open_gl_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiOpenGlSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_OPEN_GL_SURFACE_REPLACEMENT(replacement)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_open_gl_surface_target_async,
    callback, user_data, FALSE
  );
  mln_open_gl_surface_replacement replacement_native_;
  maplibre_native_ffi_open_gl_surface_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_open_gl_surface_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_set_open_gl_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiVulkanBorrowedTextureReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_VULKAN_BORROWED_TEXTURE_REPLACEMENT(replacement)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_async,
    callback, user_data, FALSE
  );
  mln_vulkan_borrowed_texture_replacement replacement_native_;
  maplibre_native_ffi_vulkan_borrowed_texture_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_vulkan_borrowed_texture_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean
maplibre_native_ffi_render_session_set_vulkan_borrowed_texture_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_render_session_set_vulkan_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  const MaplibreNativeFfiVulkanSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(replacement != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_vulkan_surface_target_async,
    callback, user_data, FALSE
  );
  mln_vulkan_surface_replacement replacement_native_;
  maplibre_native_ffi_vulkan_surface_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_vulkan_surface_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_render_session_set_vulkan_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

#if defined(__linux__)
void maplibre_native_ffi_render_session_set_wayland_surface_target_async(
  MaplibreNativeFfiRenderSession* self,
  MaplibreNativeFfiWaylandSurfaceReplacement* replacement,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  g_return_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_WAYLAND_SURFACE_REPLACEMENT(replacement)
  );
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_render_session_set_wayland_surface_target_async,
    callback, user_data, FALSE
  );
  mln_wayland_surface_replacement replacement_native_;
  maplibre_native_ffi_wayland_surface_replacement_to_native(
    replacement, &replacement_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_render_session_set_wayland_surface_target(
    self->handle, &replacement_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}
#endif

#if defined(__linux__)
gboolean maplibre_native_ffi_render_session_set_wayland_surface_target_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}
#endif

void maplibre_native_ffi_render_session_ready_async(
  MaplibreNativeFfiRenderSession* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RENDER_SESSION(self));
  GTask* task_ = g_task_new(self, NULL, callback, user_data);
  g_task_set_source_tag(task_, maplibre_native_ffi_render_session_ready_async);
  maplibre_native_ffi_ready_wait(self->ready, task_);
  g_object_unref(task_);
}

gboolean maplibre_native_ffi_render_session_ready_finish(
  MaplibreNativeFfiRenderSession* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiResourceRequest, maplibre_native_ffi_resource_request,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_resource_request_object_dispose(
  GObject* object
) {
  MaplibreNativeFfiResourceRequest* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_REQUEST(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_resource_request_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_resource_request_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_resource_request_class_init(
  MaplibreNativeFfiResourceRequestClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_resource_request_object_dispose;
}

static void maplibre_native_ffi_resource_request_init(
  MaplibreNativeFfiResourceRequest* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiResourceRequest* maplibre_native_ffi_resource_request_wrap(
  mln_resource_request handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiResourceRequest* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_REQUEST, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_resource_request_get_id(
  MaplibreNativeFfiResourceRequest* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_REQUEST(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

gboolean maplibre_native_ffi_resource_request_complete(
  MaplibreNativeFfiResourceRequest* self,
  MaplibreNativeFfiResourceResponse* response, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_REQUEST(self), FALSE);
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_RESPONSE(response), FALSE
  );
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_resource_response response_native_;
  maplibre_native_ffi_resource_response_to_native(
    response, &response_native_, NULL
  );
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_resource_request_complete(
    self->handle, &response_native_, &diagnostic_
  );
  mln_resource_request consumed_ = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    if (status_ != MLN_STATUS_OK) mln_resource_request_release(consumed_);
    return FALSE;
  }
  if (status_ != MLN_STATUS_OK) mln_resource_request_release(consumed_);
  return TRUE;
}

gboolean maplibre_native_ffi_resource_request_is_cancelled(
  MaplibreNativeFfiResourceRequest* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_REQUEST(self), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  bool out_cancelled_ = 0;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_resource_request_is_cancelled(
    self->handle, &out_cancelled_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return (out_cancelled_) ? TRUE : FALSE;
}

gboolean maplibre_native_ffi_resource_request_pass_through(
  MaplibreNativeFfiResourceRequest* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_REQUEST(self), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_resource_request_pass_through(self->handle, &diagnostic_);
  mln_resource_request consumed_ = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    if (status_ != MLN_STATUS_OK) mln_resource_request_release(consumed_);
    return FALSE;
  }
  if (status_ != MLN_STATUS_OK) mln_resource_request_release(consumed_);
  return TRUE;
}

void maplibre_native_ffi_resource_request_release(
  MaplibreNativeFfiResourceRequest* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_REQUEST(self));
  mln_resource_request_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

gboolean maplibre_native_ffi_resource_request_set_cancel_callback(
  MaplibreNativeFfiResourceRequest* self,
  const MaplibreNativeFfiResourceRequestCancelHandler* handler, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_REQUEST(self), FALSE);
  g_return_val_if_fail(handler != NULL, FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_resource_request_cancel_handler handler_native_;
  maplibre_native_ffi_resource_request_cancel_handler_to_native(
    handler, &handler_native_, &call_
  );
  bool out_cancelled_ = 0;
  if (!maplibre_native_ffi_call_check(&call_, error)) return FALSE;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_resource_request_set_cancel_callback(
    self->handle, &handler_native_, &out_cancelled_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    maplibre_native_ffi_call_end(
      &call_, status_ == MLN_STATUS_OK && !out_cancelled_
    );
    return FALSE;
  }
  maplibre_native_ffi_call_end(
    &call_, status_ == MLN_STATUS_OK && !out_cancelled_
  );
  return (out_cancelled_) ? TRUE : FALSE;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiResourceTransformRequest,
  maplibre_native_ffi_resource_transform_request, G_TYPE_OBJECT
)

static void maplibre_native_ffi_resource_transform_request_object_dispose(
  GObject* object
) {
  MaplibreNativeFfiResourceTransformRequest* self =
    MAPLIBRE_NATIVE_FFI_RESOURCE_TRANSFORM_REQUEST(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_resource_transform_request_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_resource_transform_request_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_resource_transform_request_class_init(
  MaplibreNativeFfiResourceTransformRequestClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_resource_transform_request_object_dispose;
}

static void maplibre_native_ffi_resource_transform_request_init(
  MaplibreNativeFfiResourceTransformRequest* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiResourceTransformRequest*
maplibre_native_ffi_resource_transform_request_wrap(
  mln_resource_transform_request handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiResourceTransformRequest* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_TRANSFORM_REQUEST, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_resource_transform_request_get_id(
  MaplibreNativeFfiResourceTransformRequest* self
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_REQUEST(self), 0
  );
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

gboolean maplibre_native_ffi_resource_transform_request_complete(
  MaplibreNativeFfiResourceTransformRequest* self, const char* url,
  GError** error
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_REQUEST(self), FALSE
  );
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_resource_transform_request_complete(
    self->handle, maplibre_native_ffi_text_view(url), &diagnostic_
  );
  mln_resource_transform_request consumed_ = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    if (status_ != MLN_STATUS_OK)
      mln_resource_transform_request_release(consumed_);
    return FALSE;
  }
  if (status_ != MLN_STATUS_OK)
    mln_resource_transform_request_release(consumed_);
  return TRUE;
}

gboolean maplibre_native_ffi_resource_transform_request_fail(
  MaplibreNativeFfiResourceTransformRequest* self,
  MaplibreNativeFfiResourceTransformError* error_value, GError** error
) {
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_REQUEST(self), FALSE
  );
  g_return_val_if_fail(
    MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_ERROR(error_value), FALSE
  );
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_resource_transform_error error_value_native_;
  maplibre_native_ffi_resource_transform_error_to_native(
    error_value, &error_value_native_, NULL
  );
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_resource_transform_request_fail(
    self->handle, &error_value_native_, &diagnostic_
  );
  mln_resource_transform_request consumed_ = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    if (status_ != MLN_STATUS_OK)
      mln_resource_transform_request_release(consumed_);
    return FALSE;
  }
  if (status_ != MLN_STATUS_OK)
    mln_resource_transform_request_release(consumed_);
  return TRUE;
}

void maplibre_native_ffi_resource_transform_request_release(
  MaplibreNativeFfiResourceTransformRequest* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RESOURCE_TRANSFORM_REQUEST(self));
  mln_resource_transform_request_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiRuntime, maplibre_native_ffi_runtime, G_TYPE_OBJECT
)

static void maplibre_native_ffi_runtime_object_dispose(GObject* object) {
  MaplibreNativeFfiRuntime* self = MAPLIBRE_NATIVE_FFI_RUNTIME(object);
  if (self->handle != MLN_HANDLE_NULL) {
    maplibre_native_ffi_runtime_dispose(self);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_runtime_parent_class)->dispose(object);
}

static void maplibre_native_ffi_runtime_class_init(
  MaplibreNativeFfiRuntimeClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose = maplibre_native_ffi_runtime_object_dispose;
}

static void maplibre_native_ffi_runtime_init(MaplibreNativeFfiRuntime* self) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiRuntime* maplibre_native_ffi_runtime_wrap(
  mln_runtime handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiRuntime* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_runtime_get_id(MaplibreNativeFfiRuntime* self) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

void maplibre_native_ffi_runtime_clear_event_handler_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_clear_event_handler_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_clear_event_handler(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_event_handler_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_clear_http_header_transform_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_runtime_clear_http_header_transform_async, callback,
    user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_clear_http_header_transform(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_http_header_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_clear_resource_provider_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_clear_resource_provider_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_clear_resource_provider(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_resource_provider_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_clear_resource_transform_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_clear_resource_transform_async,
    callback, user_data, TRUE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_clear_resource_transform(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_clear_resource_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

MaplibreNativeFfiRuntime* maplibre_native_ffi_runtime_create(
  MaplibreNativeFfiRuntimeOptions* options, GError** error
) {
  g_return_val_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_RUNTIME_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_runtime_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_runtime_options_to_native(
      options, &options_native_, NULL
    );
  mln_runtime out_runtime_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_runtime_create(
    (options != NULL ? &options_native_ : NULL), &out_runtime_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_runtime_wrap(out_runtime_, NULL, FALSE, NULL);
}

MaplibreNativeFfiMap* maplibre_native_ffi_runtime_create_map(
  MaplibreNativeFfiRuntime* self, MaplibreNativeFfiMapOptions* options,
  GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self), NULL);
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_MAP_OPTIONS(options), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_map_options options_native_;
  maplibre_native_ffi_map_options_to_native(options, &options_native_, &call_);
  mln_map out_map_ = MLN_HANDLE_NULL;
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  MaplibreNativeFfiReady* ready_ = maplibre_native_ffi_ready_new();
  mln_completion completion_;
  if (!maplibre_native_ffi_completion_init_ready(&completion_, ready_, error)) {
    maplibre_native_ffi_call_end(&call_, FALSE);
    maplibre_native_ffi_ready_unref(ready_);
    return NULL;
  }
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_runtime_create_map(
    self->handle, &options_native_, &out_map_, &completion_, &diagnostic_
  );
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  if (status_ != MLN_STATUS_OK) {
    maplibre_native_ffi_completion_reject(&completion_);
    maplibre_native_ffi_ready_unref(ready_);
    (void)maplibre_native_ffi_check(status_, &diagnostic_, error);
    return NULL;
  }
  MaplibreNativeFfiMap* object_ =
    maplibre_native_ffi_map_wrap(out_map_, self, FALSE, ready_);
  maplibre_native_ffi_ready_unref(ready_);
  return object_;
}

void maplibre_native_ffi_runtime_create_offline_region_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiOfflineRegionDefinition* definition, GBytes* metadata,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  g_return_if_fail(definition != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_create_offline_region_async,
    callback, user_data, FALSE
  );
  mln_offline_region_definition definition_native_;
  maplibre_native_ffi_offline_region_definition_to_native(
    definition, &definition_native_, &async_.call
  );
  mln_buffer_view metadata_view_ = maplibre_native_ffi_bytes_view(metadata);
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_runtime_create_offline_region_decode,
        (GDestroyNotify)maplibre_native_ffi_offline_region_info_free
      ))
    return;
  mln_status status_ = mln_runtime_create_offline_region(
    self->handle, &definition_native_, metadata_view_.data, metadata_view_.size,
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_runtime_create_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_delete_offline_region_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_delete_offline_region_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_delete_offline_region(
    self->handle, (mln_offline_region_id)(region_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_delete_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_dispose(MaplibreNativeFfiRuntime* self) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  mln_runtime handle = self->handle;
  self->handle = MLN_HANDLE_NULL;
  if (handle != MLN_HANDLE_NULL) {
    (void)mln_runtime_dispose(handle, NULL);
    self->released_handle = handle;
  }
}

void maplibre_native_ffi_runtime_get_offline_region_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_get_offline_region_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_runtime_get_offline_region_decode,
        (GDestroyNotify)maplibre_native_ffi_offline_region_info_free
      ))
    return;
  mln_status status_ = mln_runtime_get_offline_region(
    self->handle, (mln_offline_region_id)(region_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_runtime_get_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_get_offline_region_status_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_get_offline_region_status_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_runtime_get_offline_region_status_decode,
        (GDestroyNotify)maplibre_native_ffi_offline_region_status_free
      ))
    return;
  mln_status status_ = mln_runtime_get_offline_region_status(
    self->handle, (mln_offline_region_id)(region_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiOfflineRegionStatus*
maplibre_native_ffi_runtime_get_offline_region_status_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_invalidate_offline_region_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_invalidate_offline_region_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_invalidate_offline_region(
    self->handle, (mln_offline_region_id)(region_id),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_invalidate_offline_region_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_list_offline_regions_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_list_offline_regions_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_runtime_list_offline_regions_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_runtime_list_offline_regions(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_runtime_list_offline_regions_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_merge_offline_regions_async(
  MaplibreNativeFfiRuntime* self, const char* side_database_path,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_merge_offline_regions_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_runtime_merge_offline_regions_decode,
        (GDestroyNotify)g_ptr_array_unref
      ))
    return;
  mln_status status_ = mln_runtime_merge_offline_regions(
    self->handle, maplibre_native_ffi_text_view(side_database_path),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

GPtrArray* maplibre_native_ffi_runtime_merge_offline_regions_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_release_async(
  MaplibreNativeFfiRuntime* self, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_release_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_release(
    self->handle, async_.completion_argument, &async_.diagnostic
  );
  if (status_ == MLN_STATUS_OK) {
    self->released_handle = self->handle;
    self->handle = MLN_HANDLE_NULL;
  }
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_release_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_run_cache_database_operation_async(
  MaplibreNativeFfiRuntime* self,
  MaplibreNativeFfiCacheDatabaseOperation operation,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_runtime_run_cache_database_operation_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_run_cache_database_operation(
    self->handle, (mln_cache_database_operation)(operation),
    async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_run_cache_database_operation_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_event_handler_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiRuntimeEventHandler* handler,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  g_return_if_fail(handler != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_set_event_handler_async,
    callback, user_data, TRUE
  );
  mln_runtime_event_handler handler_native_;
  maplibre_native_ffi_runtime_event_handler_to_native(
    handler, &handler_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_set_event_handler(
    self->handle, &handler_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_event_handler_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_http_header_transform_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiHttpHeaderTransform* transform,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  g_return_if_fail(transform != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_set_http_header_transform_async,
    callback, user_data, TRUE
  );
  mln_http_header_transform transform_native_;
  maplibre_native_ffi_http_header_transform_to_native(
    transform, &transform_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_set_http_header_transform(
    self->handle, &transform_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_http_header_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_async(
  MaplibreNativeFfiRuntime* self, guint64 size, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_set_maximum_ambient_cache_size(
    self->handle, (uint64_t)(size), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_set_maximum_ambient_cache_size_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_offline_region_download_state_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  MaplibreNativeFfiOfflineRegionDownloadState state,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_runtime_set_offline_region_download_state_async,
    callback, user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_set_offline_region_download_state(
    self->handle, (mln_offline_region_id)(region_id),
    (mln_offline_region_download_state)(state), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_set_offline_region_download_state_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_offline_region_observed_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id,
  MaplibreNativeFfiRuntimeEventFlag events, GAsyncReadyCallback callback,
  gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_runtime_set_offline_region_observed_async, callback,
    user_data, FALSE
  );
  if (!maplibre_native_ffi_async_begin(&async_, NULL, NULL)) return;
  mln_status status_ = mln_runtime_set_offline_region_observed(
    self->handle, (mln_offline_region_id)(region_id),
    (mln_runtime_event_flags)(events), async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

gboolean maplibre_native_ffi_runtime_set_offline_region_observed_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), FALSE);
  return g_task_propagate_boolean(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_resource_provider_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiResourceProvider* provider,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  g_return_if_fail(provider != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_set_resource_provider_async,
    callback, user_data, TRUE
  );
  mln_resource_provider provider_native_;
  maplibre_native_ffi_resource_provider_to_native(
    provider, &provider_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_set_resource_provider(
    self->handle, &provider_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_resource_provider_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_set_resource_transform_async(
  MaplibreNativeFfiRuntime* self,
  const MaplibreNativeFfiResourceTransform* transform,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  g_return_if_fail(transform != NULL);
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self, maplibre_native_ffi_runtime_set_resource_transform_async,
    callback, user_data, TRUE
  );
  mln_resource_transform transform_native_;
  maplibre_native_ffi_resource_transform_to_native(
    transform, &transform_native_, &async_.call
  );
  if (!maplibre_native_ffi_async_begin(
        &async_, maplibre_native_ffi_command_decode,
        (GDestroyNotify)maplibre_native_ffi_command_completion_free
      ))
    return;
  mln_status status_ = mln_runtime_set_resource_transform(
    self->handle, &transform_native_, async_.completion_argument,
    &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiCommandCompletion*
maplibre_native_ffi_runtime_set_resource_transform_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

void maplibre_native_ffi_runtime_update_offline_region_metadata_async(
  MaplibreNativeFfiRuntime* self, gint64 region_id, GBytes* metadata,
  GAsyncReadyCallback callback, gpointer user_data
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self));
  MaplibreNativeFfiAsync async_;
  maplibre_native_ffi_async_init(
    &async_, self,
    maplibre_native_ffi_runtime_update_offline_region_metadata_async, callback,
    user_data, FALSE
  );
  mln_buffer_view metadata_view_ = maplibre_native_ffi_bytes_view(metadata);
  if (!maplibre_native_ffi_async_begin(
        &async_, mln_runtime_update_offline_region_metadata_decode,
        (GDestroyNotify)maplibre_native_ffi_offline_region_info_free
      ))
    return;
  mln_status status_ = mln_runtime_update_offline_region_metadata(
    self->handle, (mln_offline_region_id)(region_id), metadata_view_.data,
    metadata_view_.size, async_.completion_argument, &async_.diagnostic
  );
  maplibre_native_ffi_async_end(&async_, status_);
}

MaplibreNativeFfiOfflineRegionInfo*
maplibre_native_ffi_runtime_update_offline_region_metadata_finish(
  MaplibreNativeFfiRuntime* self, GAsyncResult* result, GError** error
) {
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);
  return g_task_propagate_pointer(G_TASK(result), error);
}

gboolean maplibre_native_ffi_runtime_is_event_source(
  MaplibreNativeFfiRuntime* self, const MaplibreNativeFfiRuntimeEvent* event
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_RUNTIME(self), FALSE);
  g_return_val_if_fail(event != NULL, FALSE);
  return maplibre_native_ffi_runtime_event_get_source_type(event) ==
           MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_SOURCE_RUNTIME &&
         maplibre_native_ffi_runtime_event_get_source(event) ==
           maplibre_native_ffi_runtime_get_id(self);
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiStyleExpression, maplibre_native_ffi_style_expression,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_style_expression_object_dispose(
  GObject* object
) {
  MaplibreNativeFfiStyleExpression* self =
    MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_style_expression_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_style_expression_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_style_expression_class_init(
  MaplibreNativeFfiStyleExpressionClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_style_expression_object_dispose;
}

static void maplibre_native_ffi_style_expression_init(
  MaplibreNativeFfiStyleExpression* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiStyleExpression* maplibre_native_ffi_style_expression_wrap(
  mln_style_expression handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiStyleExpression* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_style_expression_get_id(
  MaplibreNativeFfiStyleExpression* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

MaplibreNativeFfiStyleExpression* maplibre_native_ffi_style_expression_create(
  const char* expression, MaplibreNativeFfiStyleExpressionOptions* options,
  GError** error
) {
  g_return_val_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_OPTIONS(options),
    NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_style_expression_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_style_expression_options_to_native(
      options, &options_native_, NULL
    );
  mln_style_expression out_expression_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_style_expression_create(
    maplibre_native_ffi_text_view(expression),
    (options != NULL ? &options_native_ : NULL), &out_expression_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_style_expression_wrap(
    out_expression_, NULL, FALSE, NULL
  );
}

char* maplibre_native_ffi_style_expression_evaluate(
  MaplibreNativeFfiStyleExpression* self,
  MaplibreNativeFfiStyleExpressionInputs* inputs, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION(self), NULL);
  g_return_val_if_fail(
    inputs == NULL || MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION_INPUTS(inputs),
    NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_style_expression_inputs inputs_native_;
  if (inputs != NULL)
    maplibre_native_ffi_style_expression_inputs_to_native(
      inputs, &inputs_native_, &call_
    );
  if (!maplibre_native_ffi_call_check(&call_, error)) return NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  size_t capacity_ = 256;
  size_t length_ = 0;
  char* text_ = NULL;
  mln_status status_;
  for (;;) {
    text_ = g_realloc(text_, capacity_);
    status_ = mln_style_expression_evaluate(
      self->handle, (inputs != NULL ? &inputs_native_ : NULL), text_, capacity_,
      &length_, &diagnostic_
    );
    if (status_ != MLN_STATUS_OK || length_ < capacity_) break;
    capacity_ = length_ + 1;
  }
  char* text_value_ =
    status_ == MLN_STATUS_OK ? g_strndup(text_, length_) : NULL;
  g_free(text_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
    return NULL;
  }
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  return text_value_;
}

MaplibreNativeFfiStyleExpressionInfo*
maplibre_native_ffi_style_expression_get_info(
  MaplibreNativeFfiStyleExpression* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_style_expression_info out_info_;
  memset(&out_info_, 0, sizeof out_info_);
  (&out_info_)->size = sizeof *(&out_info_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_style_expression_get_info(self->handle, &out_info_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_style_expression_info_from_native(&out_info_);
}

void maplibre_native_ffi_style_expression_release(
  MaplibreNativeFfiStyleExpression* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_STYLE_EXPRESSION(self));
  mln_style_expression_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

G_DEFINE_FINAL_TYPE(
  MaplibreNativeFfiVulkanDevice, maplibre_native_ffi_vulkan_device,
  G_TYPE_OBJECT
)

static void maplibre_native_ffi_vulkan_device_object_dispose(GObject* object) {
  MaplibreNativeFfiVulkanDevice* self =
    MAPLIBRE_NATIVE_FFI_VULKAN_DEVICE(object);
  if (self->handle != MLN_HANDLE_NULL) {
    (void)mln_vulkan_device_release(self->handle);
    self->handle = MLN_HANDLE_NULL;
  }
  g_clear_object(&self->parent);
  g_clear_pointer(&self->ready, maplibre_native_ffi_ready_unref);
  G_OBJECT_CLASS(maplibre_native_ffi_vulkan_device_parent_class)
    ->dispose(object);
}

static void maplibre_native_ffi_vulkan_device_class_init(
  MaplibreNativeFfiVulkanDeviceClass* klass
) {
  G_OBJECT_CLASS(klass)->dispose =
    maplibre_native_ffi_vulkan_device_object_dispose;
}

static void maplibre_native_ffi_vulkan_device_init(
  MaplibreNativeFfiVulkanDevice* self
) {
  self->handle = MLN_HANDLE_NULL;
  self->released_handle = MLN_HANDLE_NULL;
}

MaplibreNativeFfiVulkanDevice* maplibre_native_ffi_vulkan_device_wrap(
  mln_vulkan_device handle, gpointer parent, gboolean borrowed,
  MaplibreNativeFfiReady* ready
) {
  MaplibreNativeFfiVulkanDevice* self =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_DEVICE, NULL);
  self->handle = handle;
  self->parent = parent != NULL ? g_object_ref(G_OBJECT(parent)) : NULL;
  self->borrowed = borrowed;
  if (ready != NULL) {
    // The wrapper takes its own reference, which the caller keeps.
    self->ready = maplibre_native_ffi_ready_ref(ready);
  }
  return self;
}

guint64 maplibre_native_ffi_vulkan_device_get_id(
  MaplibreNativeFfiVulkanDevice* self
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE(self), 0);
  return (guint64)(self->handle != MLN_HANDLE_NULL ? self->handle
                                                   : self->released_handle);
}

MaplibreNativeFfiVulkanDevice* maplibre_native_ffi_vulkan_device_create(
  MaplibreNativeFfiVulkanDeviceOptions* options, GError** error
) {
  g_return_val_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE_OPTIONS(options),
    NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_vulkan_device_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_vulkan_device_options_to_native(
      options, &options_native_, NULL
    );
  mln_vulkan_device out_device_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_vulkan_device_create(
    (options != NULL ? &options_native_ : NULL), &out_device_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_vulkan_device_wrap(out_device_, NULL, FALSE, NULL);
}

MaplibreNativeFfiVulkanDeviceContextResult*
maplibre_native_ffi_vulkan_device_get_context(
  MaplibreNativeFfiVulkanDevice* self, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE(self), NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_vulkan_device_context_result out_result_;
  memset(&out_result_, 0, sizeof out_result_);
  (&out_result_)->size = sizeof *(&out_result_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_vulkan_device_get_context(self->handle, &out_result_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_vulkan_device_context_result_from_native(
    &out_result_
  );
}

void maplibre_native_ffi_vulkan_device_release(
  MaplibreNativeFfiVulkanDevice* self
) {
  g_return_if_fail(MAPLIBRE_NATIVE_FFI_IS_VULKAN_DEVICE(self));
  mln_vulkan_device_release(self->handle);
  self->handle = MLN_HANDLE_NULL;
}

#if defined(__APPLE__)
MaplibreNativeFfiFramePacer* maplibre_native_ffi_apple_frame_pacer_create(
  MaplibreNativeFfiFramePacerOptions* options, GError** error
) {
  g_return_val_if_fail(
    options == NULL || MAPLIBRE_NATIVE_FFI_IS_FRAME_PACER_OPTIONS(options), NULL
  );
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_frame_pacer_options options_native_;
  if (options != NULL)
    maplibre_native_ffi_frame_pacer_options_to_native(
      options, &options_native_, NULL
    );
  mln_frame_pacer out_pacer_ = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_apple_frame_pacer_create(
    (options != NULL ? &options_native_ : NULL), &out_pacer_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_frame_pacer_wrap(out_pacer_, NULL, FALSE, NULL);
}
#endif

guint32 maplibre_native_ffi_c_version(void) {
  uint32_t result_ = mln_c_version();
  return (guint32)(result_);
}

MaplibreNativeFfiCameraOptions* maplibre_native_ffi_camera_for_tile(
  const MaplibreNativeFfiCanonicalTileId* tile,
  const MaplibreNativeFfiLogicalSize* logical_size, GError** error
) {
  g_return_val_if_fail(tile != NULL, NULL);
  g_return_val_if_fail(logical_size != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_canonical_tile_id tile_native_;
  maplibre_native_ffi_canonical_tile_id_to_native(tile, &tile_native_, NULL);
  mln_logical_size logical_size_native_;
  maplibre_native_ffi_logical_size_to_native(
    logical_size, &logical_size_native_, NULL
  );
  mln_camera_options out_camera_;
  memset(&out_camera_, 0, sizeof out_camera_);
  (&out_camera_)->size = sizeof *(&out_camera_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_camera_for_tile(
    tile_native_, logical_size_native_, &out_camera_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_camera_options_from_native(&out_camera_);
}

MaplibreNativeFfiLatLng* maplibre_native_ffi_lat_lng_for_projected_meters(
  const MaplibreNativeFfiProjectedMeters* meters, GError** error
) {
  g_return_val_if_fail(meters != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_projected_meters meters_native_;
  maplibre_native_ffi_projected_meters_to_native(meters, &meters_native_, NULL);
  mln_lat_lng out_coordinate_;
  memset(&out_coordinate_, 0, sizeof out_coordinate_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_lat_lng_for_projected_meters(
    meters_native_, &out_coordinate_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_lat_lng_from_native(&out_coordinate_);
}

gboolean maplibre_native_ffi_clear_log_callback(GError** error) {
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_log_clear_callback(&diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

gboolean maplibre_native_ffi_set_log_async_severity_mask(
  MaplibreNativeFfiLogSeverityFlag mask, GError** error
) {
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_log_set_async_severity_mask(
    (mln_log_severity_flags)(mask), &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

gboolean maplibre_native_ffi_set_log_callback(
  MaplibreNativeFfiLogHandler* handler, GError** error
) {
  g_return_val_if_fail(MAPLIBRE_NATIVE_FFI_IS_LOG_HANDLER(handler), FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  MaplibreNativeFfiCall call_;
  maplibre_native_ffi_call_init(&call_);
  mln_log_handler handler_native_;
  maplibre_native_ffi_log_handler_to_native(handler, &handler_native_, &call_);
  if (!maplibre_native_ffi_call_check(&call_, error)) return FALSE;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_log_set_callback(&handler_native_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
    return FALSE;
  }
  maplibre_native_ffi_call_end(&call_, status_ == MLN_STATUS_OK);
  return TRUE;
}

gboolean maplibre_native_ffi_logical_extent_physical_size(
  const MaplibreNativeFfiLogicalExtent* extent, guint32* out_width,
  guint32* out_height, GError** error
) {
  g_return_val_if_fail(extent != NULL, FALSE);
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_logical_extent extent_native_;
  maplibre_native_ffi_logical_extent_to_native(extent, &extent_native_, NULL);
  uint32_t out_width_ = 0;
  uint32_t out_height_ = 0;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_logical_extent_physical_size(
    extent_native_, &out_width_, &out_height_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  if (out_width != NULL) *out_width = (guint32)(out_width_);
  if (out_height != NULL) *out_height = (guint32)(out_height_);
  return TRUE;
}

MaplibreNativeFfiNetworkStatus maplibre_native_ffi_get_network_status(
  GError** error
) {
  g_return_val_if_fail(
    error == NULL || *error == NULL, (MaplibreNativeFfiNetworkStatus)0
  );
  mln_network_status out_status_ = 0;
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_network_get_status(&out_status_, &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return (MaplibreNativeFfiNetworkStatus)0;
  }
  return (MaplibreNativeFfiNetworkStatus)(out_status_);
}

gboolean maplibre_native_ffi_set_network_status(
  MaplibreNativeFfiNetworkStatus status, GError** error
) {
  g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ =
    mln_network_set_status((mln_network_status)(status), &diagnostic_);
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return FALSE;
  }
  return TRUE;
}

MaplibreNativeFfiOpenGlContextProviderFlag
maplibre_native_ffi_open_gl_supported_context_provider_mask(void) {
  mln_open_gl_context_provider_flags result_ =
    mln_open_gl_supported_context_provider_mask();
  return (MaplibreNativeFfiOpenGlContextProviderFlag)(result_);
}

gpointer maplibre_native_ffi_get_plugin_register_function_v1(void) {
  mln_plugin_register_function_v1 result_ =
    mln_plugin_get_register_function_v1();
  return (gpointer)(result_);
}

MaplibreNativeFfiProjectedMeters*
maplibre_native_ffi_projected_meters_for_lat_lng(
  const MaplibreNativeFfiLatLng* coordinate, GError** error
) {
  g_return_val_if_fail(coordinate != NULL, NULL);
  g_return_val_if_fail(error == NULL || *error == NULL, NULL);
  mln_lat_lng coordinate_native_;
  maplibre_native_ffi_lat_lng_to_native(coordinate, &coordinate_native_, NULL);
  mln_projected_meters out_meters_;
  memset(&out_meters_, 0, sizeof out_meters_);
  mln_diagnostic diagnostic_ = {.size = sizeof diagnostic_};
  mln_status status_ = mln_projected_meters_for_lat_lng(
    coordinate_native_, &out_meters_, &diagnostic_
  );
  if (!maplibre_native_ffi_check(status_, &diagnostic_, error)) {
    return NULL;
  }
  return maplibre_native_ffi_projected_meters_from_native(&out_meters_);
}

MaplibreNativeFfiRenderBackendFlag
maplibre_native_ffi_supported_render_backend_mask(void) {
  mln_render_backend_flags result_ = mln_supported_render_backend_mask();
  return (MaplibreNativeFfiRenderBackendFlag)(result_);
}
