// Text that native lends to the host is NUL-terminated after its bytes, and
// its data is never null, even when the text is empty. These cases read
// completion values and diagnostics in both modes, runtime event messages in
// callback mode, and log callback messages. The suite's own helpers check the
// same for queued events, query results, layer and source info, and provider
// requests, and the transform and delivery cases check their URLs and
// messages.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

// What one completion lent, read during its callback.
typedef struct lent_completion {
  atomic_bool done;
  mln_status status;
  size_t diagnostic_size;
  bool diagnostic_terminated;
  size_t value_count;
  size_t value_size;
  bool value_terminated;
} lent_completion;

static void read_lent_completion(
  lent_completion* out, const mln_completion_result* result
) {
  out->status = result->status;
  out->diagnostic_size = result->diagnostic.size;
  out->diagnostic_terminated =
    mln_test_lent_text_is_terminated(result->diagnostic);
  out->value_count = result->value_count;
  if (result->value_count == 1) {
    const mln_buffer_view* value = result->value;
    out->value_size = value->size;
    out->value_terminated = mln_test_lent_text_is_terminated(*value);
  }
}

static void record_lent_completion(
  void* user_data, const mln_completion_result* result
) {
  lent_completion* lent = user_data;
  read_lent_completion(lent, result);
  mln_test_flag_set(&lent->done);
}

static void expect_text_value(const lent_completion* lent, bool empty) {
  MLN_TEST_OK(lent->status);
  TEST_ASSERT_EQUAL_size_t(0, lent->diagnostic_size);
  TEST_ASSERT_TRUE(lent->diagnostic_terminated);
  TEST_ASSERT_EQUAL_size_t(1, lent->value_count);
  TEST_ASSERT_EQUAL(empty, lent->value_size == 0);
  TEST_ASSERT_TRUE(lent->value_terminated);
}

static void expect_failure_diagnostic(const lent_completion* lent) {
  MLN_TEST_STATUS(MLN_STATUS_NOT_FOUND, lent->status);
  TEST_ASSERT_GREATER_THAN_size_t(0, lent->diagnostic_size);
  TEST_ASSERT_TRUE(lent->diagnostic_terminated);
}

typedef mln_status (*map_query)(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
);

static mln_status remove_absent_layer(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("absent-layer"), completion, diagnostic
  );
}

static lent_completion call_with_callback(mln_map map, map_query query) {
  lent_completion lent = {0};
  atomic_init(&lent.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = record_lent_completion,
    .user_data = &lent,
  };
  MLN_TEST_OK(query(map, &completion, MLN_TEST_DIAGNOSTIC));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&lent.done));
  return lent;
}

typedef struct lent_wait {
  mln_delivery_queue queue;
  lent_completion* lent;
  mln_status failure;
} lent_wait;

static bool dispatched_lent(void* context) {
  lent_wait* wait = context;
  const mln_status status = mln_delivery_queue_dispatch(wait->queue, NULL);
  if (status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY) {
    wait->failure = status;
    return true;
  }
  return atomic_load(&wait->lent->done);
}

// The same, with the completion copied into queue and read in its dispatch.
static lent_completion call_with_queue(
  mln_map map, mln_delivery_queue queue, map_query query
) {
  lent_completion lent = {0};
  atomic_init(&lent.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = record_lent_completion,
    .user_data = &lent,
    .queue = queue,
  };
  MLN_TEST_OK(query(map, &completion, MLN_TEST_DIAGNOSTIC));
  lent_wait wait = {.queue = queue, .lent = &lent};
  TEST_ASSERT_TRUE(mln_test_await(
    dispatched_lent, &wait, mln_test_deadline_default(), "a completion"
  ));
  MLN_TEST_OK(wait.failure);
  return lent;
}

// A style set from JSON has an empty URL, so the URL query lends empty text
// and the JSON query lends the style. A success lends an empty diagnostic and
// a failure lends its message.
static void completions_lend_terminated_text(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);

  lent_completion lent = call_with_callback(map, mln_map_get_style_url);
  expect_text_value(&lent, true);
  lent = call_with_callback(map, mln_map_get_loaded_style_json);
  expect_text_value(&lent, false);
  lent = call_with_callback(map, remove_absent_layer);
  expect_failure_diagnostic(&lent);

  lent = call_with_queue(map, queue.handle, mln_map_get_style_url);
  expect_text_value(&lent, true);
  lent = call_with_queue(map, queue.handle, mln_map_get_loaded_style_json);
  expect_text_value(&lent, false);
  lent = call_with_queue(map, queue.handle, remove_absent_layer);
  expect_failure_diagnostic(&lent);

  mln_test_destroy_map(map);
  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

// What a callback-mode event handler saw. It runs on runtime threads.
typedef struct lent_events {
  atomic_int events;
  atomic_int unterminated;
  atomic_bool failed;
  atomic_size_t failure_message_size;
} lent_events;

static void read_lent_events(
  void* user_data, const mln_event_batch_view* events
) {
  lent_events* lent = user_data;
  for (size_t index = 0; index < events->event_count; index += 1) {
    const mln_runtime_event* event = MLN_STRIDED_AT(
      mln_runtime_event, events->events, events->event_size, index
    );
    atomic_fetch_add(&lent->events, 1);
    if (!mln_test_lent_text_is_terminated(event->message)) {
      atomic_fetch_add(&lent->unterminated, 1);
    }
    if (event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
      atomic_store(&lent->failure_message_size, event->message.size);
      mln_test_flag_set(&lent->failed);
    }
  }
}

// A callback handler borrows each event's message, empty or not, terminated.
// The provider fails the style, whose failure event carries a message; the
// events before it carry none.
static void callback_events_lend_terminated_messages(void) {
  mln_test_provider* provider = mln_test_provider_create(NULL, 0);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  static lent_events lent;
  atomic_store(&lent.events, 0);
  atomic_store(&lent.unterminated, 0);
  atomic_store(&lent.failed, false);
  atomic_store(&lent.failure_message_size, 0);
  const mln_runtime_event_handler handler = {
    .size = sizeof(handler),
    .callback = read_lent_events,
    .user_data = &lent,
  };
  MLN_TEST_AWAIT_OK(mln_runtime_set_event_handler(
    runtime, &handler, &completion.descriptor, NULL
  ));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://missing-style.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&lent.failed));
  MLN_TEST_AWAIT_OK(
    mln_runtime_clear_event_handler(runtime, &completion.descriptor, NULL)
  );

  TEST_ASSERT_GREATER_THAN_INT(1, atomic_load(&lent.events));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&lent.unterminated));
  TEST_ASSERT_GREATER_THAN_size_t(0, atomic_load(&lent.failure_message_size));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

// What a log callback saw. It runs on whichever thread logs.
typedef struct lent_logs {
  atomic_int records;
  atomic_int unterminated;
} lent_logs;

static bool read_lent_log(
  void* user_data, mln_log_severity severity, mln_log_event event, int64_t code,
  mln_buffer_view message
) {
  (void)severity;
  (void)event;
  (void)code;
  lent_logs* lent = user_data;
  atomic_fetch_add(&lent->records, 1);
  if (!mln_test_lent_text_is_terminated(message)) {
    atomic_fetch_add(&lent->unterminated, 1);
  }
  return true;
}

static void log_callback_body(void) {
  static lent_logs lent;
  atomic_store(&lent.records, 0);
  atomic_store(&lent.unterminated, 0);
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, NULL));
  const mln_log_handler handler = {
    .size = sizeof(handler),
    .callback = read_lent_log,
    .user_data = &lent,
  };
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  MLN_TEST_AWAIT_OK(mln_map_dump_debug_logs(map, &completion.descriptor, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  MLN_TEST_OK(mln_log_clear_callback(NULL));

  TEST_ASSERT_GREATER_THAN_INT(0, atomic_load(&lent.records));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&lent.unterminated));
}

// The log callback and its async mask belong to the process, so the case
// restores both.
static void log_callbacks_lend_terminated_messages(void) {
  if (TEST_PROTECT()) {
    log_callback_body();
  }
  (void)mln_log_clear_callback(NULL);
  (void)mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_DEFAULT, NULL);
}

MLN_TEST_GROUP {
  RUN_TEST(completions_lend_terminated_text);
  RUN_TEST(callback_events_lend_terminated_messages);
  RUN_TEST(log_callbacks_lend_terminated_messages);
}
