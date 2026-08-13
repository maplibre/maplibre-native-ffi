// One turn of a display-paced host loop: drain scheduled notifications and draw
// a published render update. Call it from the frame callback.

#include <maplibre_native_c.h>
#include <stdatomic.h>

typedef struct frame_receiver {
  mln_notification_source notifications;
  mln_runtime runtime;
  mln_map map;
  atomic_bool scheduled;
  void (*schedule)(void* user_data);
  void* schedule_user_data;
} frame_receiver;

// This callback may run on any native thread. It only schedules the receiver.
void schedule_frame_receiver(void* user_data) {
  frame_receiver* receiver = user_data;
  atomic_store_explicit(&receiver->scheduled, true, memory_order_release);
  receiver->schedule(receiver->schedule_user_data);
}

mln_status select_frame_events(mln_map map) {
  uint64_t command_id = 0;
  return mln_map_set_event_mask(
    map,
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
      MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED,
    &command_id
  );
}

static bool wants_a_frame(const mln_runtime_event* event, mln_map map) {
  if (event->source != map) return false;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) return true;
  if (event->type != MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) return false;
  return event->payload.render_frame.needs_repaint;
}

static void drain_runtime_events(frame_receiver* receiver, bool* pending) {
  mln_event_batch batch = MLN_HANDLE_NULL;
  if (mln_runtime_drain_events(receiver->runtime, 0, &batch) != MLN_STATUS_OK)
    return;
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view)
  };
  if (mln_event_batch_get(batch, &view) == MLN_STATUS_OK) {
    for (size_t index = 0; index < view.event_count; index++) {
      const char* bytes = (const char*)view.events + index * view.event_size;
      if (wants_a_frame((const mln_runtime_event*)bytes, receiver->map)) {
        *pending = true;
      }
    }
  }
  mln_event_batch_release(batch);
}

void run_one_frame(
  frame_receiver* receiver, mln_render_session session, bool* pending
) {
  // #region notifications
  if (
    atomic_exchange_explicit(&receiver->scheduled, false, memory_order_acq_rel)
  ) {
    mln_ready_batch ready = MLN_HANDLE_NULL;
    if (
      mln_notification_source_drain_ready(receiver->notifications, &ready) ==
      MLN_STATUS_OK
    ) {
      mln_ready_batch_view view = {.size = sizeof(mln_ready_batch_view)};
      if (mln_ready_batch_get(ready, &view) == MLN_STATUS_OK) {
        for (size_t index = 0; index < view.endpoint_count; index++) {
          const char* bytes =
            (const char*)view.endpoints + index * view.endpoint_size;
          const mln_ready_endpoint* endpoint = (const mln_ready_endpoint*)bytes;
          if (
            endpoint->kind == MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS &&
            endpoint->id == receiver->runtime
          ) {
            drain_runtime_events(receiver, pending);
          }
        }
      }
      mln_ready_batch_release(ready);
    }
  }
  // #endregion notifications

  // #region render
  if (!*pending) return;
  *pending = false;
  mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
  const mln_status status = mln_render_session_render_update(session, &result);
  if (status != MLN_STATUS_OK || result != MLN_RENDER_RESULT_RENDERED) {
    *pending = true;
  }
  // #endregion render
}
