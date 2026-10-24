// Delivery queues: the wake on each empty-to-nonempty transition, draining,
// batch lifetimes, release, and handle validation.

#include "support/delivery.h"
#include "support/test_support.h"

// Submits a runtime command whose completion goes to queue with user_data.
static void submit_queued_command(
  mln_runtime runtime, mln_delivery_queue queue, void* user_data
) {
  const mln_completion completion = mln_test_queue_completion(queue, user_data);
  MLN_TEST_OK(mln_runtime_clear_resource_transform(runtime, &completion, NULL));
}

static void the_wake_fires_once_per_empty_to_nonempty_transition(void) {
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

  mln_test_delivery_drain drain;
  MLN_TEST_OK(mln_test_delivery_drain_into(queue.handle, &drain));
  TEST_ASSERT_EQUAL_size_t(2, drain.view.delivery_count);
  TEST_ASSERT_EQUAL_PTR(
    (void*)1, mln_test_delivery_at(&drain.view, 0)->user_data
  );
  TEST_ASSERT_EQUAL_PTR(
    (void*)2, mln_test_delivery_at(&drain.view, 1)->user_data
  );
  mln_delivery_batch_release(drain.batch);

  // An empty queue reports NOT_READY and leaves the outputs alone.
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_test_delivery_drain_into(queue.handle, &drain)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, drain.batch);
  TEST_ASSERT_NULL(drain.view.deliveries);

  // The drain re-armed the wake.
  const mln_completion third =
    mln_test_queue_completion(queue.handle, (void*)3);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &third, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(2, atomic_load(&queue.wakes));

  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum { concurrent_completions = 64, concurrent_drainers = 4 };

typedef struct concurrent_drain {
  mln_delivery_queue queue;
  atomic_int seen[concurrent_completions];
  atomic_int total;
} concurrent_drain;

static void drain_until_all_seen(void* argument) {
  concurrent_drain* drain = argument;
  const mln_test_deadline deadline = mln_test_deadline_default();
  while (atomic_load(&drain->total) < concurrent_completions &&
         !mln_test_deadline_passed(deadline)) {
    mln_delivery_batch batch = MLN_HANDLE_NULL;
    mln_delivery_batch_view view = {.size = sizeof(view)};
    const mln_status status =
      mln_delivery_queue_drain(drain->queue, &batch, &view, NULL);
    if (status != MLN_STATUS_OK) continue;
    for (size_t index = 0; index < view.delivery_count; index += 1) {
      const mln_delivery* delivery =
        (const mln_delivery*)((const unsigned char*)view.deliveries +
                              index * view.delivery_size);
      const uintptr_t slot = (uintptr_t)delivery->user_data - 1;
      if (slot < concurrent_completions) {
        atomic_fetch_add(&drain->seen[slot], 1);
      }
      atomic_fetch_add(&drain->total, 1);
    }
    mln_delivery_batch_release(batch);
  }
}

static void concurrent_drains_deliver_each_item_exactly_once(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  static concurrent_drain drain;
  drain.queue = queue.handle;
  atomic_store(&drain.total, 0);
  for (size_t index = 0; index < concurrent_completions; index += 1) {
    atomic_store(&drain.seen[index], 0);
  }

  mln_test_thread* threads[concurrent_drainers];
  for (size_t index = 0; index < concurrent_drainers; index += 1) {
    threads[index] = mln_test_thread_start(drain_until_all_seen, &drain);
  }
  for (uintptr_t index = 0; index < concurrent_completions; index += 1) {
    submit_queued_command(runtime, queue.handle, (void*)(index + 1));
  }
  for (size_t index = 0; index < concurrent_drainers; index += 1) {
    mln_test_thread_join(threads[index]);
  }

  TEST_ASSERT_EQUAL_INT(concurrent_completions, atomic_load(&drain.total));
  for (size_t index = 0; index < concurrent_completions; index += 1) {
    TEST_ASSERT_EQUAL_INT(1, atomic_load(&drain.seen[index]));
  }
  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

// A drained batch belongs to the host: releasing the queue and the runtime
// leaves it readable.
static void a_batch_outlives_its_queue_and_runtime(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  submit_queued_command(runtime, queue.handle, (void*)7);
  const mln_delivery* delivery = NULL;
  const mln_delivery_batch batch =
    mln_test_delivery_await_single(queue.handle, &delivery);

  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);

  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_COMPLETION, delivery->type);
  TEST_ASSERT_EQUAL_PTR((void*)7, delivery->user_data);
  MLN_TEST_OK(delivery->payload.completion.status);
  mln_delivery_batch_release(batch);
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
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_runtime_clear_resource_transform(
      runtime,
      &(mln_completion){.size = sizeof(mln_completion), .queue = wake.queue},
      NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&wake.calls));
  mln_test_destroy_runtime(runtime);
}

static void handles_report_their_status(void) {
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  mln_delivery_batch_view view = {.size = sizeof(view)};
  MLN_TEST_INVALID(
    mln_delivery_queue_drain(MLN_HANDLE_NULL, &batch, &view, NULL)
  );
  mln_delivery_queue_release(MLN_HANDLE_NULL);
  mln_delivery_batch_release(MLN_HANDLE_NULL);

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
  MLN_TEST_INVALID(mln_delivery_queue_drain(queue, NULL, &view, NULL));
  MLN_TEST_INVALID(mln_delivery_queue_drain(queue, &batch, NULL, NULL));
  view.size = 1;
  MLN_TEST_INVALID(mln_delivery_queue_drain(queue, &batch, &view, NULL));
  view.size = sizeof(view);
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY, mln_delivery_queue_drain(queue, &batch, &view, NULL)
  );
  mln_delivery_queue_release(queue);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_delivery_queue_drain(queue, &batch, &view, NULL)
  );
  // Release of a released handle is a no-op.
  mln_delivery_queue_release(queue);
  // A handle of another type is invalid.
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_INVALID(mln_delivery_queue_drain(runtime, &batch, &view, NULL));
  mln_test_destroy_runtime(runtime);
}

static void take_reports_its_status(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  submit_queued_command(runtime, queue.handle, NULL);
  const mln_delivery* delivery = NULL;
  mln_delivery_batch batch =
    mln_test_delivery_await_single(queue.handle, &delivery);
  // A delivery that owns nothing takes as often as the host asks.
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  MLN_TEST_INVALID(mln_delivery_batch_take(batch, 1, NULL));
  mln_delivery_batch_release(batch);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_delivery_batch_take(batch, 0, NULL)
  );

  mln_test_delivery_queue_release(&queue);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(the_wake_fires_once_per_empty_to_nonempty_transition);
  RUN_TEST(concurrent_drains_deliver_each_item_exactly_once);
  RUN_TEST(a_batch_outlives_its_queue_and_runtime);
  RUN_TEST(releasing_from_the_wake_neither_deadlocks_nor_wakes_again);
  RUN_TEST(handles_report_their_status);
  RUN_TEST(take_reports_its_status);
}
