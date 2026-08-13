// Creates a runtime and a map, loads a style, pumps until the style resolves,
// and tears everything down. One thread does all of it: the thread that creates
// the runtime owns the runtime and every map under it.

#include <maplibre_native_c.h>
#include <stdio.h>
#include <time.h>

int main(void) {
  mln_notification_source notifications = MLN_HANDLE_NULL;
  if (mln_notification_source_create(&notifications) != MLN_STATUS_OK) return 1;

  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.cache_path = ":memory:";
  runtime_options.notification_source = notifications;

  mln_runtime runtime = MLN_HANDLE_NULL;
  if (mln_runtime_create(&runtime_options, &runtime) != MLN_STATUS_OK) {
    mln_notification_source_close(notifications);
    return 1;
  }

  mln_map_options map_options = mln_map_options_default();
  map_options.width = 512;
  map_options.height = 512;
  map_options.scale_factor = 1.0;

  mln_map map = MLN_HANDLE_NULL;
  if (mln_map_create(runtime, &map_options, &map) != MLN_STATUS_OK) {
    mln_runtime_destroy(runtime);
    mln_notification_source_close(notifications);
    return 1;
  }

  // A map queues an event only while its subscription selects the type.
  mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED |
           MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED
  );

  // This call only accepts the request; the outcome arrives later as an event.
  const mln_status requested =
    mln_map_set_style_url(map, "https://tiles.openfreemap.org/styles/bright");
  bool loaded = false;
  bool settled = requested != MLN_STATUS_OK;

  const time_t deadline = time(NULL) + 30;
  while (!settled && time(NULL) < deadline) {
    // A positive timeout parks the thread until work arrives or it expires.
    mln_runtime_pump(runtime, 100);

    mln_event_batch batch = MLN_HANDLE_NULL;
    if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) continue;
    mln_runtime_event_batch_view view = {0};
    if (mln_event_batch_get(batch, &view) != MLN_STATUS_OK) {
      mln_event_batch_release(batch);
      continue;
    }

    for (size_t index = 0; index < view.event_count; index++) {
      const char* bytes = (const char*)view.events + index * view.event_size;
      const mln_runtime_event* event = (const mln_runtime_event*)bytes;
      if (event->source != map) continue;
      if (event->type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
        loaded = true;
        settled = true;
      } else if (event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
        // A message lives in the batch's arena, at the event's own offset.
        if (event->message_size > 0) {
          fprintf(
            stderr, "style failed: %.*s\n", (int)event->message_size,
            view.messages + event->message_offset
          );
        }
        settled = true;
      }
    }
    mln_event_batch_release(batch);
  }

  // mln_runtime_destroy returns MLN_STATUS_INVALID_STATE while a map is live.
  mln_map_destroy(map);
  mln_runtime_destroy(runtime);
  mln_notification_source_close(notifications);
  return loaded ? 0 : 1;
}
