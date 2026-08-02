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
  if (event->payload_type != MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME) {
    return false;
  }
  if (event->payload_size < sizeof(mln_runtime_event_render_frame)) {
    return false;
  }
  const mln_runtime_event_render_frame* frame = event->payload;
  return frame->needs_repaint;
}

static void drain_events(
  mln_runtime runtime, mln_map map, pump_channel* channel
) {
  mln_runtime_event event = {.size = sizeof(event)};
  bool has_event = false;
  while (mln_runtime_poll_event(runtime, &event, &has_event) == MLN_STATUS_OK &&
         has_event) {
    if (wants_a_frame(&event, map)) {
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
  // #endregion acquire

  // #region loop
  while (!atomic_load(&channel->stop_requested)) {
    mln_runtime_pump(runtime, park_timeout_ms);
    drain_events(runtime, map, channel);
  }
  // #endregion loop

  mln_wake_source_destroy(channel->wake);
}
