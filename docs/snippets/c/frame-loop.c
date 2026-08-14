// One turn of a display-paced host loop: drain scheduled notifications, service
// caller-driver work, and submit a frame demand when the map invalidates.

#include <maplibre_native_c.h>
#include <stdatomic.h>

typedef struct frame_receiver {
  mln_notification_source notifications;
  mln_runtime runtime;
  mln_map map;
  atomic_bool scheduled;
  void (*schedule)(void* user_data);
  void* schedule_user_data;
  uint64_t next_frame_token;
} frame_receiver;

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

static void drain_frame_results(mln_render_session session, bool* pending) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  if (
    mln_render_session_drain_frame_results(session, 0, &batch) != MLN_STATUS_OK
  )
    return;
  size_t count = 0;
  if (mln_render_frame_batch_count(batch, &count) == MLN_STATUS_OK) {
    for (size_t index = 0; index < count; index++) {
      mln_render_frame_result result = {
        .size = sizeof(mln_render_frame_result)
      };
      if (
        mln_render_frame_batch_get(batch, index, &result) == MLN_STATUS_OK &&
        result.disposition != MLN_RENDER_RESULT_RENDERED
      ) {
        *pending = true;
      }
    }
  }
  mln_render_frame_batch_release(batch);
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
            endpoint->id == receiver->runtime &&
            endpoint->kind == MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS
          ) {
            drain_runtime_events(receiver, pending);
          } else if (
            endpoint->id == session &&
            endpoint->kind == MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK
          ) {
            size_t serviced = 0;
            (void)mln_render_session_service_driver_work(
              session, 64, &serviced
            );
          } else if (
            endpoint->id == session &&
            endpoint->kind == MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES
          ) {
            drain_frame_results(session, pending);
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
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_PRESENT;
  demand.token = ++receiver->next_frame_token;
  if (mln_render_session_request_frame(session, &demand) != MLN_STATUS_OK) {
    *pending = true;
  }
  // #endregion render
}
