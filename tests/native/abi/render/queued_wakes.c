// Render session wakes that name a delivery queue: the wake's calls and its
// release run in mln_delivery_queue_dispatch(), wakes that arrive while a call
// is queued join it, and attach checks the queue.

#include <string.h>

#include "support/delivery.h"
#include "support/frames.h"
#include "support/test_support.h"

// Set on the thread that runs the case, which is the only one that dispatches.
static MLN_TEST_THREAD_LOCAL bool on_case_thread;

typedef struct queued_wake {
  atomic_int off_thread;
  int calls;
  int releases;
  int calls_at_release;
} queued_wake;

static void count_call(void* user_data) {
  queued_wake* wake = user_data;
  if (!on_case_thread) {
    atomic_fetch_add(&wake->off_thread, 1);
    return;
  }
  wake->calls += 1;
}

static void count_release(void* user_data) {
  queued_wake* wake = user_data;
  if (!on_case_thread) {
    atomic_fetch_add(&wake->off_thread, 1);
    return;
  }
  wake->releases += 1;
  wake->calls_at_release = wake->calls;
}

typedef struct release_wait {
  mln_delivery_queue queue;
  const queued_wake* wake;
  mln_status failure;
} release_wait;

static bool dispatched_release(void* context) {
  release_wait* wait = context;
  const mln_status status = mln_delivery_queue_dispatch(wait->queue, NULL);
  if (status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY) {
    wait->failure = status;
    return true;
  }
  return wait->wake->releases != 0;
}

// Two frame results each wake the host, but nothing runs until dispatch, and
// the second wake joins the call the first queued. Disposal queues the wake's
// release after that call.
static void a_queued_frame_wake_runs_once_in_dispatch(void) {
  on_case_thread = true;
  static queued_wake wake;
  memset(&wake, 0, sizeof(wake));
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.requested_texture_ring_depth = 2;
  options.frame_wake = (mln_wake){
    .callback = count_call,
    .user_data = &wake,
    .release_user_data = count_release,
    .queue = queue.handle,
  };
  options.driver_work_wake = mln_test_pulse_wake();
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);

  mln_test_render_request_forced(&fixture, 1);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture));
  mln_test_render_request_forced(&fixture, 2);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture));
  TEST_ASSERT_EQUAL_INT(0, wake.calls);

  MLN_TEST_OK(mln_test_render_session_dispose(fixture.session));
  release_wait wait = {
    .queue = queue.handle, .wake = &wake, .failure = MLN_STATUS_OK
  };
  TEST_ASSERT_TRUE(mln_test_await(
    dispatched_release, &wait, mln_test_deadline_default(),
    "the queued wake's release"
  ));
  MLN_TEST_OK(wait.failure);
  TEST_ASSERT_EQUAL_INT(1, wake.calls);
  TEST_ASSERT_EQUAL_INT(1, wake.releases);
  TEST_ASSERT_EQUAL_INT(1, wake.calls_at_release);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&wake.off_thread));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

static void ignore_wake(void* user_data) { (void)user_data; }

// A disabled wake names no queue, and a released queue is refused.
static void attach_checks_a_wake_queue(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.frame_wake.queue = queue.handle;
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_render_fixture_start_attach(
                                   map, &options, &attach.descriptor, &fixture
                                 )
  );
  mln_test_delivery_queue_release(&queue);
  options.frame_wake.callback = ignore_wake;
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_test_render_fixture_start_attach(
                                map, &options, &attach.descriptor, &fixture
                              )
  );
  mln_test_completion_reject(&attach);
  mln_test_completion_destroy(&attach);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_queued_frame_wake_runs_once_in_dispatch);
  RUN_TEST(attach_checks_a_wake_queue);
}
