// Raw C ABI coverage for the mln_runtime_pump() wake flag, the signal sources
// that release a parked owner thread, wake source lifetime across runtime
// teardown, and render-update coalescing. These need a second thread and a real
// network response, which bindings hide.

#include <stdatomic.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char wake_style_url[] = "http://example.com/wake-style.json";
static const uint8_t wake_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// Every blocking pump here is bounded, and each test asserts on elapsed time so
// a missing wake fails rather than hangs.
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

static size_t drain_events(mln_runtime runtime, uint32_t counted_type) {
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

// Pumps until the runtime is idle: the wake flag is clear and no events are
// queued. A park that follows is released by the signal the test raises.
static void quiesce(mln_runtime runtime) {
  for (size_t attempt = 0; attempt < 100; attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));
    if (drain_events(runtime, 0) == 0) {
      // One more zero pump clears the flag the drained events set.
      TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));
      if (drain_events(runtime, 0) == 0) {
        return;
      }
    }
  }
  TEST_FAIL_MESSAGE("The runtime kept producing events while idle.");
}

typedef struct signal_probe {
  mln_wake_source source;
  atomic_int signal_status;
  atomic_bool signal_done;
} signal_probe;

// Signals from a thread that owns no runtime, matching a host's task
// submission path.
static void signal_wake_source_entry(void* argument) {
  signal_probe* probe = argument;
  mln_test_sleep_milliseconds(signal_delay_milliseconds);
  atomic_store(&probe->signal_status, mln_wake_source_signal(probe->source));
  atomic_store(&probe->signal_done, true);
}

typedef struct wrong_thread_probe {
  mln_runtime runtime;
  atomic_int pump_status;
  atomic_int acquire_status;
} wrong_thread_probe;

static void foreign_thread_entry(void* argument) {
  wrong_thread_probe* probe = argument;
  atomic_store(&probe->pump_status, mln_runtime_pump(probe->runtime, 0));
  mln_wake_source source = MLN_HANDLE_NULL;
  atomic_store(
    &probe->acquire_status,
    mln_runtime_wake_source_acquire(probe->runtime, &source)
  );
  mln_wake_source_destroy(source);
}

static uint32_t wake_style_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
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

// Pins the signal a foreign thread raises while the owner thread is parked.
static void a_wake_source_releases_a_parked_owner_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_wake_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, source);
  quiesce(runtime);

  signal_probe probe = {.source = source};
  mln_test_thread* thread =
    mln_test_thread_start(signal_wake_source_entry, &probe);

  const uint64_t started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, park_timeout_milliseconds)
  );
  const uint64_t elapsed = mln_test_monotonic_milliseconds() - started;
  // Elapsed time is what separates a signalled wake from an expired timeout.
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

// A signal raised before the pump sets the wake flag, and one pump clears it.
static void a_signal_before_the_pump_sets_the_wake_flag(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_wake_source source = MLN_HANDLE_NULL;
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
    "A pump waited even though the wake flag was set."
  );

  // The pump above cleared the wake flag, so this one waits its full timeout.
  started = mln_test_monotonic_milliseconds();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_pump(runtime, idle_park_milliseconds)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_monotonic_milliseconds() - started >=
      (uint64_t)idle_park_milliseconds / 2,
    "The first pump left the wake flag set."
  );

  mln_wake_source_destroy(source);
  mln_test_destroy_runtime(runtime);
}

// A style response arrives on a MapLibre file source thread and releases the
// parked owner thread.
static void a_style_response_wakes_a_parked_owner_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = wake_style_provider,
    .user_data = NULL,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_set_resource_provider(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
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
  // Native work ends each park, so the load costs well under one timeout.
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_monotonic_milliseconds() - started < prompt_return_milliseconds,
    "Parks sat out their timeouts while the style load was pending."
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Queued unread events return a blocking pump without parking.
static void queued_events_return_from_the_pump_immediately(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  quiesce(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, (const char*)wake_style_json)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));

  // The queue holds unread events and the wake flag is clear, so the queued
  // events are what return the pump.
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

// A render draws the latest update, so back-to-back invalidations collapse to
// one queued event.
static void render_updates_coalesce_at_the_queue_tail(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  quiesce(runtime);

  // Each repaint request invalidates the map synchronously on this thread, so
  // the pushes land back to back.
  for (size_t repaint = 0; repaint < coalesced_repaint_count; repaint += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_request_repaint(map));
  }

  const size_t render_updates =
    drain_events(runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE);
  TEST_ASSERT_EQUAL_size_t(1, render_updates);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A wake source stays signalable and releasable after its runtime closes, so
// hosts tear the two down in either order.
static void a_wake_source_outlives_its_runtime(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_wake_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  mln_test_destroy_runtime(runtime);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_wake_source_signal(source));
  mln_wake_source_destroy(source);
  mln_wake_source_destroy(MLN_HANDLE_NULL);
}

typedef struct stale_signal_probe {
  mln_wake_source source;
  atomic_int signal_status;
  atomic_bool signal_done;
} stale_signal_probe;

static void signal_stale_wake_source_entry(void* argument) {
  stale_signal_probe* probe = argument;
  atomic_store(&probe->signal_status, mln_wake_source_signal(probe->source));
  atomic_store(&probe->signal_done, true);
}

// Signalling is documented as any-thread, so a released wake source can reach
// mln_wake_source_signal() from a thread that never saw the release. The
// signal is rejected, and no later wake source can be signalled in its place.
static void a_released_wake_source_rejects_a_foreign_thread_signal(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_wake_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  mln_wake_source_destroy(source);

  // Acquiring again reuses the slot the release just freed, so this proves the
  // generation and not merely the slot's emptiness.
  mln_wake_source reused = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &reused)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(source, reused);

  stale_signal_probe probe = {.source = source};
  atomic_store(&probe.signal_status, MLN_STATUS_OK);
  atomic_store(&probe.signal_done, false);
  mln_test_thread* thread =
    mln_test_thread_start(signal_stale_wake_source_entry, &probe);
  TEST_ASSERT_NOT_NULL(thread);
  mln_test_thread_join(thread);
  TEST_ASSERT_TRUE(atomic_load(&probe.signal_done));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, atomic_load(&probe.signal_status)
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_wake_source_signal(reused));
  mln_wake_source_destroy(reused);
  mln_test_destroy_runtime(runtime);
}

// This verifies raw null handles, output initialization, and owner-thread
// validation that binding wrappers hide.
static void pump_and_wake_sources_reject_raw_invalid_arguments(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_pump(MLN_HANDLE_NULL, 0)
  );
  mln_wake_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_wake_source_acquire(MLN_HANDLE_NULL, &source)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_wake_source_signal(MLN_HANDLE_NULL)
  );

  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_wake_source_acquire(runtime, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_wake_source_acquire(runtime, &source)
  );
  // A non-null output handle is rejected so the live handle survives.
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
  RUN_TEST(a_signal_before_the_pump_sets_the_wake_flag);
  RUN_TEST(a_style_response_wakes_a_parked_owner_thread);
  RUN_TEST(queued_events_return_from_the_pump_immediately);
  RUN_TEST(render_updates_coalesce_at_the_queue_tail);
  RUN_TEST(a_wake_source_outlives_its_runtime);
  RUN_TEST(a_released_wake_source_rejects_a_foreign_thread_signal);
  RUN_TEST(pump_and_wake_sources_reject_raw_invalid_arguments);
}
