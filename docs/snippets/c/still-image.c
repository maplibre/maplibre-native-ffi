// Requests one still image and reads the pixels back to the CPU. The map is in
// static or tile mode, and the session owns its texture.

#include <maplibre_native_c.h>
#include <stdlib.h>

void render_still_image(
  mln_runtime* runtime, mln_map* map, mln_render_session* session
) {
  if (mln_map_request_still_image(map) != MLN_STATUS_OK) return;

  // The request takes several render calls. Render each update the map
  // publishes until it reports the image finished.
  bool finished = false;
  while (!finished) {
    mln_runtime_pump(runtime, 100);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (event.source != map) continue;
      if (event.type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) {
        bool rendered = false;
        mln_render_session_render_update(session, &rendered);
      } else if (event.type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED) {
        finished = true;
      } else if (
        event.type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED ||
        event.type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED
      ) {
        return;
      }
    }
  }

  // Two-call sizing. The default constructor fills the size field that readback
  // checks before it writes anything else.
  mln_texture_image_info info = mln_texture_image_info_default();
  mln_texture_read_premultiplied_rgba8(session, NULL, 0, &info);

  uint8_t* pixels = malloc(info.byte_length);
  if (pixels == NULL) return;
  if (
    mln_texture_read_premultiplied_rgba8(
      session, pixels, info.byte_length, &info
    ) == MLN_STATUS_OK
  ) {
    // info.width by info.height premultiplied RGBA8, info.stride per row.
  }
  free(pixels);
}
