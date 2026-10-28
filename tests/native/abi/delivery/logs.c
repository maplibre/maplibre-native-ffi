// Queue-mode log handlers: copied records, retirement on replace and clear,
// records that a released queue refuses, and dispatched records that may call
// the C API. Every case changes the
// process-global handler, so each runs inside restoring_log_state().

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

static void restore_log_defaults(void) {
  (void)mln_log_clear_callback(NULL);
  (void)mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_DEFAULT, NULL);
}

static void restoring_log_state(void (*body)(void)) {
  if (TEST_PROTECT()) {
    body();
  }
  restore_log_defaults();
}

#define LOG_STATE_CASE(name)                                   \
  static void name##_body(void);                               \
  static void name(void) { restoring_log_state(name##_body); } \
  static void name##_body(void)

static const char dump_separator[] =
  "----------------------------------------------------------------------"
  "----------";
static const char dump_style_url[] = "styleURL: ";

static bool is_dump_record(const char* message) {
  return strcmp(message, dump_separator) == 0 ||
         strncmp(message, dump_style_url, sizeof(dump_style_url) - 1) == 0;
}

static mln_log_handler queue_handler(mln_delivery_queue queue, void* tag) {
  return (mln_log_handler){
    .size = sizeof(mln_log_handler),
    .user_data = tag,
    .queue = queue,
  };
}

// Writes the three dump records of a map with an empty style, synchronously
// when the async mask leaves info records synchronous.
static void dump_debug_logs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  MLN_TEST_AWAIT_OK(mln_map_dump_debug_logs(map, &completion.descriptor, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum { max_seen = 64 };

// What the queue delivered, copied before each batch is released.
typedef struct seen_deliveries {
  size_t count;
  uint32_t types[max_seen];
  void* user_data[max_seen];
  mln_log_delivery records[max_seen];
  char messages[max_seen][96];
} seen_deliveries;

static void drain_all(mln_delivery_queue queue, seen_deliveries* seen) {
  while (true) {
    mln_test_delivery_drain drain;
    const mln_status status = mln_test_delivery_drain_into(queue, &drain);
    if (status == MLN_STATUS_NOT_READY) return;
    MLN_TEST_OK(status);
    for (size_t index = 0; index < drain.view.delivery_count; index += 1) {
      const mln_delivery* delivery = mln_test_delivery_at(&drain.view, index);
      // Keep only this case's dump records and every retirement.
      if (delivery->type == MLN_DELIVERY_TYPE_LOG_RECORD) {
        const mln_buffer_view message = delivery->payload.log_record.message;
        TEST_ASSERT_TRUE(mln_test_lent_text_is_terminated(message));
        if (!is_dump_record(message.data)) continue;
      }
      TEST_ASSERT_TRUE(seen->count < max_seen);
      const size_t slot = seen->count++;
      seen->types[slot] = delivery->type;
      seen->user_data[slot] = delivery->user_data;
      if (delivery->type == MLN_DELIVERY_TYPE_LOG_RECORD) {
        seen->records[slot] = delivery->payload.log_record;
        strncpy(
          seen->messages[slot], delivery->payload.log_record.message.data,
          sizeof(seen->messages[slot]) - 1
        );
      }
    }
    mln_delivery_batch_release(drain.batch);
  }
}

LOG_STATE_CASE(a_queue_handler_copies_each_record) {
  MLN_TEST_OK(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_ERROR, NULL)
  );
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  const mln_log_handler handler = queue_handler(queue.handle, &tag);
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));

  dump_debug_logs();
  // The queue wakes inline on the logging thread, before the dump command that
  // logged the records completes.
  TEST_ASSERT_GREATER_THAN_INT(0, atomic_load(&queue.wakes));
  MLN_TEST_OK(mln_log_clear_callback(NULL));
  // A cleared handler queues nothing more.
  dump_debug_logs();

  static seen_deliveries seen;
  memset(&seen, 0, sizeof(seen));
  drain_all(queue.handle, &seen);
  TEST_ASSERT_EQUAL_size_t(4, seen.count);
  static const char* const expected[3] = {
    dump_separator, dump_style_url, dump_separator
  };
  for (size_t index = 0; index < 3; index += 1) {
    TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_LOG_RECORD, seen.types[index]);
    TEST_ASSERT_EQUAL_PTR(&tag, seen.user_data[index]);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_LOG_SEVERITY_INFO, seen.records[index].severity
    );
    TEST_ASSERT_EQUAL_UINT32(MLN_LOG_EVENT_GENERAL, seen.records[index].event);
    TEST_ASSERT_EQUAL_INT64(-1, seen.records[index].code);
    TEST_ASSERT_EQUAL_STRING(expected[index], seen.messages[index]);
  }
  // Clearing the handler retires it after its final record.
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RETIRED, seen.types[3]);
  TEST_ASSERT_EQUAL_PTR(&tag, seen.user_data[3]);
  mln_test_delivery_queue_release(&queue);
}

// Replacing a queue handler retires it once, as the last delivery for its
// user data; clearing retires the replacement.
LOG_STATE_CASE(replace_and_clear_retire_a_queue_handler_once) {
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int first = 0;
  int second = 0;
  const mln_log_handler first_handler = queue_handler(queue.handle, &first);
  MLN_TEST_OK(mln_log_set_callback(&first_handler, NULL));
  const mln_log_handler second_handler = queue_handler(queue.handle, &second);
  MLN_TEST_OK(mln_log_set_callback(&second_handler, NULL));
  MLN_TEST_OK(mln_log_clear_callback(NULL));
  MLN_TEST_OK(mln_log_clear_callback(NULL));

  static seen_deliveries seen;
  memset(&seen, 0, sizeof(seen));
  drain_all(queue.handle, &seen);
  TEST_ASSERT_EQUAL_size_t(2, seen.count);
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RETIRED, seen.types[0]);
  TEST_ASSERT_EQUAL_PTR(&first, seen.user_data[0]);
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RETIRED, seen.types[1]);
  TEST_ASSERT_EQUAL_PTR(&second, seen.user_data[1]);
  mln_test_delivery_queue_release(&queue);
}

LOG_STATE_CASE(a_queue_handler_rejects_an_invalid_shape) {
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;

  mln_log_handler handler = queue_handler(MLN_HANDLE_NULL, &tag);
  MLN_TEST_INVALID(mln_log_set_callback(&handler, NULL));
  handler = queue_handler(queue.handle, &tag);
  handler.release_user_data = mln_test_delivery_count_wake_release;
  MLN_TEST_INVALID(mln_log_set_callback(&handler, NULL));

  mln_test_delivery_queue_release(&queue);
  handler = queue_handler(queue.handle, &tag);
  MLN_TEST_INVALID_STATE(mln_log_set_callback(&handler, NULL));
}

// A handler whose queue is released stays installed and refuses each record,
// which neither fails the command that logged it nor keeps the handler from
// clearing.
LOG_STATE_CASE(a_handler_on_a_released_queue_refuses_its_records) {
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, NULL));
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  const mln_log_handler handler = queue_handler(queue.handle, &tag);
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));
  mln_test_delivery_queue_release(&queue);

  dump_debug_logs();
  MLN_TEST_OK(mln_log_clear_callback(NULL));
}

// What a dispatched log handler saw. It runs on the dispatching thread, so
// plain fields suffice; a record that reaches another thread counts there.
typedef struct dispatched_log {
  atomic_int off_thread;
  size_t records;
  size_t refused_calls;
  size_t releases;
  size_t records_at_release;
} dispatched_log;

static MLN_TEST_THREAD_LOCAL bool dispatching_logs;

// Calls a status-returning function, which a log callback on a native thread
// may not, and asks for the platform logger, which a queued handler ignores.
static bool record_dispatched_log(
  void* user_data, mln_log_severity severity, mln_log_event event, int64_t code,
  mln_buffer_view message
) {
  (void)severity;
  (void)event;
  (void)code;
  dispatched_log* log = user_data;
  if (!dispatching_logs) {
    atomic_fetch_add(&log->off_thread, 1);
    return false;
  }
  if (!is_dump_record(message.data)) return false;
  log->records += 1;
  if (
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_ERROR, NULL) !=
    MLN_STATUS_OK
  ) {
    log->refused_calls += 1;
  }
  return false;
}

static void release_dispatched_log(void* user_data) {
  dispatched_log* log = user_data;
  if (!dispatching_logs) {
    atomic_fetch_add(&log->off_thread, 1);
    return;
  }
  log->releases += 1;
  log->records_at_release = log->records;
}

LOG_STATE_CASE(a_dispatched_handler_may_call_the_c_api) {
  MLN_TEST_OK(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_ERROR, NULL)
  );
  dispatching_logs = true;
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  static dispatched_log log;
  memset(&log, 0, sizeof(log));
  mln_log_handler handler = queue_handler(queue.handle, &log);
  handler.callback = record_dispatched_log;
  handler.release_user_data = release_dispatched_log;
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));

  dump_debug_logs();
  MLN_TEST_OK(mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_size_t(0, log.records);
  TEST_ASSERT_EQUAL_size_t(0, log.releases);
  // Clearing queued the release after the handler's last record.
  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_EQUAL_size_t(3, log.records);
  TEST_ASSERT_EQUAL_size_t(0, log.refused_calls);
  TEST_ASSERT_EQUAL_size_t(1, log.releases);
  TEST_ASSERT_EQUAL_size_t(3, log.records_at_release);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&log.off_thread));
  mln_test_delivery_queue_release(&queue);
}

MLN_TEST_GROUP {
  RUN_TEST(a_queue_handler_copies_each_record);
  RUN_TEST(replace_and_clear_retire_a_queue_handler_once);
  RUN_TEST(a_queue_handler_rejects_an_invalid_shape);
  RUN_TEST(a_handler_on_a_released_queue_refuses_its_records);
  RUN_TEST(a_dispatched_handler_may_call_the_c_api);
}
