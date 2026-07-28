// Renders one still image with no window and reads the pixels back to the CPU:
// a static-mode map, a session-owned texture, and a loop that ends on a
// completion event instead of running forever.

#include <maplibre_native_c.h>
#include <stdlib.h>
#include <time.h>

// Returns malloc'd premultiplied RGBA8 pixels, or NULL. On success out_info
// describes the image layout.
uint8_t* render_still_image(
  mln_runtime* runtime, const char* style_url, uint32_t width, uint32_t height,
  const mln_opengl_context_descriptor* context, mln_texture_image_info* out_info
) {
  mln_map_options map_options = mln_map_options_default();
  map_options.width = width;
  map_options.height = height;
  map_options.scale_factor = 1.0;
  map_options.map_mode = MLN_MAP_MODE_STATIC;

  mln_map* map = NULL;
  if (mln_map_create(runtime, &map_options, &map) != MLN_STATUS_OK) return NULL;

  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context = *context;

  uint8_t* pixels = NULL;
  mln_render_session* session = NULL;
  const mln_status attached =
    mln_opengl_owned_texture_attach(map, &descriptor, &session);
  if (attached != MLN_STATUS_OK) {
    mln_map_destroy(map);
    return NULL;
  }

  if (mln_map_set_style_url(map, style_url) != MLN_STATUS_OK) goto done;
  if (mln_map_request_still_image(map) != MLN_STATUS_OK) goto done;

  // The still image is not one render call. Keep rendering the updates the map
  // publishes until it reports the image is finished.
  bool finished = false;
  bool failed = false;
  const time_t deadline = time(NULL) + 30;
  while (!finished && !failed && time(NULL) < deadline) {
    mln_runtime_run_once(runtime);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (event.source != map) continue;
      switch (event.type) {
        case MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE: {
          bool rendered = false;
          mln_render_session_render_update(session, &rendered);
          break;
        }
        case MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED:
          finished = true;
          break;
        case MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED:
        case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
          failed = true;
          break;
        default:
          break;
      }
    }
  }
  if (!finished) goto done;

  // Two-call sizing: ask for the layout first, then read into a buffer that
  // fits. The size query reports INVALID_ARGUMENT and still fills out_info.
  mln_texture_read_premultiplied_rgba8(session, NULL, 0, out_info);
  pixels = malloc(out_info->byte_length);
  if (pixels == NULL) goto done;

  const mln_status read = mln_texture_read_premultiplied_rgba8(
    session, pixels, out_info->byte_length, out_info
  );
  if (read != MLN_STATUS_OK) {
    free(pixels);
    pixels = NULL;
  }

done:
  mln_render_session_destroy(session);
  mln_map_destroy(map);
  return pixels;
}
