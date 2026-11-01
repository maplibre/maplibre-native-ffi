// Snippets for the docs site's Events and logs page. They compile with the
// native tests, which never run them.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static mln_map map = MLN_HANDLE_NULL;

static void on_style_loaded(void) {}

static void on_loading_failed(mln_buffer_view message) { (void)message; }

static void post_to_event_loop(void (*task)(void)) { (void)task; }

static void write_log(mln_log_severity severity, mln_buffer_view message) {
  (void)severity;
  (void)message;
}

// #region handler
static void on_events(void* user_data, const mln_event_batch_view* batch) {
  (void)user_data;
  for (size_t i = 0; i < batch->event_count; i++) {
    // Step by event_size, not by sizeof(mln_runtime_event).
    const mln_runtime_event* event =
      MLN_STRIDED_AT(mln_runtime_event, batch->events, batch->event_size, i);
    if (event->source != map) continue;

    switch (event->type) {
      case MLN_RUNTIME_EVENT_MAP_STYLE_LOADED:
        on_style_loaded();
        break;
      case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
        // The message is valid only during this callback.
        on_loading_failed(event->message);
        break;
      default:
        break;
    }
  }
}
// #endregion handler

void mln_doc_events_handler(mln_runtime runtime);

void mln_doc_events_handler(mln_runtime runtime) {
  // #region handler
  mln_runtime_event_handler handler = {
    .size = sizeof(mln_runtime_event_handler),
    .callback = on_events,
  };
  mln_runtime_set_event_handler(runtime, &handler, NULL, NULL);
  // #endregion handler

  // #region mask
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .fields = MLN_MAP_OPTION_EVENT_MASK,
    .initial_extent = {.width = 512, .height = 512, .scale_factor = 1.0},
    .event_mask = MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED |
                  MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED,
  };
  mln_runtime_create_map(runtime, &map_options, &map, NULL, NULL);
  // #endregion mask
}

// #region queue
static mln_delivery_queue queue = MLN_HANDLE_NULL;

// Runs on your event loop.
static void dispatch_queue(void) { mln_delivery_queue_dispatch(queue, NULL); }

// Runs on a library thread. It posts a task and returns.
static void on_wake(void* user_data) {
  (void)user_data;
  post_to_event_loop(dispatch_queue);
}
// #endregion queue

void mln_doc_events_queue(mln_runtime runtime);

void mln_doc_events_queue(mln_runtime runtime) {
  // #region queue
  mln_delivery_queue_options queue_options = {
    .size = sizeof(mln_delivery_queue_options),
    .wake = {.callback = on_wake},
  };
  mln_delivery_queue_create(&queue_options, &queue, NULL);

  mln_runtime_event_handler handler = {
    .size = sizeof(mln_runtime_event_handler),
    .callback = on_events,
    .queue = queue,
  };
  mln_runtime_set_event_handler(runtime, &handler, NULL, NULL);
  // #endregion queue
}

// #region logs
static bool on_log(
  void* user_data, mln_log_severity severity, mln_log_event event, int64_t code,
  mln_buffer_view message
) {
  (void)user_data;
  (void)event;
  (void)code;
  // The message is valid only during this callback.
  write_log(severity, message);
  // Consume the record. Return false to let the platform logger print it too.
  return true;
}
// #endregion logs

void mln_doc_events_logs(void);

void mln_doc_events_logs(void) {
  // #region logs
  mln_log_handler log_handler = {
    .size = sizeof(mln_log_handler),
    .callback = on_log,
  };
  mln_log_set_callback(&log_handler, NULL);
  // #endregion logs
}
