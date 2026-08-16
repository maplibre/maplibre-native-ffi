// One turn of a display-paced host loop: pump the runtime, drain its events,
// and draw a published render update. Call it from the frame callback.

#include <maplibre_native_c.h>

// Call this before the map loads a style. Narrowing a subscription keeps the
// events already queued.
mln_status select_frame_events(mln_map map) {
  return mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
           MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED
  );
}

static bool wants_a_frame(const mln_runtime_event* event, mln_map map) {
  if (event->source != map) return false;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) return true;
  if (event->type != MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) return false;
  return event->payload.render_frame.needs_repaint;
}

void run_one_frame(
  mln_runtime runtime, mln_map map, mln_render_session session, bool* pending
) {
  // #region pump
  mln_runtime_pump(runtime, 0, 8);

  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) return;

  for (size_t index = 0; index < batch.event_count; index++) {
    const char* bytes = (const char*)batch.events + index * batch.event_size;
    if (wants_a_frame((const mln_runtime_event*)bytes, map)) *pending = true;
  }
  // #endregion pump

  // #region render
  if (!*pending) return;

  mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
  bool needs_repaint = false;
  const mln_status status =
    mln_render_session_render_update(session, &result, &needs_repaint);
  // A rendered frame clears the request unless the map asked for another
  // frame while rendering it. Any other result keeps the frame pending for
  // the next turn.
  if (status == MLN_STATUS_OK && result == MLN_RENDER_RESULT_RENDERED) {
    *pending = needs_repaint;
  }
  // #endregion render
}
