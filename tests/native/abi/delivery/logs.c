// Queued log handlers: copied records, release on replace and clear, records
// that a released queue refuses, and dispatched records that may call the C
// API. Every case changes the process-global handler, so each runs inside
// restoring_log_state().

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

// What the queue ran: this case's dump records and every release, copied
// during each call. Dispatch runs on the case's thread.
typedef struct seen_calls {
  size_t count;
  bool unterminated;
  char kinds[max_seen];
  void* user_data[max_seen];
  mln_log_severity severities[max_seen];
  mln_log_event events[max_seen];
  int64_t codes[max_seen];
  char messages[max_seen][96];
} seen_calls;

static seen_calls seen;

static bool record_log(
  void* user_data, mln_log_severity severity, mln_log_event event, int64_t code,
  mln_buffer_view message
) {
  if (!mln_test_lent_text_is_terminated(message)) seen.unterminated = true;
  if (!is_dump_record(message.data) || seen.count == max_seen) return false;
  const size_t slot = seen.count++;
  seen.kinds[slot] = 'l';
  seen.user_data[slot] = user_data;
  seen.severities[slot] = severity;
  seen.events[slot] = event;
  seen.codes[slot] = code;
  strncpy(seen.messages[slot], message.data, sizeof(seen.messages[slot]) - 1);
  return false;
}

static void record_log_release(void* user_data) {
  if (seen.count == max_seen) return;
  const size_t slot = seen.count++;
  seen.kinds[slot] = 'r';
  seen.user_data[slot] = user_data;
}

static mln_log_handler queue_handler(mln_delivery_queue queue, void* tag) {
  return (mln_log_handler){
    .size = sizeof(mln_log_handler),
    .callback = record_log,
    .user_data = tag,
    .release_user_data = record_log_release,
    .queue = queue,
  };
}

static void dispatch_all(mln_delivery_queue queue) {
  memset(&seen, 0, sizeof(seen));
  mln_status status = MLN_STATUS_OK;
  while ((status = mln_delivery_queue_dispatch(queue, NULL)) == MLN_STATUS_OK) {
  }
  MLN_TEST_STATUS(MLN_STATUS_NOT_READY, status);
  TEST_ASSERT_FALSE(seen.unterminated);
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

  dispatch_all(queue.handle);
  TEST_ASSERT_EQUAL_size_t(4, seen.count);
  static const char* const expected[3] = {
    dump_separator, dump_style_url, dump_separator
  };
  for (size_t index = 0; index < 3; index += 1) {
    TEST_ASSERT_EQUAL_CHAR('l', seen.kinds[index]);
    TEST_ASSERT_EQUAL_PTR(&tag, seen.user_data[index]);
    TEST_ASSERT_EQUAL_UINT32(MLN_LOG_SEVERITY_INFO, seen.severities[index]);
    TEST_ASSERT_EQUAL_UINT32(MLN_LOG_EVENT_GENERAL, seen.events[index]);
    TEST_ASSERT_EQUAL_INT64(-1, seen.codes[index]);
    TEST_ASSERT_EQUAL_STRING(expected[index], seen.messages[index]);
  }
  // Clearing the handler releases it after its final record.
  TEST_ASSERT_EQUAL_CHAR('r', seen.kinds[3]);
  TEST_ASSERT_EQUAL_PTR(&tag, seen.user_data[3]);
  mln_test_delivery_queue_release(&queue);
}

// Replacing a queue handler releases it once, as the last call for its user
// data; clearing releases the replacement.
LOG_STATE_CASE(replace_and_clear_release_a_queue_handler_once) {
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

  dispatch_all(queue.handle);
  TEST_ASSERT_EQUAL_size_t(2, seen.count);
  TEST_ASSERT_EQUAL_CHAR('r', seen.kinds[0]);
  TEST_ASSERT_EQUAL_PTR(&first, seen.user_data[0]);
  TEST_ASSERT_EQUAL_CHAR('r', seen.kinds[1]);
  TEST_ASSERT_EQUAL_PTR(&second, seen.user_data[1]);
  mln_test_delivery_queue_release(&queue);
}

LOG_STATE_CASE(a_queue_handler_rejects_an_invalid_shape) {
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;

  // A queue does not stand in for the callback.
  mln_log_handler handler = queue_handler(queue.handle, &tag);
  handler.callback = NULL;
  MLN_TEST_INVALID(mln_log_set_callback(&handler, NULL));
  handler.release_user_data = NULL;
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
  RUN_TEST(replace_and_clear_release_a_queue_handler_once);
  RUN_TEST(a_queue_handler_rejects_an_invalid_shape);
  RUN_TEST(a_handler_on_a_released_queue_refuses_its_records);
  RUN_TEST(a_dispatched_handler_may_call_the_c_api);
}
