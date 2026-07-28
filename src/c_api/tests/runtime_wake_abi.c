// Pump-and-wake coverage: the latch semantics of mln_runtime_pump(), the signal
// sources that release a parked owner thread, wake source lifetime across
// runtime teardown, and render-update coalescing. A parked owner thread cannot
// be observed from a binding test without a second thread and a real network
// response, so the wake sources live here.

#include <stdatomic.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char wake_style_url[] = "http://example.com/wake-style.json";
static const uint8_t wake_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// A broken wake would otherwise park the suite forever, so every blocking pump
// here is bounded and the test asserts on how long it actually took.
static const int64_t park_timeout_milliseconds = 10000;
// Well below park_timeout_milliseconds, and far above the scheduling noise a
// loaded CI machine adds to a condition-variable wake.
static const uint64_t prompt_return_milliseconds = 5000;
static const int64_t idle_park_milliseconds = 200;
static const size_t style_load_attempts = 20;
static const unsigned int signal_delay_milliseconds = 20;
static const size_t coalesced_repaint_count = 5;

static mln_runtime_event empty_event(void) {
  return (mln_runtime_event){
    .size = sizeof(mln_runtime_event),
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE,
  };
}

static size_t drain_events(mln_runtime* runtime, uint32_t counted_type) {
  size_t counted = 0;
  while (true) {
    mln_runtime_event event = empty_event();
    bool has_event = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_runtime_poll_event(runtime, &event, &has_event)
    );
    if (!has_event) {
      return counted;
    }
    if (event.type == counted_type) {
      counted += 1;
    }
  }
}

// Leaves the runtime idle with no latched wake and no unread events, so a
// following park can only be released by the signal the test raises.
static void quiesce(mln_runtime* runtime) {
  for (size_t attempt = 0; attempt < 100; attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));
    if (drain_events(runtime, 0) == 0) {
      // One more zero pump consumes a latch that the drained events raised.
      TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));
      if (drain_events(runtime, 0) == 0) {
        return;
      }
    }
  }
  TEST_FAIL_MESSAGE("The runtime kept producing events while idle.");
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
  atomic_int pump_status;
  atomic_int acquire_status;
} wrong_thread_probe;

static void foreign_thread_entry(void* argument) {
  wrong_thread_probe* probe = argument;
  atomic_store(&probe->pump_status, mln_runtime_pump(probe->runtime, 0));
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
  quiesce(runtime);

  signal_probe probe = {.source = source};
  mln_test_thread* thread =
    mln_test_thread_start(signal_wake_source_entry, &probe);

  const uint64_t started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, park_timeout_milliseconds)
  );
  const uint64_t elapsed = mln_test_monotonic_milliseconds() - started;
  // Sitting out the timeout is the failure this guards: the park has to end
  // because the foreign signal arrived, not because ten seconds passed.
  TEST_ASSERT_TRUE_MESSAGE(
    elapsed < prompt_return_milliseconds,
    "The parked owner thread timed out instead of taking the signal."
  );

  mln_test_thread_join(thread);
  TEST_ASSERT_TRUE(atomic_load(&probe.signal_done));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.signal_status));

  mln_wake_source_destroy(source);
  mln_test_destroy_runtime(runtime);
}

// The signal is a latch rather than an edge a parked thread has to be present
// for, so a host that signals before it parks does not lose the wake, and one
// pump consumes exactly one latch.
static void a_signal_that_precedes_the_pump_is_latched(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_wake_source* source = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  quiesce(runtime);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_wake_source_signal(source));

  uint64_t started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, park_timeout_milliseconds)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_monotonic_milliseconds() - started < prompt_return_milliseconds,
    "A pump blocked despite a latched signal."
  );

  // The latch is spent, so an idle runtime now sits out its whole timeout.
  started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, idle_park_milliseconds)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_monotonic_milliseconds() - started >=
      (uint64_t)idle_park_milliseconds / 2,
    "A second pump consumed a latch that the first one should have spent."
  );

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
  quiesce(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_url(map, wake_style_url)
  );

  const uint64_t started = mln_test_monotonic_milliseconds();
  bool style_loaded = false;
  for (size_t attempt = 0; attempt < style_load_attempts && !style_loaded;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_runtime_pump(runtime, park_timeout_milliseconds)
    );
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
  // Every park before the style resolves is ended by native work, so the whole
  // load costs well under one timeout rather than one per iteration.
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_monotonic_milliseconds() - started < prompt_return_milliseconds,
    "Parks sat out their timeouts while the style load was pending."
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A host that stops polling before the queue empties would park behind events
// it has already been handed, because queueing them latched a wake an earlier
// pump consumed.
static void queued_events_return_from_the_pump_immediately(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  quiesce(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, (const char*)wake_style_json)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));

  // The queue now holds events this test has not read. A blocking pump must
  // not park behind them even though the latch they raised is spent.
  const uint64_t started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, park_timeout_milliseconds)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_monotonic_milliseconds() - started < prompt_return_milliseconds,
    "A pump parked behind unread runtime events."
  );

  mln_runtime_event event = empty_event();
  bool has_event = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_poll_event(runtime, &event, &has_event)
  );
  TEST_ASSERT_TRUE(has_event);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A render always draws the latest update, so back-to-back invalidations must
// collapse to one queued event. Otherwise a host that renders per event redraws
// successively newer state once per invalidation for one frame of progress.
static void render_updates_coalesce_at_the_queue_tail(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  quiesce(runtime);

  // Each repaint request invalidates the map synchronously on this thread, so
  // nothing can interleave between the pushes.
  for (size_t repaint = 0; repaint < coalesced_repaint_count; repaint += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_request_repaint(map));
  }

  const size_t render_updates =
    drain_events(runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE);
  TEST_ASSERT_EQUAL_size_t(1, render_updates);

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
static void pump_and_wake_sources_reject_raw_invalid_arguments(void) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, mln_runtime_pump(NULL, 0));
  mln_wake_source* source = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_wake_source_acquire(NULL, &source)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_wake_source_signal(NULL)
  );

  mln_runtime* runtime = mln_test_create_runtime();
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
    MLN_STATUS_WRONG_THREAD, atomic_load(&probe.pump_status)
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
  RUN_TEST(a_signal_that_precedes_the_pump_is_latched);
  RUN_TEST(a_style_response_wakes_a_parked_owner_thread);
  RUN_TEST(queued_events_return_from_the_pump_immediately);
  RUN_TEST(render_updates_coalesce_at_the_queue_tail);
  RUN_TEST(a_wake_source_outlives_its_runtime);
  RUN_TEST(pump_and_wake_sources_reject_raw_invalid_arguments);
}
