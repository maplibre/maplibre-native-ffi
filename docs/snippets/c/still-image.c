// Renders one map image with no window and copies it to host memory. The map
// is in static mode and the session owns its texture. EGL names the host's
// offscreen context here; Metal and Vulkan carry their own handles in their own
// descriptor type and attach with their own function.

#include <maplibre_native_c.h>
#include <stdlib.h>
#include <time.h>

static mln_render_session attach_owned_texture(
  mln_map map, void* egl_display, void* egl_config, void* egl_share_context,
  uint32_t width, uint32_t height
) {
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = 1.0;

  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor.context.data.egl.display = egl_display;
  descriptor.context.data.egl.config = egl_config;
  descriptor.context.data.egl.share_context = egl_share_context;

  mln_render_session session = MLN_HANDLE_NULL;
  const mln_status status =
    mln_opengl_owned_texture_attach(map, &descriptor, &session);
  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

static bool await_still_image(
  mln_runtime runtime, mln_map map, mln_render_session session
) {
  bool finished = false;
  bool rendered = false;
  const time_t deadline = time(NULL) + 30;

  while (!(finished && rendered) && time(NULL) < deadline) {
    mln_runtime_pump(runtime, 10);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (event.source != map) continue;
      if (event.type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED) {
        finished = true;
      } else if (
        event.type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED ||
        event.type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED
      ) {
        return false;
      }
    }

    bool produced = false;
    const mln_status status =
      mln_render_session_render_update(session, &produced);
    if (status == MLN_STATUS_OK && produced) rendered = true;
  }

  return finished && rendered;
}

// Returns premultiplied RGBA8 pixels the caller frees, and fills out_info with
// the width, height, and row stride they use.
uint8_t* render_still_image(
  mln_runtime runtime, const char* style_url, uint32_t width, uint32_t height,
  void* egl_display, void* egl_config, void* egl_share_context,
  mln_texture_image_info* out_info
) {
  mln_map_options options = mln_map_options_default();
  options.width = width;
  options.height = height;
  options.scale_factor = 1.0;
  options.map_mode = MLN_MAP_MODE_STATIC;

  mln_map map = MLN_HANDLE_NULL;
  if (mln_map_create(runtime, &options, &map) != MLN_STATUS_OK) return NULL;

  uint8_t* pixels = NULL;
  const mln_render_session session = attach_owned_texture(
    map, egl_display, egl_config, egl_share_context, width, height
  );

  if (
    session != MLN_HANDLE_NULL &&
    mln_map_set_style_url(map, style_url) == MLN_STATUS_OK &&
    mln_map_request_still_image(map) == MLN_STATUS_OK &&
    await_still_image(runtime, map, session)
  ) {
    mln_texture_image_info info = mln_texture_image_info_default();
    mln_texture_read_premultiplied_rgba8(session, NULL, 0, &info);

    pixels = malloc(info.byte_length);
    if (pixels != NULL) {
      const mln_status read = mln_texture_read_premultiplied_rgba8(
        session, pixels, info.byte_length, &info
      );
      if (read == MLN_STATUS_OK) {
        *out_info = info;
      } else {
        free(pixels);
        pixels = NULL;
      }
    }
  }

  if (session != MLN_HANDLE_NULL) mln_render_session_destroy(session);
  mln_map_destroy(map);
  return pixels;
}
