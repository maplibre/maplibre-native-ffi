// Renders one map image offscreen and copies it to host memory. The map uses
// static mode and the session owns its texture.

#include <maplibre_native_c.h>
#include <stdlib.h>
#include <string.h>

static mln_status wait_until_complete(mln_operation operation) {
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  if (status != MLN_STATUS_OK) return status;
  if (!completed) return MLN_STATUS_NOT_READY;
  mln_status terminal = MLN_STATUS_OK;
  return mln_operation_get_status(operation, &terminal) == MLN_STATUS_OK
           ? terminal
           : MLN_STATUS_NATIVE_ERROR;
}

static mln_render_session attach_owned_texture(
  mln_map map, void* egl_display, void* egl_config, uint32_t width,
  uint32_t height
) {
  // #region attach
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor.context.ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;
  descriptor.context.data.egl.display = egl_display;
  descriptor.context.data.egl.config = egl_config;
  descriptor.context.data.egl.share_context = NULL;
  descriptor.context.data.egl.client_api = MLN_OPENGL_CLIENT_API_GLES;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  options.requested_texture_ring_depth = 1;

  mln_render_session session = MLN_HANDLE_NULL;
  mln_operation attach = MLN_HANDLE_NULL;
  mln_status status = mln_opengl_owned_texture_attach_start(
    map, &descriptor, &options, &session, &attach
  );
  if (status == MLN_STATUS_OK) status = wait_until_complete(attach);
  mln_operation_release(attach);
  // #endregion attach
  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

static bool await_still_image(
  mln_render_session session, mln_operation still_operation
) {
  // #region await
  mln_frame_demand demand = mln_frame_demand_default();
  demand.token = 1;
  if (mln_render_session_request_frame(session, &demand) != MLN_STATUS_OK)
    return false;

  bool still_completed = false;
  bool rendered = false;
  bool demand_pending = true;
  while (!still_completed || !rendered) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    if (
      mln_render_session_drain_frame_results(session, &batch) == MLN_STATUS_OK
    ) {
      size_t count = 0;
      mln_render_frame_batch_count(batch, &count);
      for (size_t index = 0; index < count; ++index) {
        mln_render_frame_result result = {.size = sizeof(result)};
        if (
          mln_render_frame_batch_get(batch, index, &result) == MLN_STATUS_OK &&
          result.token == demand.token
        ) {
          demand_pending = false;
          rendered =
            rendered || result.disposition == MLN_RENDER_RESULT_RENDERED;
        }
      }
      mln_render_frame_batch_release(batch);
    }
    if (
      mln_operation_wait(still_operation, 1, &still_completed) != MLN_STATUS_OK
    )
      return false;
    if (!demand_pending && (!still_completed || !rendered)) {
      if (mln_render_session_request_frame(session, &demand) != MLN_STATUS_OK)
        return false;
      demand_pending = true;
    }
  }
  // #endregion await
  return wait_until_complete(still_operation) == MLN_STATUS_OK;
}

static uint8_t* read_pixels(
  mln_render_session session, mln_texture_image_info* out_info
) {
  // #region read
  mln_operation readback = MLN_HANDLE_NULL;
  mln_status status =
    mln_texture_read_premultiplied_rgba8_start(session, &readback);
  if (status == MLN_STATUS_OK) status = wait_until_complete(readback);
  mln_buffer pixels = MLN_HANDLE_NULL;
  mln_texture_image_info info = mln_texture_image_info_default();
  if (status == MLN_STATUS_OK)
    status = mln_texture_read_premultiplied_rgba8_take_result(
      readback, &pixels, &info
    );
  mln_operation_release(readback);
  mln_buffer_view view = {0};
  if (status == MLN_STATUS_OK) status = mln_buffer_get(pixels, &view);
  uint8_t* copy = status == MLN_STATUS_OK ? malloc(view.size) : NULL;
  if (copy != NULL) memcpy(copy, view.data, view.size);
  mln_buffer_destroy(pixels);
  // #endregion read
  if (copy != NULL) *out_info = info;
  return copy;
}

uint8_t* render_still_image(
  mln_notification_source notifications, mln_runtime runtime,
  const char* style_url, uint32_t width, uint32_t height, void* egl_display,
  void* egl_config, mln_texture_image_info* out_info
) {
  (void)notifications;
  // #region create
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = width, .height = height, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_operation create = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_status status = mln_map_create_start(runtime, &options, &create);
  bool completed = false;
  if (status == MLN_STATUS_OK)
    status = mln_operation_wait(create, -1, &completed);
  if (status == MLN_STATUS_OK && completed)
    status = mln_map_create_take_result(create, &map);
  mln_operation_release(create);
  // #endregion create
  if (status != MLN_STATUS_OK) return NULL;

  uint64_t command_id = 0;
  uint8_t* pixels = NULL;
  mln_render_session session =
    attach_owned_texture(map, egl_display, egl_config, width, height);
  mln_operation still = MLN_HANDLE_NULL;
  if (
    session != MLN_HANDLE_NULL &&
    mln_map_set_style_url(map, style_url, &command_id) == MLN_STATUS_OK &&
    mln_map_request_still_image_start(map, &still) == MLN_STATUS_OK &&
    await_still_image(session, still)
  )
    pixels = read_pixels(session, out_info);
  mln_operation_release(still);

  if (session != MLN_HANDLE_NULL) {
    mln_operation detach = MLN_HANDLE_NULL;
    if (mln_render_session_detach_start(session, &detach) == MLN_STATUS_OK)
      (void)wait_until_complete(detach);
    mln_operation_release(detach);
    (void)mln_render_session_destroy(session);
  }
  mln_operation close = MLN_HANDLE_NULL;
  if (mln_map_close_start(map, &close) == MLN_STATUS_OK) {
    completed = false;
    (void)mln_operation_wait(close, -1, &completed);
  }
  mln_operation_release(close);
  return pixels;
}
