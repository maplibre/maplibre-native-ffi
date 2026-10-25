// Raw C ABI coverage for one-shot completions and direct queue wakes.

#include "support/camera.h"
#include "support/frames.h"
#include "support/test_support.h"

typedef struct callback_probe {
  atomic_uint calls;
  atomic_int releases;
  atomic_int status;
} callback_probe;

static void record_completion(
  void* user_data, const mln_completion_result* result
) {
  callback_probe* probe = user_data;
  atomic_store(&probe->status, result->status);
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

  const mln_map_options options = mln_test_map_options();
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_runtime_create_map(runtime, &options, &map, &completion, NULL)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&probe.releases, 1));
  TEST_ASSERT_EQUAL_UINT32(1, atomic_load(&probe.calls));
  MLN_TEST_OK(atomic_load(&probe.status));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_rejected_submission_leaves_callback_state_with_the_caller(void) {
  callback_probe probe = {0};
  mln_completion completion = probed_completion(&probe);
  const mln_map_options options = mln_test_map_options();
  mln_map map = MLN_HANDLE_NULL;

  MLN_TEST_INVALID(
    mln_runtime_create_map(MLN_HANDLE_NULL, &options, &map, &completion, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  TEST_ASSERT_EQUAL_UINT32(0, atomic_load(&probe.calls));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.releases));
}

static void record_wake(void* user_data) {
  atomic_fetch_add((atomic_uint*)user_data, 1);
}

static void runtime_events_wake_the_receiver_directly(void) {
  atomic_uint wakes;
  atomic_init(&wakes, 0);
  const mln_wake event_wake = {
    .callback = record_wake,
    .user_data = &wakes,
  };
  mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, NULL));
  mln_test_configure_events(runtime, event_wake);
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_GREATER_THAN_UINT32(0, atomic_load(&wakes));
  TEST_ASSERT_GREATER_THAN_size_t(0, mln_test_drain_all(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void discarded_commands_publish_state_and_events_before_a_later_query(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_test_drain_all(runtime);

  mln_map_snapshot before = {.size = sizeof(before)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &before, NULL));
  const mln_logical_extent extent = {
    .width = 96, .height = 80, .scale_factor = 1
  };
  mln_camera_update camera = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  mln_test_transition_end end;
  mln_test_transition_end_init(&end);
  camera.animation.end_handler = mln_test_transition_end_handler(&end);
  camera.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.camera.zoom = 4;
  MLN_TEST_OK(mln_map_begin_command_group(map, NULL, NULL));
  MLN_TEST_OK(mln_map_resize(map, extent, NULL, NULL));
  MLN_TEST_OK(
    mln_map_set_debug_options(map, MLN_MAP_DEBUG_TILE_BORDERS, NULL, NULL)
  );
  MLN_TEST_OK(mln_map_update_camera(map, &camera, NULL, NULL));
  MLN_TEST_OK(mln_map_end_command_group(map, NULL, NULL));
  MLN_TEST_OK(mln_map_request_repaint(map, NULL, NULL));
  MLN_TEST_OK(mln_test_map_fence(map));

  TEST_ASSERT_TRUE(mln_test_wait_transition_end(&end));
  mln_map_snapshot after = {.size = sizeof(after)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &after, NULL));
  TEST_ASSERT_GREATER_THAN_UINT64(before.generation, after.generation);
  TEST_ASSERT_EQUAL_UINT32(96, after.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(80, after.view.extent.height);
  TEST_ASSERT_EQUAL_UINT32(MLN_MAP_DEBUG_TILE_BORDERS, after.debug_options);
  mln_camera observed;
  MLN_TEST_OK(mln_test_map_get_camera(map, &observed));
  TEST_ASSERT_EQUAL_DOUBLE(4, observed.zoom);
  TEST_ASSERT_GREATER_THAN_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_later_command_completion_fences_discarded_commands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_map_set_event_mask(map, 0, NULL, NULL));
  MLN_TEST_OK(
    mln_map_set_debug_options(map, MLN_MAP_DEBUG_TILE_BORDERS, NULL, NULL)
  );
  MLN_TEST_AWAIT_OK(mln_map_request_repaint(map, &completion.descriptor, NULL));
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  TEST_ASSERT_EQUAL_UINT64(0, snapshot.event_mask);
  TEST_ASSERT_EQUAL_UINT32(MLN_MAP_DEBUG_TILE_BORDERS, snapshot.debug_options);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void discarded_commands_keep_synchronous_validation(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  MLN_TEST_INVALID(mln_map_set_event_mask(map, UINT64_MAX, NULL, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "mask"));
  MLN_TEST_INVALID(mln_map_resize(map, (mln_logical_extent){0}, NULL, NULL));
  mln_completion undersized = {.size = sizeof(mln_completion) - 1};
  MLN_TEST_INVALID(mln_map_request_repaint(map, &undersized, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void queries_operations_and_lifecycle_require_a_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_INVALID(mln_map_get_camera(map, NULL, NULL));
  MLN_TEST_INVALID(mln_map_get_loaded_style_json(map, NULL, NULL));
  MLN_TEST_INVALID(mln_map_request_still_image(map, NULL, NULL));
  MLN_TEST_INVALID(
    mln_runtime_set_maximum_ambient_cache_size(runtime, 0, NULL, NULL)
  );
  mln_map created = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_runtime_create_map(runtime, NULL, &created, NULL, NULL));
  MLN_TEST_INVALID(mln_map_release(map, NULL, NULL));
  MLN_TEST_INVALID(mln_runtime_release(runtime, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_session_barrier_fences_a_discarded_resize(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_render_session_snapshot before = {.size = sizeof(before)};
  MLN_TEST_OK(mln_render_session_get_snapshot(fixture.session, &before, NULL));
  const mln_logical_extent extent = {
    .width = 96, .height = 80, .scale_factor = 1
  };
  MLN_TEST_OK(mln_render_session_resize(fixture.session, extent, NULL, NULL));
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(80, snapshot.extent.height);
  TEST_ASSERT_EQUAL_UINT64(before.generation + 1, snapshot.generation);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(an_accepted_completion_runs_and_releases_exactly_once);
  RUN_TEST(a_rejected_submission_leaves_callback_state_with_the_caller);
  RUN_TEST(runtime_events_wake_the_receiver_directly);
  RUN_TEST(discarded_commands_publish_state_and_events_before_a_later_query);
  RUN_TEST(a_later_command_completion_fences_discarded_commands);
  RUN_TEST(discarded_commands_keep_synchronous_validation);
  RUN_TEST(queries_operations_and_lifecycle_require_a_completion);
  RUN_TEST(a_session_barrier_fences_a_discarded_resize);
}
