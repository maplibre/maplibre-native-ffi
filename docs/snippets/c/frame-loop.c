// One turn of a host frame loop: pump the runtime, drain its events, and render
// when the map has something new. Call it from whatever paces your frames.

#include <maplibre_native_c.h>

// MapLibre reports "I need another frame after this one" on the finished-frame
// event, which is how animations and label placement settle.
static bool asks_for_another_frame(const mln_runtime_event* event) {
  if (event->type != MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) return false;
  if (event->payload_type != MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME) {
    return false;
  }
  if (event->payload_size < sizeof(mln_runtime_event_render_frame)) {
    return false;
  }
  const mln_runtime_event_render_frame* frame = event->payload;
  return frame->needs_repaint;
}

void run_one_frame(
  mln_runtime* runtime, mln_map* map, mln_render_session* session, bool* pending
) {
  mln_runtime_run_once(runtime);

  mln_runtime_event event = {.size = sizeof(event)};
  bool has_event = false;
  while (mln_runtime_poll_event(runtime, &event, &has_event) == MLN_STATUS_OK &&
         has_event) {
    // One runtime can own several maps, and every map's events land in the same
    // queue. Route them by source before acting on them.
    if (event.source != map) continue;

    if (
      event.type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE ||
      asks_for_another_frame(&event)
    ) {
      *pending = true;
    }
  }

  if (!*pending) return;

  // A false result means no frame was produced, which is normal before the map
  // publishes its first update. Stay pending and try again next turn.
  bool rendered = false;
  const mln_status status =
    mln_render_session_render_update(session, &rendered);
  if (status == MLN_STATUS_OK && rendered) {
    *pending = false;
  }
}
