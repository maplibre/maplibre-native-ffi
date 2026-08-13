// A host-owned pump thread. The runtime and map use this owner thread. The
// display thread owns the render session and draws its updates.

#include <maplibre_native_c.h>
#include <stdatomic.h>

enum { park_timeout_ms = 100 };

// #region channel
typedef struct pump_channel {
  atomic_bool render_pending;
  atomic_bool stop_requested;
  mln_wake_source wake;
} pump_channel;
// #endregion channel

// #region signal
void pump_channel_wake(pump_channel* channel) {
  mln_wake_source_signal(channel->wake);
}
// #endregion signal

static bool wants_a_frame(const mln_runtime_event* event, mln_map map) {
  if (event->source != map) return false;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) return true;
  if (event->type != MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) return false;
  return event->payload.render_frame.needs_repaint;
}

static void drain_events(
  mln_runtime runtime, mln_map map, pump_channel* channel
) {
  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) return;

  for (size_t index = 0; index < batch.event_count; index++) {
    const char* bytes = (const char*)batch.events + index * batch.event_size;
    if (wants_a_frame((const mln_runtime_event*)bytes, map)) {
      atomic_store(&channel->render_pending, true);
    }
  }
}

void pump_until_stopped(
  mln_runtime runtime, mln_map map, pump_channel* channel
) {
  // #region acquire
  channel->wake = MLN_HANDLE_NULL;
  if (
    mln_runtime_wake_source_acquire(runtime, &channel->wake) != MLN_STATUS_OK
  ) {
    return;
  }

  // Only a selected event wakes this thread.
  mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
           MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED
  );
  // #endregion acquire

  // #region loop
  while (!atomic_load(&channel->stop_requested)) {
    mln_runtime_pump(runtime, park_timeout_ms);
    drain_events(runtime, map, channel);
  }
  // #endregion loop

  mln_wake_source_destroy(channel->wake);
}
