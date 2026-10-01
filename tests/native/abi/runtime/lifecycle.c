// Runtime and map lifecycle through the public ABI: creation and its
// validation, stale and foreign handles, ordered barriers, and release.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void runtime_creation_returns_a_runtime(void) {
  const mln_runtime_options options = mln_runtime_options_default();

  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  MLN_TEST_INVALID(mln_runtime_create(&options, &runtime, NULL));

  MLN_TEST_OK(mln_test_runtime_close(runtime));
}

static void ignore_wake(void* user_data) { (void)user_data; }

static void runtime_options_with_small_size(void* descriptor) {
  ((mln_runtime_options*)descriptor)->size = sizeof(mln_runtime_options) - 1;
}

static void runtime_options_with_unknown_flag(void* descriptor) {
  ((mln_runtime_options*)descriptor)->flags = UINT32_C(1) << 31U;
}

static void runtime_options_with_unknown_event_bit(void* descriptor) {
  ((mln_runtime_options*)descriptor)->event_mask |= UINT64_C(1) << 40U;
}

static void runtime_options_with_small_wake(void* descriptor) {
  mln_runtime_options* options = descriptor;
  options->event_wake.callback = ignore_wake;
  options->event_wake.size = sizeof(mln_wake) - 1;
}

static void runtime_options_with_retaining_disabled_wake(void* descriptor) {
  mln_runtime_options* options = descriptor;
  options->event_wake.callback = NULL;
  options->event_wake.release_user_data = ignore_wake;
}

static const mln_test_validation_case runtime_option_cases[] = {
  {"defaults", NULL, MLN_STATUS_OK, NULL},
  {"undersized", runtime_options_with_small_size, MLN_STATUS_INVALID_ARGUMENT,
   "size"},
  {"unknown flag", runtime_options_with_unknown_flag,
   MLN_STATUS_INVALID_ARGUMENT, "flags"},
  {"unknown event bit", runtime_options_with_unknown_event_bit,
   MLN_STATUS_INVALID_ARGUMENT, "event_mask"},
  {"undersized wake", runtime_options_with_small_wake,
   MLN_STATUS_INVALID_ARGUMENT, "mln_wake.size"},
  {"disabled wake that retains user data",
   runtime_options_with_retaining_disabled_wake, MLN_STATUS_INVALID_ARGUMENT,
   "disabled wake"},
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

// Every runtime entry point that takes a handle, called with one.
typedef mln_status (*runtime_call)(mln_runtime runtime);

static mln_status call_release(mln_runtime runtime) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_runtime_release(runtime, &discard, NULL);
}

static mln_status call_barrier(mln_runtime runtime) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_runtime_barrier(runtime, &discard, NULL);
}

static mln_status call_dispose(mln_runtime runtime) {
  return mln_runtime_dispose(runtime, NULL);
}

static mln_status call_drain(mln_runtime runtime) {
  mln_event_batch batch = MLN_HANDLE_NULL;
  const mln_status status = mln_runtime_drain_events(runtime, &batch, NULL);
  mln_event_batch_release(batch);
  return status;
}

static mln_status call_get_event_mask(mln_runtime runtime) {
  uint64_t mask = 0;
  return mln_runtime_get_event_mask(runtime, &mask, NULL);
}

static mln_status call_set_event_mask(mln_runtime runtime) {
  return mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_ALL, NULL);
}

static mln_status call_create_map(mln_runtime runtime) {
  const mln_map_options options = mln_map_options_default();
  const mln_completion discard = mln_test_discard_completion();
  return mln_map_create(runtime, &options, &discard, NULL);
}

static const struct {
  const char* label;
  runtime_call call;
} runtime_calls[] = {
  {"release", call_release},
  {"barrier", call_barrier},
  {"dispose", call_dispose},
  {"drain", call_drain},
  {"get event mask", call_get_event_mask},
  {"set event mask", call_set_event_mask},
  {"create map", call_create_map},
};

// The null handle, a released runtime, and a live handle of another kind each
// name no runtime, and every entry point rejects them the same way.
static void runtime_calls_reject_a_value_that_names_no_runtime(void) {
  mln_runtime live = mln_test_create_runtime();
  mln_map map = mln_test_create_map(live);
  mln_runtime released = mln_test_create_runtime();
  mln_test_destroy_runtime(released);

  const mln_runtime handles[] = {MLN_HANDLE_NULL, released, map};
  const size_t call_count = sizeof(runtime_calls) / sizeof(*runtime_calls);
  for (size_t handle = 0; handle < sizeof(handles) / sizeof(*handles);
       handle += 1) {
    for (size_t call = 0; call < call_count; call += 1) {
      TEST_ASSERT_EQUAL_INT_MESSAGE(
        MLN_STATUS_INVALID_ARGUMENT, runtime_calls[call].call(handles[handle]),
        runtime_calls[call].label
      );
    }
  }

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
  const mln_status status = mln_map_create(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  // A rejected submission never runs or releases the completion.
  TEST_ASSERT_NOT_EQUAL_INT(MLN_STATUS_OK, status);
  TEST_ASSERT_FALSE(mln_test_completion_poll(&completion));
  mln_test_completion_reject(&completion);
  mln_test_completion_destroy(&completion);
  return status;
}

static void map_creation_validates_its_options_and_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_map_options defaults = mln_map_options_default();
  mln_test_run_validation_table(
    map_creation_cases,
    sizeof(map_creation_cases) / sizeof(*map_creation_cases), &defaults,
    sizeof(defaults), create_map_from, &runtime
  );
  MLN_TEST_INVALID(mln_map_create(runtime, &defaults, NULL, NULL));
  mln_test_destroy_runtime(runtime);
}

// Every map entry point below rejects a released map without running its
// completion.
static void a_released_map_accepts_no_call(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_destroy_map(map);
  MLN_TEST_INVALID(mln_test_map_close(map));
  MLN_TEST_INVALID(mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL("{}")));
  MLN_TEST_INVALID(mln_test_map_request_repaint(map));
  mln_completion completion = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_request_still_image(map, &completion, NULL));
  mln_camera_options camera = mln_camera_options_default();
  MLN_TEST_INVALID(mln_test_map_get_camera(map, &camera));
  MLN_TEST_INVALID(mln_map_dispose(map, NULL));
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
  MLN_TEST_INVALID(mln_test_map_set_style_url(map, NULL));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "url must not be null"), mln_test_last_error()
  );

  // A rejected style read never invokes or releases the completion, so the
  // caller still owns the user_data and releases it itself.
  mln_test_completion held = mln_test_completion_buffer_view();
  MLN_TEST_INVALID(mln_map_loaded_style_json(map, NULL, NULL));
  MLN_TEST_INVALID(mln_map_style_url(map, NULL, NULL));
  MLN_TEST_INVALID(
    mln_map_loaded_style_json(MLN_HANDLE_NULL, &held.descriptor, NULL)
  );
  MLN_TEST_INVALID(mln_map_style_url(MLN_HANDLE_NULL, &held.descriptor, NULL));
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

// A barrier waits for earlier work that is still pending. A static map with no
// render session holds a still image until the map closes, and the resize
// after the barrier is the fence: once it completes, the barrier has been
// admitted and would have completed already if it ignored the still image.
static void a_barrier_completes_after_preceding_work(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, NULL));
  mln_test_completion barrier = mln_test_completion_default(0);
  MLN_TEST_OK(mln_runtime_barrier(runtime, &barrier.descriptor, NULL));
  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_extent){128, 128, 1.0}, &completion.descriptor, NULL
  ));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&barrier));

  // Closing the map cancels the still image, which lets the barrier finish.
  mln_test_destroy_map(map);
  MLN_TEST_OK(mln_test_completion_finish(&barrier));
  TEST_ASSERT_TRUE(mln_test_completion_poll(&still));
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, mln_test_completion_status(&still));
  mln_test_completion_destroy(&barrier);
  mln_test_completion_destroy(&still);
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
    mln_map_create(probe->runtime, NULL, &create_map.descriptor, NULL);
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
  const mln_status barrier_status = mln_test_runtime_barrier(probe.runtime);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  const mln_status snapshot_status =
    mln_map_snapshot_get(probe.map, &snapshot, NULL);
  const mln_status map_close_status = mln_test_map_close(probe.map);
  const mln_status runtime_close_status = mln_test_runtime_close(probe.runtime);

  MLN_TEST_OK(resize_status);
  MLN_TEST_OK(barrier_status);
  MLN_TEST_OK(snapshot_status);
  TEST_ASSERT_EQUAL_UINT32(extent.width, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(extent.height, snapshot.logical_extent.height);
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
  MLN_TEST_INVALID(mln_runtime_get_event_mask(runtime, &mask, NULL));
}

static void disposal_wake(void* user_data) { (void)user_data; }

static void disposal_release(void* user_data) {
  mln_test_flag_set((atomic_bool*)user_data);
}

static void disposal_waits_for_children_and_retires_callback_state(void) {
  atomic_bool released;
  atomic_init(&released, false);
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake.callback = disposal_wake;
  options.event_wake.user_data = &released;
  options.event_wake.release_user_data = disposal_release;
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  MLN_TEST_OK(mln_runtime_dispose(runtime, NULL));
  uint64_t mask = 0;
  MLN_TEST_INVALID(mln_runtime_get_event_mask(runtime, &mask, NULL));
  TEST_ASSERT_FALSE(atomic_load(&released));
  MLN_TEST_OK(mln_map_dispose(map, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&released));
}

MLN_TEST_GROUP {
  RUN_TEST(runtime_creation_returns_a_runtime);
  RUN_TEST(runtime_creation_validates_its_options);
  RUN_TEST(runtime_calls_reject_a_value_that_names_no_runtime);
  RUN_TEST(map_creation_validates_its_options_and_completion);
  RUN_TEST(a_released_map_accepts_no_call);
  RUN_TEST(style_functions_reject_null_inputs);
  RUN_TEST(disposal_waits_for_children_and_retires_callback_state);
  RUN_TEST(close_preflight_leaves_a_runtime_with_a_live_child_open);
  RUN_TEST(a_barrier_completes_after_preceding_work);
  RUN_TEST(accepted_close_is_any_thread_and_retires_the_handle);
  RUN_TEST(runtime_and_map_outlive_the_creating_host_thread);
}
