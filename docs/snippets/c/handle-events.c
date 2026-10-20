// Receiving runtime events: selecting the event types a host reads,
// matching an event to its source, and reading a typed payload.

#include <maplibre_native_c.h>
#include <stdatomic.h>

typedef struct map_observer {
  atomic_uint_fast64_t map;
  atomic_bool needs_repaint;
  atomic_bool style_ready;
  atomic_bool load_failed;
} map_observer;

// #region subscribe
mln_status select_map_events(mln_map map, const mln_completion* completion) {
  return mln_map_set_event_mask(
    map,
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED |
      MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED |
      MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED,
    completion, NULL
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

void receive_events(void* user_data, const mln_event_batch_view* events) {
  map_observer* observer = user_data;
  // #region drain
  const mln_event_batch_view view = *events;
  for (size_t index = 0; index < view.event_count; index++) {
    const char* bytes = (const char*)view.events + index * view.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;

    // #region match
    // One runtime serves every map under it, and they share this handler.
    if (event->source_type != MLN_RUNTIME_EVENT_SOURCE_MAP) continue;
    if (event->source != observer->map) continue;
    // #endregion match

    switch (event->type) {
      case MLN_RUNTIME_EVENT_MAP_STYLE_LOADED:
        observer->style_ready = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
        observer->needs_repaint = asks_for_a_repaint(event);
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

mln_status observe_events(
  mln_runtime runtime, map_observer* observer, const mln_completion* completion
) {
  const mln_runtime_event_handler handler = {
    .size = sizeof(handler),
    .callback = receive_events,
    .user_data = observer,
  };
  return mln_runtime_set_event_handler(runtime, &handler, completion, NULL);
}
