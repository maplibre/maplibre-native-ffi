// The process-global log callback and async severity mask. Every case here
// changes process-wide state, so each runs inside restoring_log_state(), which
// clears the callback and restores the default mask even when an assertion
// fails.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// MapLibre's platform logger writes to stderr on desktop Linux and macOS, so
// the suite can read what reaches it there. Android, OpenHarmony, Windows, the
// browser, and Apple's other platforms log to a system facility instead.
#if (                                                                  \
  defined(__linux__) && !defined(__ANDROID__) && !defined(__OHOS__) && \
  !defined(__EMSCRIPTEN__)                                             \
) ||                                                                   \
  (defined(__APPLE__) && TARGET_OS_OSX)
#define MLN_TEST_PLATFORM_LOG_ON_STDERR 1
#include <unistd.h>
#endif

#include "maplibre_native_c/callback_adapter.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void restore_log_defaults(void) {
  (void)mln_log_clear_callback(NULL);
  (void)mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_DEFAULT, NULL);
}

// Runs body, then restores the defaults whether it passed or failed. A failed
// assertion in body lands here, and Unity still reports it.
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

static uint32_t ignore_log_record(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  (void)user_data;
  (void)severity;
  (void)event;
  (void)code;
  (void)message;
  return 0;
}

static void count_log_callback_release(void* user_data) { ++*(int*)user_data; }

LOG_STATE_CASE(log_callback_releases_owned_user_data) {
  int first_releases = 0;
  int second_releases = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_callback(
      ignore_log_record, &first_releases, count_log_callback_release, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(0, first_releases);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_callback(
      ignore_log_record, &second_releases, count_log_callback_release, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(1, first_releases);
  TEST_ASSERT_EQUAL_INT(0, second_releases);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, second_releases);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, second_releases);
}

// Counts the releases a deferred log context hands its listener.
static void count_deferred_release(
  void* user_data, mln_adapter_deferred_call_record* record
) {
  if (record == NULL) {
    atomic_fetch_add((atomic_int*)user_data, 1);
  } else {
    mln_adapter_deferred_call_record_destroy(record);
  }
}

// Setting and clearing a deferred log registration releases its context once,
// after the final call. The counter outlives a failed case, whose registration
// restoring_log_state() releases later.
LOG_STATE_CASE(a_deferred_log_registration_releases_its_context_once) {
  static atomic_int releases;
  atomic_store(&releases, 0);
  void* context = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_deferred_callback_create(
                     MLN_ADAPTER_DEFERRED_LOG_CALLBACK, count_deferred_release,
                     &releases, &context, NULL
                   )
  );
  void* address =
    mln_adapter_deferred_callback_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK);
  TEST_ASSERT_NOT_NULL(address);
  mln_log_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_callback(
      callback, context, mln_adapter_deferred_callback_release, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&releases));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&releases));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&releases));
}

static const struct {
  uint32_t mask;
  mln_status expected;
} async_masks[] = {
  {MLN_LOG_SEVERITY_MASK_DEFAULT, MLN_STATUS_OK},
  {MLN_LOG_SEVERITY_MASK_ALL, MLN_STATUS_OK},
  {0, MLN_STATUS_OK},
  {MLN_LOG_SEVERITY_MASK_ERROR, MLN_STATUS_OK},
  // Bit 0 names no severity: the mask bits start at the INFO severity value.
  {UINT32_C(1), MLN_STATUS_INVALID_ARGUMENT},
  {UINT32_C(1) << 4U, MLN_STATUS_INVALID_ARGUMENT},
  {MLN_LOG_SEVERITY_MASK_ALL | (UINT32_C(1) << 31U),
   MLN_STATUS_INVALID_ARGUMENT},
};

LOG_STATE_CASE(the_async_mask_accepts_only_severity_bits) {
  for (size_t index = 0; index < sizeof(async_masks) / sizeof(*async_masks);
       index += 1) {
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    const mln_status status =
      mln_log_set_async_severity_mask(async_masks[index].mask, &diagnostic);
    TEST_ASSERT_EQUAL_INT(async_masks[index].expected, status);
    if (status != MLN_STATUS_OK) {
      TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "unknown bits"));
    }
  }
}

// The records mln_map_dump_debug_logs writes for a map whose style has no URL
// and no source: a separator, the empty style URL, and a separator.
static const char dump_separator[] =
  "----------------------------------------------------------------------"
  "----------";
static const char dump_style_url[] = "styleURL: ";
#define DUMP_RECORD_COUNT 3

// A marker whose address differs on every thread, which tells the thread that
// ran a callback apart from the others without a platform thread API.
static MLN_TEST_THREAD_LOCAL char thread_marker;

typedef struct dump_record {
  uint32_t severity;
  uint32_t event;
  int64_t code;
  char message[96];
  const void* thread;
} dump_record;

typedef struct dump_probe {
  dump_record records[DUMP_RECORD_COUNT];
  atomic_int count;
  atomic_int unexpected;
  // How many records had arrived when the command's completion ran, and on
  // which thread it ran.
  atomic_int count_at_completion;
  const void* completion_thread;
  atomic_bool completed;
} dump_probe;

static bool is_dump_record(const char* message) {
  return strcmp(message, dump_separator) == 0 ||
         strncmp(message, dump_style_url, sizeof(dump_style_url) - 1) == 0;
}

// Consumes every other record, so the verdict can only matter per record.
static uint32_t record_dump(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  dump_probe* probe = user_data;
  if (message == NULL || !is_dump_record(message)) {
    return 0;
  }
  const int index = atomic_fetch_add(&probe->count, 1);
  if (index >= DUMP_RECORD_COUNT) {
    atomic_fetch_add(&probe->unexpected, 1);
  } else {
    dump_record* record = &probe->records[index];
    record->severity = severity;
    record->event = event;
    record->code = code;
    record->thread = &thread_marker;
    const size_t length = strlen(message) < sizeof(record->message) - 1
                            ? strlen(message)
                            : sizeof(record->message) - 1;
    memcpy(record->message, message, length);
    record->message[length] = '\0';
  }
  mln_test_pulse();
  return (uint32_t)(index % 2);
}

static void record_dump_completion(
  void* user_data, const mln_completion_result* result
) {
  dump_probe* probe = user_data;
  (void)result;
  atomic_store(&probe->count_at_completion, atomic_load(&probe->count));
  probe->completion_thread = &thread_marker;
  mln_test_flag_set(&probe->completed);
}

// Dumps a map's debug logs through a callback that records them, under
// whatever async mask is in effect, and waits for the command and all three
// records. The probe must outlive a failed case: a failed wait leaves the
// callback registered until restoring_log_state() clears it, and a late
// record or completion still writes to the probe.
static void dump_debug_logs(dump_probe* probe) {
  memset(probe, 0, sizeof(*probe));
  atomic_init(&probe->count, 0);
  atomic_init(&probe->unexpected, 0);
  atomic_init(&probe->count_at_completion, -1);
  atomic_init(&probe->completed, false);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_log_set_callback(record_dump, probe, NULL, NULL)
  );

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_json(map, mln_test_empty_style_json)
  );
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = record_dump_completion,
    .user_data = probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_dump_debug_logs(map, &completion, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->completed));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&probe->count, DUMP_RECORD_COUNT));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback(NULL));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe->unexpected));
  static const char* const expected[DUMP_RECORD_COUNT] = {
    dump_separator, dump_style_url, dump_separator
  };
  for (size_t index = 0; index < DUMP_RECORD_COUNT; index += 1) {
    const dump_record* record = &probe->records[index];
    TEST_ASSERT_EQUAL_UINT32(MLN_LOG_SEVERITY_INFO, record->severity);
    TEST_ASSERT_EQUAL_UINT32(MLN_LOG_EVENT_GENERAL, record->event);
    // MapLibre's Log::Info carries no code, which it reports as -1.
    TEST_ASSERT_EQUAL_INT64(-1, record->code);
    TEST_ASSERT_EQUAL_STRING(expected[index], record->message);
  }
}

// With info records synchronous, the dump reaches the callback on the thread
// that runs the command, before the command completes. Consuming a record or
// passing it on changes nothing about the next one.
LOG_STATE_CASE(a_synchronous_record_arrives_on_the_logging_thread) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_ERROR, NULL)
  );
  static dump_probe probe;
  dump_debug_logs(&probe);
  TEST_ASSERT_EQUAL_INT(
    DUMP_RECORD_COUNT, atomic_load(&probe.count_at_completion)
  );
  for (size_t index = 0; index < DUMP_RECORD_COUNT; index += 1) {
    TEST_ASSERT_EQUAL_PTR(probe.completion_thread, probe.records[index].thread);
  }
}

// With info records asynchronous, MapLibre hands the dump to its log thread,
// which delivers every record in order and none on the thread that logged it.
LOG_STATE_CASE(an_asynchronous_record_arrives_on_the_log_thread) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_MASK_INFO, NULL)
  );
  static dump_probe probe;
  dump_debug_logs(&probe);
  const void* log_thread = probe.records[0].thread;
  TEST_ASSERT_NOT_EQUAL(probe.completion_thread, log_thread);
  TEST_ASSERT_NOT_EQUAL(&thread_marker, log_thread);
  for (size_t index = 1; index < DUMP_RECORD_COUNT; index += 1) {
    TEST_ASSERT_EQUAL_PTR(log_thread, probe.records[index].thread);
  }
}

// A rejected mask leaves the previous one in effect: here every severity
// stays synchronous.
LOG_STATE_CASE(a_rejected_async_mask_leaves_the_previous_one) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_log_set_async_severity_mask(0, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_log_set_async_severity_mask(
      MLN_LOG_SEVERITY_MASK_ALL | (UINT32_C(1) << 31U), NULL
    )
  );
  static dump_probe probe;
  dump_debug_logs(&probe);
  TEST_ASSERT_EQUAL_INT(
    DUMP_RECORD_COUNT, atomic_load(&probe.count_at_completion)
  );
}

#if defined(MLN_TEST_PLATFORM_LOG_ON_STDERR)

// Redirects stderr into a pipe until stop_capturing_stderr() restores it.
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

// Restores stderr and reads what the pipe received, up to capacity - 1 bytes.
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

// A record the callback consumes stops there. One it passes on also reaches
// MapLibre's platform logger. record_dump consumes the style URL record and
// passes on the two separators.
LOG_STATE_CASE(only_records_the_callback_passes_on_reach_the_platform_logger) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_log_set_async_severity_mask(0, NULL)
  );
  stderr_capture capture;
  start_capturing_stderr(&capture);
  // stderr must come back even when the dump fails, or a full pipe would
  // block every later write to it. Unity records a failure caught here.
  volatile bool dumped = false;
  static dump_probe probe;
  if (TEST_PROTECT()) {
    dump_debug_logs(&probe);
    dumped = true;
  }
  static char text[16384];
  stop_capturing_stderr(&capture, text, sizeof(text));
  if (!dumped) {
    return;
  }

  if (TEST_PROTECT()) {
    TEST_ASSERT_NOT_NULL(strstr(text, dump_separator));
    TEST_ASSERT_NULL(strstr(text, dump_style_url));
  }
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(log_callback_releases_owned_user_data);
  RUN_TEST(a_deferred_log_registration_releases_its_context_once);
  RUN_TEST(the_async_mask_accepts_only_severity_bits);
  RUN_TEST(a_synchronous_record_arrives_on_the_logging_thread);
  RUN_TEST(an_asynchronous_record_arrives_on_the_log_thread);
  RUN_TEST(a_rejected_async_mask_leaves_the_previous_one);
#if defined(MLN_TEST_PLATFORM_LOG_ON_STDERR)
  RUN_TEST(only_records_the_callback_passes_on_reach_the_platform_logger);
#endif
}
