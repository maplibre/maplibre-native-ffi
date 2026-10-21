// Runtime and map lifecycle through the public ABI: creation and its
// validation, stale and foreign handles, and release.

#include "support/resources.h"
#include "support/test_support.h"

static void runtime_options_with_small_size(void* descriptor) {
  ((mln_runtime_options*)descriptor)->size = sizeof(mln_runtime_options) - 1;
}

static const mln_test_validation_case runtime_option_cases[] = {
  {"defaults", NULL, MLN_STATUS_OK, NULL},
  {"undersized", runtime_options_with_small_size, MLN_STATUS_INVALID_ARGUMENT,
   "size"},
};

// A rejected creation leaves the output null; an accepted one is closed at
// once so that the table creates nothing it leaves behind.
static mln_status create_runtime_from(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  (void)context;
  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_status status =
    mln_runtime_create(descriptor, &runtime, diagnostic);
  if (status == MLN_STATUS_OK) {
    MLN_TEST_OK(mln_test_runtime_close(runtime));
  } else {
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  }
  return status;
}

static void runtime_creation_validates_its_options(void) {
  const mln_runtime_options defaults = mln_runtime_options_default();
  mln_test_run_validation_table(
    runtime_option_cases,
    sizeof(runtime_option_cases) / sizeof(*runtime_option_cases), &defaults,
    sizeof(defaults), create_runtime_from, NULL
  );

  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_runtime_create(NULL, &runtime, NULL));
  MLN_TEST_INVALID(mln_runtime_create(&defaults, NULL, NULL));

  // The output must point to the null handle, and a rejection leaves it as is.
  runtime = (mln_runtime)1;
  MLN_TEST_INVALID(mln_runtime_create(&defaults, &runtime, NULL));
  TEST_ASSERT_EQUAL_UINT64(1, runtime);
}

// Every entry point that takes a runtime reports `expected` for `handle`
// without creating anything.
static void expect_runtime_calls_reject(
  mln_runtime handle, mln_status expected
) {
  const mln_completion discard = mln_test_discard_completion();
  const mln_map_options map_options = mln_map_options_default();
  uint64_t mask = 0;
  MLN_TEST_STATUS(expected, mln_runtime_release(handle, &discard, NULL));
  MLN_TEST_STATUS(expected, mln_runtime_dispose(handle, NULL));
  MLN_TEST_STATUS(
    expected, mln_runtime_set_event_handler(handle, NULL, &discard, NULL)
  );
  MLN_TEST_STATUS(expected, mln_runtime_get_event_mask(handle, &mask, NULL));
  MLN_TEST_STATUS(
    expected,
    mln_runtime_set_event_mask(handle, MLN_RUNTIME_EVENT_MASK_ALL, NULL)
  );
  MLN_TEST_STATUS(
    expected, mln_runtime_create_map(handle, &map_options, &discard, NULL)
  );
}

// The null handle and a live handle of another kind name no runtime, so every
// runtime entry point rejects them as invalid arguments. A released runtime
// once named one, so the same entry points report it as invalid state.
static void runtime_calls_reject_a_value_that_names_no_runtime(void) {
  mln_runtime live = mln_test_create_runtime();
  mln_map map = mln_test_create_map(live);
  mln_runtime released = mln_test_create_runtime();
  mln_test_destroy_runtime(released);

  expect_runtime_calls_reject(MLN_HANDLE_NULL, MLN_STATUS_INVALID_ARGUMENT);
  expect_runtime_calls_reject(map, MLN_STATUS_INVALID_ARGUMENT);
  expect_runtime_calls_reject(released, MLN_STATUS_INVALID_STATE);

  // The map named as a runtime is unaffected.
  MLN_TEST_OK(mln_test_map_request_repaint(map));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(live);
}

static void map_options_with_small_size(void* descriptor) {
  ((mln_map_options*)descriptor)->size = sizeof(mln_map_options) - 1;
}

static void map_options_with_unknown_mode(void* descriptor) {
  ((mln_map_options*)descriptor)->map_mode = 999;
}

static const mln_test_validation_case map_creation_cases[] = {
  {"undersized", map_options_with_small_size, MLN_STATUS_INVALID_ARGUMENT,
   "mln_map_options.size"},
  {"unknown map mode", map_options_with_unknown_mode,
   MLN_STATUS_INVALID_ARGUMENT, "map_mode"},
};

static mln_status create_map_from(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_create_map(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  // A rejected submission never runs or releases the completion.
  TEST_ASSERT_NOT_EQUAL_INT(MLN_STATUS_OK, status);
  TEST_ASSERT_FALSE(mln_test_completion_poll(&completion));
  mln_test_completion_reject(&completion);
  mln_test_completion_destroy(&completion);
  return status;
}

static void map_creation_validates_its_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_map_options defaults = mln_map_options_default();
  mln_test_run_validation_table(
    map_creation_cases,
    sizeof(map_creation_cases) / sizeof(*map_creation_cases), &defaults,
    sizeof(defaults), create_map_from, &runtime
  );
  mln_test_destroy_runtime(runtime);
}

// Every map entry point below reports a released map as invalid state without
// running its completion.
static void a_released_map_accepts_no_call(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_destroy_map(map);
  MLN_TEST_INVALID_STATE(mln_test_map_close(map));
  MLN_TEST_INVALID_STATE(
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL("{}"))
  );
  MLN_TEST_INVALID_STATE(mln_test_map_request_repaint(map));
  mln_completion completion = mln_test_discard_completion();
  MLN_TEST_INVALID_STATE(mln_map_request_still_image(map, &completion, NULL));
  mln_camera_options camera = mln_camera_options_default();
  MLN_TEST_INVALID_STATE(mln_test_map_get_camera(map, &camera));
  MLN_TEST_INVALID_STATE(mln_map_dispose(map, NULL));
  mln_test_destroy_runtime(runtime);
}

static void style_functions_reject_null_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_INVALID(mln_test_map_set_style_json(map, (mln_buffer_view){0}));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "style JSON must not be empty"),
    mln_test_last_error()
  );

  // A rejected style read never invokes or releases the completion, so the
  // caller still owns the user_data and releases it itself.
  mln_test_completion held = mln_test_completion_buffer_view();
  MLN_TEST_INVALID(mln_map_get_style_url(map, NULL, NULL));
  MLN_TEST_INVALID(
    mln_map_get_loaded_style_json(MLN_HANDLE_NULL, &held.descriptor, NULL)
  );
  MLN_TEST_INVALID(
    mln_map_get_style_url(MLN_HANDLE_NULL, &held.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(mln_test_completion_poll(&held));
  mln_test_completion_reject(&held);
  mln_test_completion_destroy(&held);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void close_preflight_leaves_a_runtime_with_a_live_child_open(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_completion discard = mln_test_discard_completion();
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_runtime_release(runtime, &discard, NULL)
  );

  uint64_t mask = 0;
  MLN_TEST_OK(mln_runtime_get_event_mask(runtime, &mask, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct creator_thread_probe {
  mln_runtime runtime;
  mln_map map;
  mln_status status;
} creator_thread_probe;

static void create_on_temporary_thread(void* argument) {
  creator_thread_probe* probe = argument;
  const mln_runtime_options options = mln_runtime_options_default();
  probe->status = mln_runtime_create(&options, &probe->runtime, NULL);
  if (probe->status != MLN_STATUS_OK) return;

  mln_test_completion create_map = mln_test_completion_default(sizeof(mln_map));
  probe->status =
    mln_runtime_create_map(probe->runtime, NULL, &create_map.descriptor, NULL);
  if (probe->status == MLN_STATUS_OK) {
    probe->status = mln_test_completion_finish_value(
      &create_map, &probe->map, sizeof(probe->map)
    );
  } else {
    mln_test_completion_reject(&create_map);
    mln_test_completion_destroy(&create_map);
  }
  if (probe->status != MLN_STATUS_OK) {
    (void)mln_test_runtime_close(probe->runtime);
  }
}

static void runtime_and_map_outlive_the_creating_host_thread(void) {
  creator_thread_probe probe = {.status = MLN_STATUS_NATIVE_ERROR};
  mln_test_thread_join(
    mln_test_thread_start(create_on_temporary_thread, &probe)
  );
  MLN_TEST_OK(probe.status);

  const mln_logical_extent extent = {320, 240, 1.0};
  mln_test_completion resize = mln_test_completion_default(0);
  mln_status resize_status =
    mln_map_resize(probe.map, extent, &resize.descriptor, NULL);
  if (resize_status == MLN_STATUS_OK) {
    resize_status = mln_test_completion_settle(&resize);
  } else {
    mln_test_completion_reject(&resize);
    mln_test_completion_destroy(&resize);
  }
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  const mln_status snapshot_status =
    mln_map_get_snapshot(probe.map, &snapshot, NULL);
  const mln_status map_close_status = mln_test_map_close(probe.map);
  const mln_status runtime_close_status = mln_test_runtime_close(probe.runtime);

  MLN_TEST_OK(resize_status);
  MLN_TEST_OK(snapshot_status);
  TEST_ASSERT_EQUAL_UINT32(extent.width, snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(extent.height, snapshot.view.extent.height);
  MLN_TEST_OK(map_close_status);
  MLN_TEST_OK(runtime_close_status);
}

typedef struct close_probe {
  mln_runtime runtime;
  atomic_int status;
} close_probe;

static mln_runtime create_untracked_runtime(void) {
  const mln_runtime_options options = mln_runtime_options_default();
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  return runtime;
}

static void close_from_foreign_thread(void* argument) {
  close_probe* probe = argument;
  const mln_completion discard = mln_test_discard_completion();
  atomic_store(
    &probe->status, mln_runtime_release(probe->runtime, &discard, NULL)
  );
}

static void accepted_close_is_any_thread_and_retires_the_handle(void) {
  mln_runtime runtime = create_untracked_runtime();
  close_probe probe = {
    .runtime = runtime,
  };
  atomic_init(&probe.status, MLN_STATUS_NATIVE_ERROR);
  mln_test_thread* thread =
    mln_test_thread_start(close_from_foreign_thread, &probe);
  mln_test_thread_join(thread);
  MLN_TEST_OK(atomic_load(&probe.status));

  uint64_t mask = 0;
  MLN_TEST_INVALID_STATE(mln_runtime_get_event_mask(runtime, &mask, NULL));
}

static mln_resource_provider_decision pass_resource(
  void* context, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)context;
  (void)request;
  (void)handle;
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

static void disposal_release(void* user_data) {
  mln_test_flag_set((atomic_bool*)user_data);
}

static void disposal_waits_for_children_and_retires_callback_state(void) {
  atomic_bool released;
  atomic_init(&released, false);
  mln_runtime_options options = mln_runtime_options_default();
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  const mln_resource_provider provider = {
    .size = sizeof(provider),
    .callback = pass_resource,
    .user_data = &released,
    .release_user_data = disposal_release,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  MLN_TEST_OK(mln_runtime_dispose(runtime, NULL));
  uint64_t mask = 0;
  MLN_TEST_INVALID_STATE(mln_runtime_get_event_mask(runtime, &mask, NULL));
  TEST_ASSERT_FALSE(atomic_load(&released));
  MLN_TEST_OK(mln_map_dispose(map, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&released));
}

typedef struct retirement_wait {
  atomic_bool entered;
  atomic_bool other_released;
  atomic_bool observed_other;
} retirement_wait;

static void wait_for_other_retirement(void* user_data) {
  retirement_wait* wait = user_data;
  mln_test_flag_set(&wait->entered);
  atomic_store(
    &wait->observed_other, mln_test_wait_for_flag(&wait->other_released)
  );
}

typedef struct retirement_result {
  atomic_int status;
  atomic_bool released;
} retirement_result;

static void record_retirement(
  void* user_data, const mln_completion_result* result
) {
  retirement_result* probe = user_data;
  atomic_store(&probe->status, result->status);
}

static void release_retirement(void* user_data) {
  retirement_result* result = user_data;
  mln_test_flag_set(&result->released);
}

static void complete_other_retirement(void* user_data) {
  mln_test_flag_set(&((retirement_wait*)user_data)->other_released);
}

static void runtime_state_release_can_wait_for_another_runtime(void) {
  retirement_wait wait = {0};
  retirement_result result = {.status = MLN_STATUS_NATIVE_ERROR};
  mln_runtime_options options = mln_runtime_options_default();
  mln_runtime waiting = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &waiting, NULL));
  const mln_resource_provider provider = {
    .size = sizeof(provider),
    .callback = pass_resource,
    .user_data = &wait,
    .release_user_data = wait_for_other_retirement,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(waiting, &provider));
  const mln_runtime other = create_untracked_runtime();
  const mln_completion waiting_close = {
    .size = sizeof(waiting_close),
    .callback = record_retirement,
    .user_data = &result,
    .release_user_data = release_retirement,
  };
  MLN_TEST_OK(mln_runtime_release(waiting, &waiting_close, NULL));
  const bool entered = mln_test_wait_for_flag(&wait.entered);
  mln_completion other_close = mln_test_discard_completion();
  other_close.user_data = &wait;
  other_close.release_user_data = complete_other_retirement;
  const mln_status other_status =
    mln_runtime_release(other, &other_close, NULL);
  const bool other_released = mln_test_wait_for_flag(&wait.other_released);
  const bool waiting_released = mln_test_wait_for_flag(&result.released);
  TEST_ASSERT_TRUE(entered);
  MLN_TEST_OK(other_status);
  TEST_ASSERT_TRUE(other_released);
  TEST_ASSERT_TRUE(waiting_released);
  MLN_TEST_OK(atomic_load(&result.status));
  TEST_ASSERT_TRUE(atomic_load(&wait.observed_other));
}

typedef struct release_parent {
  mln_runtime runtime;
  const mln_completion* completion;
  atomic_int map_status;
  atomic_int submission_status;
} release_parent;

static void release_runtime_from_map_completion(
  void* user_data, const mln_completion_result* result
) {
  release_parent* parent = user_data;
  atomic_store(&parent->map_status, result->status);
  atomic_store(
    &parent->submission_status,
    mln_runtime_release(parent->runtime, parent->completion, NULL)
  );
}

static void a_map_release_completion_can_release_its_runtime(void) {
  const mln_runtime runtime = create_untracked_runtime();
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  mln_test_completion runtime_close = mln_test_completion_default(0);
  release_parent parent = {
    .runtime = runtime,
    .completion = &runtime_close.descriptor,
    .map_status = MLN_STATUS_NATIVE_ERROR,
    .submission_status = MLN_STATUS_NATIVE_ERROR,
  };
  const mln_completion map_close = {
    .size = sizeof(map_close),
    .callback = release_runtime_from_map_completion,
    .user_data = &parent,
  };
  MLN_TEST_OK(mln_map_release(map, &map_close, NULL));
  MLN_TEST_OK(mln_test_completion_settle(&runtime_close));
  MLN_TEST_OK(atomic_load(&parent.map_status));
  MLN_TEST_OK(atomic_load(&parent.submission_status));
}

MLN_TEST_GROUP {
  RUN_TEST(runtime_creation_validates_its_options);
  RUN_TEST(runtime_calls_reject_a_value_that_names_no_runtime);
  RUN_TEST(map_creation_validates_its_options);
  RUN_TEST(a_released_map_accepts_no_call);
  RUN_TEST(style_functions_reject_null_inputs);
  RUN_TEST(disposal_waits_for_children_and_retires_callback_state);
  RUN_TEST(close_preflight_leaves_a_runtime_with_a_live_child_open);
  RUN_TEST(accepted_close_is_any_thread_and_retires_the_handle);
  RUN_TEST(runtime_and_map_outlive_the_creating_host_thread);
  RUN_TEST(runtime_state_release_can_wait_for_another_runtime);
  RUN_TEST(a_map_release_completion_can_release_its_runtime);
}
