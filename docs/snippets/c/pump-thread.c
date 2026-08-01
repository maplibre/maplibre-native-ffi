// A host-owned pump thread. The runtime and the map live on this thread, and
// the display thread owns the render session and calls render_update.

#include <maplibre_native_c.h>
#include <stdatomic.h>

// Bounds a park that nothing signals. A wake source normally releases the
// park first.
enum { park_timeout_ms = 100 };

typedef struct pump_channel {
  atomic_bool render_pending;
  atomic_bool stop_requested;
  // Acquired on the pump thread, signalled from any thread.
  mln_wake_source wake;
} pump_channel;

// The display thread calls this after it queues a camera command or sets
// stop_requested.
void pump_channel_wake(pump_channel* channel) {
  mln_wake_source_signal(channel->wake);
}

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

void pump_until_stopped(
  mln_runtime runtime, mln_map map, pump_channel* channel
) {
  channel->wake = MLN_HANDLE_NULL;
  if (
    mln_runtime_wake_source_acquire(runtime, &channel->wake) != MLN_STATUS_OK
  ) {
    return;
  }

  while (!atomic_load(&channel->stop_requested)) {
    mln_runtime_pump(runtime, park_timeout_ms);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (wants_a_frame(&event, map)) {
        atomic_store(&channel->render_pending, true);
      }
    }
  }

  // A wake source outlives its runtime, so destroy it once every thread that
  // signals it has finished.
  mln_wake_source_destroy(channel->wake);
}
