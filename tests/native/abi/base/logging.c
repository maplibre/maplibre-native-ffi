// The process-global log callback and async severity mask. Every case here
// changes process-wide state, so each runs inside restoring_log_state(), which
// clears the callback and restores the default mask even when an assertion
// fails.

#include "support/test_support.h"

static void restore_log_defaults(void) {
  (void)mln_log_clear_callback(NULL);
  (void)mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_DEFAULT, NULL);
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

static bool ignore_log_record(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  mln_buffer_view message
) {
  (void)user_data;
  (void)severity;
  (void)event;
  (void)code;
  (void)message;
  return false;
}

static void count_log_callback_release(void* user_data) { ++*(int*)user_data; }

static mln_log_handler log_handler(
  mln_log_callback callback, void* user_data,
  mln_user_data_release release_user_data
) {
  return (mln_log_handler){
    .size = sizeof(mln_log_handler),
    .callback = callback,
    .user_data = user_data,
    .release_user_data = release_user_data,
  };
}

LOG_STATE_CASE(log_callback_releases_owned_user_data) {
  int first_releases = 0;
  int second_releases = 0;
  const mln_log_handler first =
    log_handler(ignore_log_record, &first_releases, count_log_callback_release);
  MLN_TEST_OK(mln_log_set_callback(&first, NULL));
  TEST_ASSERT_EQUAL_INT(0, first_releases);

  const mln_log_handler second = log_handler(
    ignore_log_record, &second_releases, count_log_callback_release
  );
  MLN_TEST_OK(mln_log_set_callback(&second, NULL));
  TEST_ASSERT_EQUAL_INT(1, first_releases);
  TEST_ASSERT_EQUAL_INT(0, second_releases);

  MLN_TEST_OK(mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, second_releases);
  MLN_TEST_OK(mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, second_releases);
}

// A rejected handler leaves the installed one in place and releases neither
// its own user data nor the installed handler's.
LOG_STATE_CASE(a_rejected_handler_keeps_the_installed_one) {
  int installed_releases = 0;
  int rejected_releases = 0;
  const mln_log_handler installed = log_handler(
    ignore_log_record, &installed_releases, count_log_callback_release
  );
  MLN_TEST_OK(mln_log_set_callback(&installed, NULL));

  MLN_TEST_INVALID(mln_log_set_callback(NULL, NULL));
  mln_log_handler undersized = log_handler(
    ignore_log_record, &rejected_releases, count_log_callback_release
  );
  undersized.size = sizeof(mln_log_handler) - 1;
  MLN_TEST_INVALID(mln_log_set_callback(&undersized, NULL));
  const mln_log_handler without_callback =
    log_handler(NULL, &rejected_releases, count_log_callback_release);
  MLN_TEST_INVALID(mln_log_set_callback(&without_callback, NULL));
  TEST_ASSERT_EQUAL_INT(0, installed_releases);
  TEST_ASSERT_EQUAL_INT(0, rejected_releases);

  MLN_TEST_OK(mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_INT(1, installed_releases);
  TEST_ASSERT_EQUAL_INT(0, rejected_releases);
}

static const struct {
  uint32_t mask;
  mln_status expected;
} async_masks[] = {
  {MLN_LOG_SEVERITY_FLAG_ALL, MLN_STATUS_OK},
  {0, MLN_STATUS_OK},
  // Bit 0 names no severity: the mask bits start at the INFO severity value.
  {UINT32_C(1), MLN_STATUS_INVALID_ARGUMENT},
  {UINT32_C(1) << 4U, MLN_STATUS_INVALID_ARGUMENT},
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

typedef struct discarded_failure_probe {
  atomic_uint warnings;
  uint32_t severity;
  uint32_t event;
  char message[256];
} discarded_failure_probe;

static bool record_discarded_failure(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  mln_buffer_view message_view
) {
  const char* message = message_view.data;
  (void)code;
  discarded_failure_probe* probe = user_data;
  if (message != NULL && strstr(message, "discarded command failed") != NULL) {
    probe->severity = severity;
    probe->event = event;
    snprintf(probe->message, sizeof(probe->message), "%s", message);
    atomic_fetch_add(&probe->warnings, 1);
  }
  return true;
}

LOG_STATE_CASE(a_failed_discarded_command_logs_one_warning) {
  static discarded_failure_probe probe;
  memset(&probe, 0, sizeof(probe));
  atomic_init(&probe.warnings, 0);
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, NULL));
  const mln_log_handler handler =
    log_handler(record_discarded_failure, &probe, NULL);
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_map_end_command_group(map, NULL, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));
  MLN_TEST_OK(mln_log_clear_callback(NULL));
  TEST_ASSERT_EQUAL_UINT32(1, atomic_load(&probe.warnings));
  TEST_ASSERT_EQUAL_UINT32(MLN_LOG_SEVERITY_WARNING, probe.severity);
  TEST_ASSERT_EQUAL_UINT32(MLN_LOG_EVENT_GENERAL, probe.event);
  char status[32];
  snprintf(status, sizeof(status), "status %d", MLN_STATUS_INVALID_STATE);
  TEST_ASSERT_NOT_NULL(strstr(probe.message, status));
  TEST_ASSERT_NOT_NULL(strstr(probe.message, "map has no open command group"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// What a log callback and its release saw when they called back into the
// logging API, which MapLibre Native runs under a mutex that is not recursive.
typedef struct reentry_probe {
  atomic_int callback_status;
  atomic_int release_status;
  char diagnostic[MLN_DIAGNOSTIC_MESSAGE_CAPACITY];
} reentry_probe;

static bool clear_from_log_callback(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  mln_buffer_view message_view
) {
  const char* message = message_view.data;
  (void)severity;
  (void)event;
  (void)code;
  reentry_probe* probe = user_data;
  if (message != NULL && strstr(message, "discarded command failed") != NULL) {
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    const mln_status status = mln_log_clear_callback(&diagnostic);
    snprintf(
      probe->diagnostic, sizeof(probe->diagnostic), "%s", diagnostic.message
    );
    atomic_store(&probe->callback_status, status);
  }
  return true;
}

static void clear_from_log_release(void* user_data) {
  reentry_probe* probe = user_data;
  atomic_store(&probe->release_status, mln_log_clear_callback(NULL));
}

// A log callback and its release that call back into the C API get an
// invalid-state status, and the handler stays installed until the host
// replaces it.
LOG_STATE_CASE(a_log_callback_cannot_call_back_into_the_c_api) {
  static reentry_probe probe;
  memset(&probe, 0, sizeof(probe));
  atomic_init(&probe.callback_status, MLN_STATUS_OK);
  atomic_init(&probe.release_status, MLN_STATUS_OK);
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, NULL));
  const mln_log_handler handler =
    log_handler(clear_from_log_callback, &probe, clear_from_log_release);
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_map_end_command_group(map, NULL, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, atomic_load(&probe.callback_status)
  );
  TEST_ASSERT_NOT_NULL(strstr(probe.diagnostic, "log callback"));

  // Replacing the handler releases its user data under the log mutex.
  const mln_log_handler replacement =
    log_handler(ignore_log_record, NULL, NULL);
  MLN_TEST_OK(mln_log_set_callback(&replacement, NULL));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, atomic_load(&probe.release_status)
  );
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
  const void* command_thread;
  atomic_bool command_thread_recorded;
  // How many records had arrived when the command's completion ran.
  atomic_int count_at_completion;
  atomic_bool completed;
} dump_probe;

// A camera command's transition-end handler identifies its runtime worker.
static void record_command_thread(
  void* user_data, const mln_camera_transition_end* end
) {
  dump_probe* probe = user_data;
  (void)end;
  probe->command_thread = &thread_marker;
  mln_test_flag_set(&probe->command_thread_recorded);
}

static bool is_dump_record(const char* message) {
  return strcmp(message, dump_separator) == 0 ||
         strncmp(message, dump_style_url, sizeof(dump_style_url) - 1) == 0;
}

// Consumes every other record, so the verdict can only matter per record.
static bool record_dump(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  mln_buffer_view message_view
) {
  const char* message = message_view.data;
  dump_probe* probe = user_data;
  if (message == NULL || !is_dump_record(message)) {
    return false;
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
  atomic_init(&probe->command_thread_recorded, false);
  atomic_init(&probe->count_at_completion, -1);
  atomic_init(&probe->completed, false);
  const mln_log_handler handler = log_handler(record_dump, probe, NULL);
  MLN_TEST_OK(mln_log_set_callback(&handler, NULL));

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  // An empty camera command identifies the worker shared by this map's
  // commands without changing its camera or relying on a completion's thread.
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.animation.end_handler = (mln_camera_transition_handler){
    .callback = record_command_thread,
    .user_data = probe,
  };
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->command_thread_recorded));
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = record_dump_completion,
    .user_data = probe,
  };
  MLN_TEST_OK(mln_map_dump_debug_logs(map, &completion, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe->completed));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&probe->count, DUMP_RECORD_COUNT));
  MLN_TEST_OK(mln_log_clear_callback(NULL));

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
  MLN_TEST_OK(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_ERROR, NULL)
  );
  static dump_probe probe;
  dump_debug_logs(&probe);
  TEST_ASSERT_EQUAL_INT(
    DUMP_RECORD_COUNT, atomic_load(&probe.count_at_completion)
  );
  for (size_t index = 0; index < DUMP_RECORD_COUNT; index += 1) {
    TEST_ASSERT_EQUAL_PTR(probe.command_thread, probe.records[index].thread);
  }
}

// With info records asynchronous, MapLibre hands the dump to its log thread,
// which delivers every record in order and none on the thread that logged it.
LOG_STATE_CASE(an_asynchronous_record_arrives_on_the_log_thread) {
  MLN_TEST_OK(
    mln_log_set_async_severity_mask(MLN_LOG_SEVERITY_FLAG_INFO, NULL)
  );
  static dump_probe probe;
  dump_debug_logs(&probe);
  const void* log_thread = probe.records[0].thread;
  TEST_ASSERT_NOT_EQUAL(probe.command_thread, log_thread);
  TEST_ASSERT_NOT_EQUAL(&thread_marker, log_thread);
  for (size_t index = 1; index < DUMP_RECORD_COUNT; index += 1) {
    TEST_ASSERT_EQUAL_PTR(log_thread, probe.records[index].thread);
  }
}

// Clearing the callback stops delivery: a later dump reaches it no more.
LOG_STATE_CASE(a_cleared_callback_receives_no_more_records) {
  MLN_TEST_OK(mln_log_set_async_severity_mask(0, NULL));
  static dump_probe probe;
  dump_debug_logs(&probe);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  MLN_TEST_AWAIT_OK(mln_map_dump_debug_logs(map, &completion.descriptor, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(DUMP_RECORD_COUNT, atomic_load(&probe.count));
}

MLN_TEST_GROUP {
  RUN_TEST(log_callback_releases_owned_user_data);
  RUN_TEST(a_rejected_handler_keeps_the_installed_one);
  RUN_TEST(the_async_mask_accepts_only_severity_bits);
  RUN_TEST(a_failed_discarded_command_logs_one_warning);
  RUN_TEST(a_log_callback_cannot_call_back_into_the_c_api);
  RUN_TEST(a_synchronous_record_arrives_on_the_logging_thread);
  RUN_TEST(an_asynchronous_record_arrives_on_the_log_thread);
  RUN_TEST(a_cleared_callback_receives_no_more_records);
}
