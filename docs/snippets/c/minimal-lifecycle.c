// Creates a runtime and map, then waits for the asynchronous style outcome.

#define _POSIX_C_SOURCE 199309L

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

typedef struct receiver {
  atomic_bool scheduled;
} receiver;

static void schedule_receiver(void* user_data) {
  receiver* value = user_data;
  atomic_store(&value->scheduled, true);
}

static mln_status wait_ok(mln_operation operation) {
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  if (status != MLN_STATUS_OK || !completed) return status;
  return mln_operation_get_status(operation, &status) == MLN_STATUS_OK
           ? status
           : MLN_STATUS_NATIVE_ERROR;
}

int main(void) {
  receiver receiver_state;
  atomic_init(&receiver_state.scheduled, false);

  mln_notification_source notifications = MLN_HANDLE_NULL;
  if (mln_notification_source_create(&notifications) != MLN_STATUS_OK) return 1;
  if (
    mln_notification_source_set_callback(
      notifications, schedule_receiver, &receiver_state
    ) != MLN_STATUS_OK
  )
    return 1;

  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.cache_path = ":memory:";
  runtime_options.notification_source = notifications;
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_status status = mln_runtime_create(&runtime_options, &runtime);
  if (status != MLN_STATUS_OK) return 1;

  mln_map_options map_options = mln_map_options_default();
  map_options.initial_extent =
    (mln_logical_extent){.width = 512, .height = 512, .scale_factor = 1.0};
  mln_operation operation = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  status = mln_map_create_start(runtime, &map_options, &operation);
  if (status == MLN_STATUS_OK) status = wait_ok(operation);
  if (status == MLN_STATUS_OK)
    status = mln_map_create_take_result(operation, &map);
  mln_operation_release(operation);
  if (status != MLN_STATUS_OK) {
    (void)mln_runtime_release(runtime);
    return 1;
  }

  uint64_t command_id = 0;
  mln_map_set_event_mask(
    map,
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED |
      MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED,
    &command_id
  );
  const mln_status requested = mln_map_set_style_url(
    map, "https://tiles.openfreemap.org/styles/bright", &command_id
  );
  bool loaded = false;
  bool settled = requested != MLN_STATUS_OK;

  const time_t deadline = time(NULL) + 30;
  while (!settled && time(NULL) < deadline) {
    if (!atomic_exchange(&receiver_state.scheduled, false)) {
      nanosleep(&(struct timespec){.tv_nsec = 10000000}, NULL);
      continue;
    }

    mln_ready_batch ready = MLN_HANDLE_NULL;
    if (
      mln_notification_source_drain_ready(notifications, &ready) !=
      MLN_STATUS_OK
    )
      continue;
    mln_ready_batch_view ready_view = {.size = sizeof(mln_ready_batch_view)};
    if (mln_ready_batch_get(ready, &ready_view) != MLN_STATUS_OK) {
      mln_ready_batch_release(ready);
      continue;
    }
    for (size_t endpoint_index = 0; endpoint_index < ready_view.endpoint_count;
         endpoint_index++) {
      const char* endpoint_bytes = (const char*)ready_view.endpoints +
                                   endpoint_index * ready_view.endpoint_size;
      const mln_ready_endpoint* endpoint =
        (const mln_ready_endpoint*)endpoint_bytes;
      if (
        endpoint->kind != MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS ||
        endpoint->id != runtime
      )
        continue;
      mln_event_batch batch = MLN_HANDLE_NULL;
      if (mln_runtime_drain_events(runtime, &batch) != MLN_STATUS_OK) continue;
      mln_runtime_event_batch_view view = {
        .size = sizeof(mln_runtime_event_batch_view)
      };
      if (mln_event_batch_get(batch, &view) == MLN_STATUS_OK) {
        for (size_t index = 0; index < view.event_count; index++) {
          const char* bytes =
            (const char*)view.events + index * view.event_size;
          const mln_runtime_event* event = (const mln_runtime_event*)bytes;
          if (event->source != map) continue;
          if (event->type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
            loaded = settled = true;
          } else if (event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
            if (event->message_size > 0) {
              fprintf(
                stderr, "style failed: %.*s\n", (int)event->message_size,
                view.messages + event->message_offset
              );
            }
            settled = true;
          }
        }
      }
      mln_event_batch_release(batch);
    }
    mln_ready_batch_release(ready);
  }

  (void)mln_map_release(map);
  (void)mln_runtime_release(runtime);
  mln_notification_source_release(notifications);
  return loaded ? 0 : 1;
}
