// One turn of a display-paced host loop: pump the runtime, drain its events,
// and render when the map has something new. Call it from your frame callback.

#include <maplibre_native_c.h>

static bool wants_a_frame(const mln_runtime_event* event, mln_map map) {
  if (event->source != map) return false;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) return true;
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
  mln_runtime runtime, mln_map map, mln_render_session session, bool* pending
) {
  // #region pump
  mln_runtime_pump(runtime, 0);

  mln_runtime_event event = {.size = sizeof(event)};
  bool has_event = false;
  while (mln_runtime_poll_event(runtime, &event, &has_event) == MLN_STATUS_OK &&
         has_event) {
    if (wants_a_frame(&event, map)) *pending = true;
  }
  // #endregion pump

  // #region render
  if (!*pending) return;

  bool rendered = false;
  const mln_status status =
    mln_render_session_render_update(session, &rendered);
  if (status == MLN_STATUS_OK && rendered) {
    *pending = false;
  }
  // #endregion render
}
