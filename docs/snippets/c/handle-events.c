// Draining the runtime's event queue: selecting the event types a host reads,
// matching an event to its source, and reading a typed payload.

#include <maplibre_native_c.h>

typedef struct map_observer {
  mln_map map;
  bool render_pending;
  bool style_ready;
  bool load_failed;
} map_observer;

// #region subscribe
mln_status select_map_events(mln_map map) {
  uint64_t command_id = 0;
  return mln_map_set_event_mask(
    map,
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED |
      MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED |
      MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
      MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED,
    &command_id
  );
}
// #endregion subscribe

static bool asks_for_a_repaint(const mln_runtime_event* event) {
  // #region payload
  if (event->payload_type != MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME) {
    return false;
  }
  return event->payload.render_frame.needs_repaint;
  // #endregion payload
}

void drain_events(mln_runtime runtime, map_observer* observer) {
  // #region drain
  mln_event_batch batch = MLN_HANDLE_NULL;
  if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) return;
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view)
  };
  if (mln_event_batch_get(batch, &view) != MLN_STATUS_OK) {
    mln_event_batch_release(batch);
    return;
  }

  for (size_t index = 0; index < view.event_count; index++) {
    const char* bytes = (const char*)view.events + index * view.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;

    // #region match
    // One runtime serves every map under it, and they share this queue.
    if (event->source_type != MLN_RUNTIME_EVENT_SOURCE_MAP) continue;
    if (event->source != observer->map) continue;
    // #endregion match

    switch (event->type) {
      case MLN_RUNTIME_EVENT_MAP_STYLE_LOADED:
        observer->style_ready = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE:
        observer->render_pending = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
        if (asks_for_a_repaint(event)) observer->render_pending = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
        observer->load_failed = true;
        break;
      default:
        break;
    }
  }
  mln_event_batch_release(batch);
  // #endregion drain
}
