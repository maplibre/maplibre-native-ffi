// A queue's release_abandoned_user_data: releasing the queue passes it the
// user_data of each registration whose release_user_data, or whose completion
// callback, the release abandons, once each and on the releasing thread, and
// never one whose release ran or will run.

#include "support/delivery.h"
#include "support/resources.h"
#include "support/test_support.h"

// Set on the thread that runs a case, which is the only one that releases.
static MLN_TEST_THREAD_LOCAL bool on_case_thread;
static atomic_int off_thread_reports;

// Records 'a' with the abandoned user_data.
static void record_abandoned(void* user_data) {
  if (!on_case_thread) {
    atomic_fetch_add(&off_thread_reports, 1);
    return;
  }
  mln_test_record_call('a', user_data, MLN_STATUS_OK, 0);
}

static void start_case(void) {
  mln_test_calls_reset();
  atomic_store(&off_thread_reports, 0);
  on_case_thread = true;
}

static void record_transform(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_resource_transform_request request
) {
  (void)kind;
  (void)url;
  (void)request;
  mln_test_record_call('t', user_data, MLN_STATUS_OK, 0);
}

static void record_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)request;
  (void)handle;
  mln_test_record_call('p', user_data, MLN_STATUS_OK, 0);
}

static void record_events(void* user_data, const mln_event_batch_view* events) {
  (void)events;
  mln_test_record_call('e', user_data, MLN_STATUS_OK, 0);
}

static mln_resource_transform queued_transform(
  mln_delivery_queue queue, void* user_data
) {
  return (mln_resource_transform){
    .size = sizeof(mln_resource_transform),
    .callback = record_transform,
    .user_data = user_data,
    .release_user_data = mln_test_record_release,
    .queue = queue,
  };
}

typedef struct release_wait {
  mln_delivery_queue queue;
  void* user_data;
  mln_status failure;
} release_wait;

static bool dispatched_release(void* context) {
  release_wait* wait = context;
  const mln_status status = mln_delivery_queue_dispatch(wait->queue, NULL);
  if (status != MLN_STATUS_OK && status != MLN_STATUS_NOT_READY) {
    wait->failure = status;
    return true;
  }
  return mln_test_count_calls('r', wait->user_data) != 0;
}

// Dispatches queue as its wake pulses until user_data's release has run.
static void dispatch_until_released(mln_delivery_queue queue, void* user_data) {
  release_wait wait = {
    .queue = queue, .user_data = user_data, .failure = MLN_STATUS_OK
  };
  TEST_ASSERT_TRUE(mln_test_await(
    dispatched_release, &wait, mln_test_deadline_default(), "a release"
  ));
  MLN_TEST_OK(wait.failure);
}

// A live provider with a queued request, a retired transform whose release is
// queued, a delivered completion, and a completion still in flight are each
// reported once. A transform whose release already ran, and an event handler
// without release_user_data, are not. Retiring or completing them after the
// release reports nothing more.
static void release_reports_each_abandoned_user_data_once(void) {
  start_case();
  static char live, retired, queued_release, delivered, pending, unreleased;
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create_abandoning(&queue, record_abandoned);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_resource_transform first = queued_transform(queue.handle, &retired);
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &first));
  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  dispatch_until_released(queue.handle, &retired);

  // The queue is empty, so the request's push wakes it.
  const int wakes = atomic_load(&queue.wakes);
  const mln_resource_provider provider = {
    .size = sizeof(provider),
    .callback = record_request,
    .user_data = &live,
    .release_user_data = mln_test_record_release,
    .queue = queue.handle,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://abandoned.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, wakes + 1));

  const mln_resource_transform second =
    queued_transform(queue.handle, &queued_release);
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &second));
  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));

  const mln_completion completion =
    mln_test_queue_completion(queue.handle, &delivered);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &completion, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));

  const mln_runtime_event_handler handler = {
    .size = sizeof(handler),
    .callback = record_events,
    .user_data = &unreleased,
    .queue = queue.handle,
  };
  MLN_TEST_OK(mln_runtime_set_event_handler(runtime, &handler, NULL, NULL));

  // The parked worker holds the queued completion behind the gate.
  mln_map parking = mln_test_create_map(runtime);
  mln_test_gate gate;
  mln_test_gate_init(&gate);
  const mln_completion parked = mln_test_gate_completion(&gate);
  MLN_TEST_OK(mln_map_set_debug_options(parking, 0, &parked, NULL));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(&gate), "the runtime worker never parked"
  );
  const mln_completion in_flight =
    mln_test_queue_completion(queue.handle, &pending);
  MLN_TEST_OK(mln_map_get_style_url(parking, &in_flight, NULL));

  const size_t before = mln_test_calls.count;
  mln_test_delivery_queue_release(&queue);
  TEST_ASSERT_EQUAL_size_t(before + 4, mln_test_calls.count);
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('a', &live));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('a', &queued_release));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('a', &delivered));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('a', &pending));

  mln_test_gate_release(&gate);
  MLN_TEST_OK(mln_test_map_fence(parking));
  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  MLN_TEST_OK(mln_runtime_clear_event_handler(runtime, NULL, NULL));
  mln_test_destroy_map(parking);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  TEST_ASSERT_EQUAL_size_t(before + 4, mln_test_calls.count);
  TEST_ASSERT_EQUAL_size_t(0, mln_test_count_calls('a', &retired));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('r', &retired));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_count_calls('a', &unreleased));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&off_thread_reports));
}

typedef struct releasing_call {
  mln_delivery_queue queue;
} releasing_call;

static void release_the_queue(
  void* user_data, const mln_completion_result* result
) {
  mln_test_record_completion(user_data, result);
  releasing_call* call = user_data;
  mln_delivery_queue_release(call->queue);
}

// A call that releases its queue still runs its own release, and the release
// reports the calls queued after it, on the dispatching thread.
static void a_release_inside_dispatch_reports_the_calls_after_it(void) {
  start_case();
  static releasing_call first;
  static char second;
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create_abandoning(&queue, record_abandoned);
  first.queue = queue.handle;
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_completion releasing = {
    .size = sizeof(mln_completion),
    .callback = release_the_queue,
    .user_data = &first,
    .release_user_data = mln_test_record_release,
    .queue = queue.handle,
  };
  const mln_completion later = {
    .size = sizeof(mln_completion),
    .callback = mln_test_record_completion,
    .user_data = &second,
    .release_user_data = mln_test_record_release,
    .queue = queue.handle,
  };
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &releasing, NULL));
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &later, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));

  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wake_releases, 1));
  TEST_ASSERT_EQUAL_size_t(3, mln_test_calls.count);
  TEST_ASSERT_EQUAL_CHAR('c', mln_test_call_at(0)->kind);
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('a', &second));
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('r', &first));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_count_calls('a', &first));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&off_thread_reports));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(release_reports_each_abandoned_user_data_once);
  RUN_TEST(a_release_inside_dispatch_reports_the_calls_after_it);
}
