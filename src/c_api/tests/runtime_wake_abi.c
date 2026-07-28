// Park-and-wake coverage: the latch semantics of mln_runtime_wait(), the
// signal sources that release a parked owner thread, and wake source lifetime
// across runtime teardown. A parked owner thread cannot be observed from a
// binding test without a second thread and a real network response, so the
// wake sources live here.

#include <stdatomic.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char wake_style_url[] = "http://example.com/wake-style.json";
static const uint8_t wake_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// A broken wake would otherwise park the suite forever, so every wait in this
// file is bounded and reports the timeout through `signaled` instead.
static const int64_t wake_timeout_milliseconds = 10000;
static const size_t style_load_attempts = 20;
static const unsigned int signal_delay_milliseconds = 20;

static mln_runtime_event empty_event(void) {
  return (mln_runtime_event){
    .size = sizeof(mln_runtime_event),
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE,
  };
}

// Consumes whatever the runtime latched while the test set itself up, so a
// later wait measures only the signal the test triggers.
static void drain_latched_wakes(mln_runtime* runtime) {
  for (size_t attempt = 0; attempt < 100; attempt += 1) {
    bool signaled = true;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_runtime_wait(runtime, 0, &signaled)
    );
    if (!signaled) {
      return;
    }
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_run_once(runtime));
    while (true) {
      mln_runtime_event event = empty_event();
      bool has_event = false;
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK, mln_runtime_poll_event(runtime, &event, &has_event)
      );
      if (!has_event) {
        break;
      }
    }
  }
  TEST_FAIL_MESSAGE("The runtime kept latching wakes while idle.");
}

typedef struct signal_probe {
  mln_wake_source* source;
  atomic_int signal_status;
  atomic_bool signal_done;
} signal_probe;

// Signals from a thread that owns no runtime, which is the shape a host's task
// submission path has.
static void signal_wake_source_entry(void* argument) {
  signal_probe* probe = argument;
  mln_test_sleep_milliseconds(signal_delay_milliseconds);
  atomic_store(&probe->signal_status, mln_wake_source_signal(probe->source));
  atomic_store(&probe->signal_done, true);
}

typedef struct wrong_thread_probe {
  mln_runtime* runtime;
  atomic_int wait_status;
  atomic_int acquire_status;
} wrong_thread_probe;

static void foreign_thread_entry(void* argument) {
  wrong_thread_probe* probe = argument;
  bool signaled = true;
  atomic_store(
    &probe->wait_status, mln_runtime_wait(probe->runtime, 0, &signaled)
  );
  mln_wake_source* source = NULL;
  atomic_store(
    &probe->acquire_status,
    mln_runtime_wake_source_acquire(probe->runtime, &source)
  );
  mln_wake_source_destroy(source);
}

static uint32_t wake_style_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle* handle
) {
  (void)user_data;
  (void)request;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = wake_style_json,
    .byte_count = sizeof(wake_style_json) - 1,
  };
  mln_resource_request_complete(handle, &response);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

// A host that parks has no way to learn about work except the wake, so this
// pins the signal that a foreign thread raises while the owner thread blocks.
static void a_wake_source_releases_a_parked_owner_thread(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_wake_source* source = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  TEST_ASSERT_NOT_NULL(source);
  drain_latched_wakes(runtime);

  signal_probe probe = {.source = source};
  mln_test_thread* thread =
    mln_test_thread_start(signal_wake_source_entry, &probe);

  bool signaled = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_wait(runtime, wake_timeout_milliseconds, &signaled)
  );
  // A timeout reports `false`, so this is the assertion that the wait blocked
  // and the foreign signal is what ended it.
  TEST_ASSERT_TRUE_MESSAGE(
    signaled, "The parked owner thread timed out instead of taking the signal."
  );

  mln_test_thread_join(thread);
  TEST_ASSERT_TRUE(atomic_load(&probe.signal_done));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.signal_status));

  mln_wake_source_destroy(source);
  mln_test_destroy_runtime(runtime);
}

// The signal is a latch rather than an edge a parked thread has to be present
// for, so a host that signals before it parks does not lose the wake.
static void a_signal_that_precedes_the_wait_is_latched(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_wake_source* source = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  drain_latched_wakes(runtime);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_wake_source_signal(source));

  bool signaled = false;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_wait(runtime, 0, &signaled));
  TEST_ASSERT_TRUE(signaled);

  // One wait consumes one latch, so an idle runtime parks again afterwards.
  signaled = true;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_wait(runtime, 0, &signaled));
  TEST_ASSERT_FALSE(signaled);

  mln_wake_source_destroy(source);
  mln_test_destroy_runtime(runtime);
}

// The reason the API exists: a style response that arrives on a MapLibre file
// source thread has to release the parked owner thread, or loading stalls until
// the host's timeout.
static void a_style_response_wakes_a_parked_owner_thread(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = wake_style_provider,
    .user_data = NULL,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_set_resource_provider(runtime, &provider)
  );
  mln_map* map = mln_test_create_map(runtime);
  drain_latched_wakes(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_url(map, wake_style_url)
  );

  bool style_loaded = false;
  for (size_t attempt = 0; attempt < style_load_attempts && !style_loaded;
       attempt += 1) {
    bool signaled = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_runtime_wait(runtime, wake_timeout_milliseconds, &signaled)
    );
    // Every park before the style resolves is ended by native work, so a
    // timeout here means a signal source is missing rather than slow.
    TEST_ASSERT_TRUE_MESSAGE(
      signaled, "A park timed out while the style load was still pending."
    );
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_run_once(runtime));
    while (true) {
      mln_runtime_event event = empty_event();
      bool has_event = false;
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK, mln_runtime_poll_event(runtime, &event, &has_event)
      );
      if (!has_event) {
        break;
      }
      TEST_ASSERT_NOT_EQUAL_INT(
        MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, (int)event.type
      );
      if (event.type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
        style_loaded = true;
      }
    }
  }
  TEST_ASSERT_TRUE(style_loaded);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A host that stops polling before the queue empties would park behind events
// it has already been handed, because queueing them latched a wake an earlier
// wait consumed.
static void queued_events_return_from_the_wait_immediately(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  drain_latched_wakes(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, (const char*)wake_style_json)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_run_once(runtime));

  // The first wait consumes the latch the queued event raised. The second one
  // can only report a signal from the queue the host has not drained.
  bool signaled = false;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_wait(runtime, 0, &signaled));
  TEST_ASSERT_TRUE(signaled);
  signaled = false;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_wait(runtime, 0, &signaled));
  TEST_ASSERT_TRUE(signaled);

  mln_runtime_event event = empty_event();
  bool has_event = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_poll_event(runtime, &event, &has_event)
  );
  TEST_ASSERT_TRUE(has_event);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A host tears its wake source and its runtime down in either order, and a
// submission path that signals during shutdown races that teardown.
static void a_wake_source_outlives_its_runtime(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_wake_source* source = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  mln_test_destroy_runtime(runtime);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_wake_source_signal(source));
  mln_wake_source_destroy(source);
  mln_wake_source_destroy(NULL);
}

// This verifies raw null handles, output initialization, and owner-thread
// validation that binding wrappers hide.
static void wait_and_wake_sources_reject_raw_invalid_arguments(void) {
  bool signaled = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_wait(NULL, 0, &signaled)
  );
  mln_wake_source* source = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_wake_source_acquire(NULL, &source)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_wake_source_signal(NULL)
  );

  mln_runtime* runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_wait(runtime, 0, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_wake_source_acquire(runtime, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  // A non-null output handle would be overwritten and leaked.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_wake_source_acquire(runtime, &source)
  );

  wrong_thread_probe probe = {.runtime = runtime};
  mln_test_thread* thread = mln_test_thread_start(foreign_thread_entry, &probe);
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_WRONG_THREAD, atomic_load(&probe.wait_status)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_WRONG_THREAD, atomic_load(&probe.acquire_status)
  );

  mln_wake_source_destroy(source);
  mln_test_destroy_runtime(runtime);
}

void run_runtime_wake_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(a_wake_source_releases_a_parked_owner_thread);
  RUN_TEST(a_signal_that_precedes_the_wait_is_latched);
  RUN_TEST(a_style_response_wakes_a_parked_owner_thread);
  RUN_TEST(queued_events_return_from_the_wait_immediately);
  RUN_TEST(a_wake_source_outlives_its_runtime);
  RUN_TEST(wait_and_wake_sources_reject_raw_invalid_arguments);
}
