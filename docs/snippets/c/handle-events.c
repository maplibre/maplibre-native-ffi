// Draining the runtime's event queue: matching an event to its source and
// reading a typed payload safely.

#include <maplibre_native_c.h>

typedef struct map_observer {
  mln_map map;
  bool render_pending;
  bool style_ready;
  bool load_failed;
} map_observer;

static bool asks_for_a_repaint(const mln_runtime_event* event) {
  // #region payload
  // Check the payload's type and size before reading it. A runtime built from
  // a newer header can carry a longer payload than this host was compiled for.
  if (event->payload_type != MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME) {
    return false;
  }
  if (event->payload_size < sizeof(mln_runtime_event_render_frame)) {
    return false;
  }

  const mln_runtime_event_render_frame* frame = event->payload;
  return frame->needs_repaint;
  // #endregion payload
}

void drain_events(mln_runtime runtime, map_observer* observer) {
  // #region drain
  mln_runtime_event event = {.size = sizeof(event)};
  bool has_event = false;

  while (mln_runtime_poll_event(runtime, &event, &has_event) == MLN_STATUS_OK &&
         has_event) {
    // #region match
    // One runtime serves every map under it, and they share this queue.
    if (event.source_type != MLN_RUNTIME_EVENT_SOURCE_MAP) continue;
    if (event.source != observer->map) continue;
    // #endregion match

    switch (event.type) {
      case MLN_RUNTIME_EVENT_MAP_STYLE_LOADED:
        observer->style_ready = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE:
        observer->render_pending = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
        if (asks_for_a_repaint(&event)) observer->render_pending = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
        observer->load_failed = true;
        break;
      default:
        break;
    }
  }
  // #endregion drain
}
