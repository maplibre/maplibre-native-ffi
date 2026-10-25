// Queue-mode log handlers: copied records, retirement on replace and clear,
// and records that a released queue refuses. Every case changes the
// process-global handler, so each runs inside restoring_log_state().

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// MapLibre's platform logger writes to stderr on desktop Linux and macOS, so
// the suite can read what reaches it there.
#if (                                                                  \
  defined(__linux__) && !defined(__ANDROID__) && !defined(__OHOS__) && \
  !defined(__EMSCRIPTEN__)                                             \
) ||                                                                   \
  (defined(__APPLE__) && TARGET_OS_OSX)
#define MLN_TEST_PLATFORM_LOG_ON_STDERR 1
#include <unistd.h>
#endif

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

static void restore_log_defaults(void) {
  (void)mln_log_clear_callback(NULL);
  (void)mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_DEFAULT, NULL);
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
      if (
        delivery->type == MLN_DELIVERY_TYPE_LOG_RECORD &&
        !is_dump_record(delivery->payload.log_record.message)
      ) {
        continue;
      }
      TEST_ASSERT_TRUE(seen->count < max_seen);
      const size_t slot = seen->count++;
      seen->types[slot] = delivery->type;
      seen->user_data[slot] = delivery->user_data;
      if (delivery->type == MLN_DELIVERY_TYPE_LOG_RECORD) {
        seen->records[slot] = delivery->payload.log_record;
        strncpy(
          seen->messages[slot], delivery->payload.log_record.message,
          sizeof(seen->messages[slot]) - 1
        );
        seen->records[slot].message = seen->messages[slot];
      }
    }
    mln_delivery_batch_release(drain.batch);
  }
}

LOG_STATE_CASE(a_queue_handler_copies_each_record) {
  MLN_TEST_OK(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_ERROR, NULL)
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
    TEST_ASSERT_EQUAL_STRING(expected[index], seen.records[index].message);
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

static bool ignore_log_record(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  (void)user_data;
  (void)severity;
  (void)event;
  (void)code;
  (void)message;
  return false;
}

LOG_STATE_CASE(a_queue_handler_rejects_an_invalid_shape) {
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;

  mln_log_handler handler = queue_handler(queue.handle, &tag);
  handler.callback = ignore_log_record;
  MLN_TEST_INVALID(mln_log_set_callback(&handler, NULL));
  handler = queue_handler(MLN_HANDLE_NULL, &tag);
  MLN_TEST_INVALID(mln_log_set_callback(&handler, NULL));
  handler = queue_handler(queue.handle, &tag);
  handler.release_user_data = mln_test_delivery_count_wake_release;
  MLN_TEST_INVALID(mln_log_set_callback(&handler, NULL));

  mln_test_delivery_queue_release(&queue);
  handler = queue_handler(queue.handle, &tag);
  MLN_TEST_INVALID_STATE(mln_log_set_callback(&handler, NULL));
}

#if defined(MLN_TEST_PLATFORM_LOG_ON_STDERR)

typedef struct stderr_capture {
  int saved;
  int pipe_read;
  int pipe_write;
} stderr_capture;

static void start_capturing_stderr(stderr_capture* capture) {
  int ends[2];
  TEST_ASSERT_EQUAL_INT(0, pipe(ends));
  capture->pipe_read = ends[0];
  capture->pipe_write = ends[1];
  fflush(stderr);
  capture->saved = dup(STDERR_FILENO);
  TEST_ASSERT_NOT_EQUAL_INT(-1, capture->saved);
  TEST_ASSERT_NOT_EQUAL_INT(-1, dup2(capture->pipe_write, STDERR_FILENO));
}

static void stop_capturing_stderr(
  stderr_capture* capture, char* out_text, size_t capacity
) {
  fflush(stderr);
  (void)dup2(capture->saved, STDERR_FILENO);
  (void)close(capture->saved);
  (void)close(capture->pipe_write);
  size_t length = 0;
  while (length + 1 < capacity) {
    const ssize_t read_bytes =
      read(capture->pipe_read, out_text + length, capacity - 1 - length);
    if (read_bytes <= 0) break;
    length += (size_t)read_bytes;
  }
  out_text[length] = '\0';
  (void)close(capture->pipe_read);
}

// Dumps the debug logs with stderr captured and reports what reached it.
static bool dump_capturing_stderr(char* text, size_t capacity) {
  stderr_capture capture;
  start_capturing_stderr(&capture);
  // stderr must come back even when the dump fails, or a full pipe would
  // block every later write to it.
  volatile bool dumped = false;
  if (TEST_PROTECT()) {
    dump_debug_logs();
    dumped = true;
  }
  stop_capturing_stderr(&capture, text, capacity);
  return dumped;
}

// A queue handler consumes each record it queues, so none reaches the
// platform logger; once its queue is released, every record does.
LOG_STATE_CASE(only_refused_records_reach_the_platform_logger) {
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, NULL));
  static mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  const mln_log_handler handler = queue_handler(queue.handle, &tag);
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));

  static char text[16384];
  if (!dump_capturing_stderr(text, sizeof(text))) return;
  TEST_ASSERT_NULL(strstr(text, dump_style_url));

  mln_test_delivery_queue_release(&queue);
  if (!dump_capturing_stderr(text, sizeof(text))) return;
  TEST_ASSERT_NOT_NULL(strstr(text, dump_style_url));
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(a_queue_handler_copies_each_record);
  RUN_TEST(replace_and_clear_retire_a_queue_handler_once);
  RUN_TEST(a_queue_handler_rejects_an_invalid_shape);
#if defined(MLN_TEST_PLATFORM_LOG_ON_STDERR)
  RUN_TEST(only_refused_records_reach_the_platform_logger);
#endif
}
