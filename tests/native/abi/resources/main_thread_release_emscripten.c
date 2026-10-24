// A resource request that the browser main thread releases while its cancel
// callback runs on another thread. Release waits for that callback to return,
// and the callback's thread already runs, so the wait needs nothing from the
// main thread.
//
// The suite runs main() on a pthread, so the browser main thread is free to
// run a call proxied to it.

#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <maplibre_native_c.h>

#include "support/resources.h"
#include "support/test_support.h"

typedef struct main_thread_release {
  atomic_bool provider_entered;
  atomic_int register_status;
  atomic_bool register_reported_cancelled;
  _Atomic mln_resource_request handle;
  atomic_bool callback_entered;
  // Set by the main thread just before it releases the request.
  atomic_bool releasing;
  // Set while the main thread is inside the release.
  atomic_bool inside_release;
  atomic_bool probe_ran;
  atomic_bool probe_ran_inside_release;
  atomic_bool callback_returned;
  bool callback_returned_before_release;
} main_thread_release;

// Runs on the main thread, from the system proxying queue.
static void note_probe(void* context) {
  main_thread_release* run = context;
  atomic_store(
    &run->probe_ran_inside_release, atomic_load(&run->inside_release)
  );
  mln_test_flag_set(&run->probe_ran);
}

// Runs on the runtime's scheduler thread, which discards the request when the
// map is destroyed. The main thread runs the system queue inside a wait, so
// the probe runs inside the release only when the release waits for this
// callback. The callback returns once the probe ran, wherever it ran.
static void block_in_cancel(void* user_data, mln_resource_request handle) {
  (void)handle;
  main_thread_release* run = user_data;
  mln_test_flag_set(&run->callback_entered);
  (void)mln_test_wait_for_flag(&run->releasing);
  emscripten_proxy_async(
    emscripten_proxy_get_system_queue(), emscripten_main_runtime_thread_id(),
    note_probe, run
  );
  (void)mln_test_wait_for_flag(&run->probe_ran);
  mln_test_flag_set(&run->callback_returned);
}

static void blocking_cancel_provider(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)request;
  main_thread_release* run = user_data;
  atomic_store(&run->handle, handle);
  bool cancelled = true;
  const mln_resource_request_cancel_handler handler = {
    .size = sizeof(mln_resource_request_cancel_handler),
    .callback = block_in_cancel,
    .user_data = run,
  };
  atomic_store(
    &run->register_status,
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  atomic_store(&run->register_reported_cancelled, cancelled);
  mln_test_flag_set(&run->provider_entered);
}

static void release_here(void* context) {
  main_thread_release* run = context;
  atomic_store(&run->inside_release, true);
  mln_test_flag_set(&run->releasing);
  mln_resource_request_release(atomic_load(&run->handle));
  run->callback_returned_before_release = atomic_load(&run->callback_returned);
  atomic_store(&run->inside_release, false);
}

static void release_on_the_main_thread_returns_while_a_cancel_runs(void) {
  main_thread_release run = {0};
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = blocking_cancel_provider,
    .user_data = &run,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://main-release.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&run.provider_entered));
  MLN_TEST_OK(atomic_load(&run.register_status));
  TEST_ASSERT_FALSE(atomic_load(&run.register_reported_cancelled));

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&run.callback_entered));
  // A private queue runs the release from the main thread's event loop. A
  // task of the system queue would hold that queue, and the main thread's
  // wait would then skip the probe.
  em_proxying_queue* queue = em_proxying_queue_create();
  TEST_ASSERT_NOT_NULL(queue);
  const int proxied = emscripten_proxy_sync(
    queue, emscripten_main_runtime_thread_id(), release_here, &run
  );
  em_proxying_queue_destroy(queue);
  // The callback reads the case's state until it returns.
  const bool callback_returned = mln_test_wait_for_flag(&run.callback_returned);

  TEST_ASSERT_EQUAL_INT(1, proxied);
  TEST_ASSERT_TRUE(callback_returned);
  TEST_ASSERT_TRUE_MESSAGE(
    atomic_load(&run.probe_ran_inside_release),
    "the release returned without waiting for the running cancel callback"
  );
  TEST_ASSERT_TRUE(run.callback_returned_before_release);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(release_on_the_main_thread_returns_while_a_cancel_runs);
}
