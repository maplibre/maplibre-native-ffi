// Renders one map image offscreen and copies it to host memory. The map uses
// static mode and the session owns its texture.

#include <maplibre_native_c.h>
#include <stdlib.h>
#include <time.h>

typedef enum still_image_state {
  STILL_IMAGE_PENDING,
  STILL_IMAGE_FINISHED,
  STILL_IMAGE_FAILED,
} still_image_state;

static mln_render_session attach_owned_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t width,
  uint32_t height
) {
  // #region attach
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context = *context;

  mln_render_session session = MLN_HANDLE_NULL;
  const mln_status status =
    mln_opengl_owned_texture_attach(map, &descriptor, &session);
  // #endregion attach

  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

static still_image_state drain_still_image_events(
  mln_runtime runtime, mln_map map
) {
  still_image_state state = STILL_IMAGE_PENDING;

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) {
    return STILL_IMAGE_FAILED;
  }

  for (size_t index = 0; index < batch.event_count; index++) {
    const char* bytes = (const char*)batch.events + index * batch.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;
    if (event->source != map) continue;
    if (event->type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED) {
      state = STILL_IMAGE_FINISHED;
    } else if (
      event->type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED ||
      event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED
    ) {
      return STILL_IMAGE_FAILED;
    }
  }

  return state;
}

static bool await_still_image(
  mln_runtime runtime, mln_map map, mln_render_session session
) {
  bool finished = false;
  bool rendered = false;
  const time_t deadline = time(NULL) + 30;

  // #region await
  while (!(finished && rendered) && time(NULL) < deadline) {
    mln_runtime_pump(runtime, 10, -1);
    const still_image_state state = drain_still_image_events(runtime, map);
    if (state == STILL_IMAGE_FAILED) return false;
    if (state == STILL_IMAGE_FINISHED) finished = true;
    mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
    bool needs_repaint = false;
    if (
      mln_render_session_render_update(session, &result, &needs_repaint) ==
      MLN_STATUS_OK
    ) {
      rendered = rendered || result == MLN_RENDER_RESULT_RENDERED;
    }
  }
  // #endregion await

  return finished && rendered;
}

static uint8_t* read_pixels(
  mln_render_session session, mln_texture_image_info* out_info
) {
  // #region read
  // A null buffer with a capacity of 0 is a size probe that fills info.
  mln_texture_image_info info = mln_texture_image_info_default();
  mln_texture_read_premultiplied_rgba8(session, NULL, 0, &info);

  uint8_t* pixels = malloc(info.byte_length);
  if (pixels == NULL) return NULL;

  const mln_status status = mln_texture_read_premultiplied_rgba8(
    session, pixels, info.byte_length, &info
  );
  // #endregion read

  if (status != MLN_STATUS_OK) {
    free(pixels);
    return NULL;
  }

  *out_info = info;
  return pixels;
}

// Returns premultiplied RGBA8 pixels that the host frees, and fills out_info
// with their width, height, and row stride.
uint8_t* render_still_image(
  mln_runtime runtime, const char* style_url, uint32_t width, uint32_t height,
  const mln_opengl_context_descriptor* context, mln_texture_image_info* out_info
) {
  // #region create
  mln_map_options options = mln_map_options_default();
  options.width = width;
  options.height = height;
  options.scale_factor = 1.0;
  options.map_mode = MLN_MAP_MODE_STATIC;

  mln_map map = MLN_HANDLE_NULL;
  if (mln_map_create(runtime, &options, &map) != MLN_STATUS_OK) return NULL;

  // A render session draws the map's latest update whatever the subscription
  // selects.
  mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED |
           MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED |
           MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED
  );
  // #endregion create

  uint8_t* pixels = NULL;
  const mln_render_session session =
    attach_owned_texture(map, context, width, height);

  if (
    session != MLN_HANDLE_NULL &&
    mln_map_set_style_url(map, style_url) == MLN_STATUS_OK &&
    mln_map_request_still_image(map) == MLN_STATUS_OK &&
    await_still_image(runtime, map, session)
  ) {
    pixels = read_pixels(session, out_info);
  }

  if (session != MLN_HANDLE_NULL) mln_render_session_destroy(session);
  mln_map_destroy(map);
  return pixels;
}
