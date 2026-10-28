// Delivery queues: the wake on each empty-to-nonempty transition, concurrent
// dispatch, calls that outlive their producer, release, and handle validation.

#include "support/delivery.h"
#include "support/test_support.h"

// Submits a runtime command whose completion queue dispatches to the call log
// with user_data.
static void submit_queued_command(
  mln_runtime runtime, mln_delivery_queue queue, void* user_data
) {
  const mln_completion completion = mln_test_queue_completion(queue, user_data);
  MLN_TEST_OK(mln_runtime_clear_resource_transform(runtime, &completion, NULL));
}

static void the_wake_fires_once_per_empty_to_nonempty_transition(void) {
  mln_test_calls_reset();
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_completion first =
    mln_test_queue_completion(queue.handle, (void*)1);
  const mln_completion second =
    mln_test_queue_completion(queue.handle, (void*)2);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &first, NULL));
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &second, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&queue.wakes));

  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_EQUAL_size_t(2, mln_test_calls.count);
  TEST_ASSERT_EQUAL_PTR((void*)1, mln_test_call_at(0)->user_data);
  TEST_ASSERT_EQUAL_PTR((void*)2, mln_test_call_at(1)->user_data);
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_delivery_queue_dispatch(queue.handle, NULL)
  );

  // The dispatch re-armed the wake.
  const mln_completion third =
    mln_test_queue_completion(queue.handle, (void*)3);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &third, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(2, atomic_load(&queue.wakes));

  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum { concurrent_completions = 64, concurrent_dispatchers = 4 };

typedef struct concurrent_dispatch {
  mln_delivery_queue queue;
  atomic_int seen[concurrent_completions];
  atomic_int total;
} concurrent_dispatch;

static concurrent_dispatch shared_dispatch;

// Runs on whichever thread's dispatch takes the call.
static void count_concurrent_completion(
  void* user_data, const mln_completion_result* result
) {
  (void)result;
  const uintptr_t slot = (uintptr_t)user_data - 1;
  if (slot < concurrent_completions) {
    atomic_fetch_add(&shared_dispatch.seen[slot], 1);
  }
  atomic_fetch_add(&shared_dispatch.total, 1);
}

static void dispatch_until_all_seen(void* argument) {
  concurrent_dispatch* dispatch = argument;
  const mln_test_deadline deadline = mln_test_deadline_default();
  while (atomic_load(&dispatch->total) < concurrent_completions &&
         !mln_test_deadline_passed(deadline)) {
    (void)mln_delivery_queue_dispatch(dispatch->queue, NULL);
  }
}

// Dispatches that race, each one BUSY while another runs, still run each call
// exactly once.
static void concurrent_dispatches_run_each_call_exactly_once(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  shared_dispatch.queue = queue.handle;
  atomic_store(&shared_dispatch.total, 0);
  for (size_t index = 0; index < concurrent_completions; index += 1) {
    atomic_store(&shared_dispatch.seen[index], 0);
  }

  mln_test_thread* threads[concurrent_dispatchers];
  for (size_t index = 0; index < concurrent_dispatchers; index += 1) {
    threads[index] =
      mln_test_thread_start(dispatch_until_all_seen, &shared_dispatch);
  }
  for (uintptr_t index = 0; index < concurrent_completions; index += 1) {
    const mln_completion completion = {
      .size = sizeof(completion),
      .callback = count_concurrent_completion,
      .user_data = (void*)(index + 1),
      .queue = queue.handle,
    };
    MLN_TEST_OK(
      mln_runtime_clear_resource_transform(runtime, &completion, NULL)
    );
  }
  for (size_t index = 0; index < concurrent_dispatchers; index += 1) {
    mln_test_thread_join(threads[index]);
  }

  TEST_ASSERT_EQUAL_INT(
    concurrent_completions, atomic_load(&shared_dispatch.total)
  );
  for (size_t index = 0; index < concurrent_completions; index += 1) {
    TEST_ASSERT_EQUAL_INT(1, atomic_load(&shared_dispatch.seen[index]));
  }
  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

// A queued call belongs to the queue: releasing the runtime that produced it
// leaves it to run.
static void a_queued_call_outlives_its_runtime(void) {
  mln_test_calls_reset();
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  submit_queued_command(runtime, queue.handle, (void*)7);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));
  mln_test_destroy_runtime(runtime);

  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_calls.count);
  TEST_ASSERT_EQUAL_PTR((void*)7, mln_test_call_at(0)->user_data);
  MLN_TEST_OK(mln_test_call_at(0)->status);
  mln_test_delivery_queue_release(&queue);
}

// A wake that releases its own queue on the first call.
typedef struct releasing_wake {
  mln_delivery_queue queue;
  atomic_int calls;
  atomic_int releases;
} releasing_wake;

static void release_queue_from_wake(void* user_data) {
  releasing_wake* wake = user_data;
  if (atomic_fetch_add(&wake->calls, 1) == 0) {
    mln_delivery_queue_release(wake->queue);
  }
  mln_test_pulse();
}

static void count_releasing_wake_release(void* user_data) {
  releasing_wake* wake = user_data;
  atomic_fetch_add(&wake->releases, 1);
  mln_test_pulse();
}

// Releasing the queue from inside its own wake returns at once, and no later
// push starts a wake call. The wake's user data is released once the call that
// released it returns.
static void releasing_from_the_wake_neither_deadlocks_nor_wakes_again(void) {
  static releasing_wake wake;
  atomic_store(&wake.calls, 0);
  atomic_store(&wake.releases, 0);
  wake.queue = MLN_HANDLE_NULL;
  const mln_delivery_queue_options options = {
    .size = sizeof(mln_delivery_queue_options),
    .wake = {
      .callback = release_queue_from_wake,
      .user_data = &wake,
      .release_user_data = count_releasing_wake_release,
    },
  };
  MLN_TEST_OK(mln_delivery_queue_create(&options, &wake.queue, NULL));
  mln_runtime runtime = mln_test_create_runtime();

  submit_queued_command(runtime, wake.queue, NULL);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&wake.releases, 1));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&wake.calls));

  // A submission that names the released queue is rejected.
  const mln_completion rejected = mln_test_queue_completion(wake.queue, NULL);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_runtime_clear_resource_transform(runtime, &rejected, NULL)
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&wake.calls));
  mln_test_destroy_runtime(runtime);
}

static void handles_report_their_status(void) {
  mln_delivery_queue_release(MLN_HANDLE_NULL);

  mln_delivery_queue unused = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_delivery_queue_create(NULL, &unused, NULL));
  mln_delivery_queue_options options = {.size = 1};
  MLN_TEST_INVALID(mln_delivery_queue_create(&options, &unused, NULL));
  options.size = sizeof(options);
  options.wake.release_user_data = mln_test_delivery_count_wake_release;
  MLN_TEST_INVALID(mln_delivery_queue_create(&options, &unused, NULL));
  options.wake.release_user_data = NULL;
  MLN_TEST_INVALID(mln_delivery_queue_create(&options, NULL, NULL));

  // A queue with a disabled wake waits for the host's own schedule.
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue, NULL));
  mln_delivery_queue occupied = queue;
  MLN_TEST_INVALID(mln_delivery_queue_create(&options, &occupied, NULL));
  mln_delivery_queue_release(queue);
  // Release of a released handle is a no-op.
  mln_delivery_queue_release(queue);
}

MLN_TEST_GROUP {
  RUN_TEST(the_wake_fires_once_per_empty_to_nonempty_transition);
  RUN_TEST(concurrent_dispatches_run_each_call_exactly_once);
  RUN_TEST(a_queued_call_outlives_its_runtime);
  RUN_TEST(releasing_from_the_wake_neither_deadlocks_nor_wakes_again);
  RUN_TEST(handles_report_their_status);
}
