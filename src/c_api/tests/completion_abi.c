// Raw C ABI coverage for one-shot completions and direct queue wakes.

#include <stdatomic.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

typedef struct callback_probe {
  atomic_uint calls;
  atomic_uint releases;
  atomic_int status;
  atomic_uint_fast64_t value;
} callback_probe;

static void record_completion(
  void* user_data, const mln_completion_result* result
) {
  callback_probe* probe = user_data;
  atomic_store(&probe->status, result->status);
  if (result->value != NULL && result->value_count == 1) {
    atomic_store(&probe->value, *(const mln_map*)result->value);
  }
  atomic_fetch_add(&probe->calls, 1);
}

static void record_release(void* user_data) {
  callback_probe* probe = user_data;
  atomic_fetch_add(&probe->releases, 1);
}

static mln_completion probed_completion(callback_probe* probe) {
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = record_completion,
    .user_data = probe,
    .release_user_data = record_release,
  };
}

static void an_accepted_completion_runs_and_releases_exactly_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  callback_probe probe = {0};
  atomic_init(&probe.status, MLN_STATUS_INVALID_STATE);
  mln_completion completion = probed_completion(&probe);

  const mln_map_options options = mln_map_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_create(runtime, &options, &completion)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_UINT32(1, atomic_load(&probe.calls));
  TEST_ASSERT_EQUAL_UINT32(1, atomic_load(&probe.releases));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.status));

  const mln_map map = (mln_map)atomic_load(&probe.value);
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_rejected_submission_leaves_callback_state_with_the_caller(void) {
  callback_probe probe = {0};
  mln_completion completion = probed_completion(&probe);
  const mln_map_options options = mln_map_options_default();

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(MLN_HANDLE_NULL, &options, &completion)
  );
  TEST_ASSERT_EQUAL_UINT32(0, atomic_load(&probe.calls));
  TEST_ASSERT_EQUAL_UINT32(0, atomic_load(&probe.releases));
}

static void completion_state_handles_inline_abandonment_and_races(void) {
  TEST_ASSERT_TRUE(mln_test_completion_contract());
}

static void record_wake(void* user_data) {
  atomic_fetch_add((atomic_uint*)user_data, 1);
}

static void runtime_events_wake_the_receiver_directly(void) {
  atomic_uint wakes;
  atomic_init(&wakes, 0);
  const mln_wake event_wake = {
    .size = sizeof(mln_wake),
    .callback = record_wake,
    .user_data = &wakes,
  };
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake = event_wake;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_create(&options, &runtime));
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(
      map, MLN_BUFFER_LITERAL("{\"version\":8,\"sources\":{},\"layers\":[]}")
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_GREATER_THAN_UINT32(0, atomic_load(&wakes));
  TEST_ASSERT_GREATER_THAN_size_t(0, mln_test_drain_all(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_completion_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(an_accepted_completion_runs_and_releases_exactly_once);
  RUN_TEST(a_rejected_submission_leaves_callback_state_with_the_caller);
  RUN_TEST(completion_state_handles_inline_abandonment_and_races);
  RUN_TEST(runtime_events_wake_the_receiver_directly);
}
