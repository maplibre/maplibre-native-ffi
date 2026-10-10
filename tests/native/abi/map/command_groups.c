// Render updates per command: each command publishes at most one, and a
// command group holds every update until its outermost end publishes the
// latest one. Without a render session nothing else publishes an update, so
// the snapshot's render-update generation counts exactly what commands
// published.

#include "support/style.h"
#include "support/test_support.h"

static uint64_t render_update_generation(mln_map map) {
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return snapshot.latest_render_update_generation;
}

static mln_map create_loaded_map(mln_runtime runtime) {
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  return map;
}

static void add_background_layer(mln_map map, const char* layer_json) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_test_view_of(layer_json), (mln_buffer_view){0},
    &completion.descriptor, NULL
  ));
}

static bool layer_exists(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_layer_info));
  MLN_TEST_OK(mln_map_get_style_layer(
    map, mln_test_view_of(id), &completion.descriptor, NULL
  ));
  mln_style_layer_info result;
  bool found = false;
  MLN_TEST_OK(mln_test_completion_finish_optional(
    &completion, &result, sizeof(result), &found
  ));
  return found;
}

static void begin_group(mln_map map) {
  MLN_TEST_AWAIT_OK(
    mln_map_begin_command_group(map, &completion.descriptor, NULL)
  );
}

static void end_group(mln_map map) {
  MLN_TEST_AWAIT_OK(
    mln_map_end_command_group(map, &completion.descriptor, NULL)
  );
}

typedef struct update_order {
  mln_map map;
  size_t updates;
  uint64_t update_generation;
  bool event_after_update;
} update_order;

// Records the map's render updates and whether another event of the map
// follows one.
static bool track_update_order(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  update_order* order = context;
  if (event->source != order->map) return false;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) {
    order->updates += 1;
    order->update_generation = event->generation;
    return true;
  }
  if (order->updates > 0) order->event_after_update = true;
  return false;
}

// Moving a layer removes and re-adds it inside MapLibre, which publishes one
// update for each step unless the command holds them. The command announces
// its one update after its other events, with its own generation.
static void a_style_command_publishes_one_render_update(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_loaded_map(runtime);
  add_background_layer(map, "{\"id\":\"a\",\"type\":\"background\"}");
  add_background_layer(map, "{\"id\":\"b\",\"type\":\"background\"}");
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_test_drain_all(runtime);
  const uint64_t before = render_update_generation(map);

  mln_test_completion moved = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_move_style_layer(
    map, MLN_BUFFER_LITERAL("a"), (mln_buffer_view){0}, &moved.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&moved));
  const uint64_t generation = mln_test_completion_generation(&moved);
  mln_test_completion_destroy(&moved);
  TEST_ASSERT_EQUAL_UINT64(before + 1, render_update_generation(map));

  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  update_order order = {.map = map};
  TEST_ASSERT_EQUAL_size_t(
    1, mln_test_drain_counting_matching(runtime, track_update_order, &order)
  );
  TEST_ASSERT_EQUAL_UINT64(generation, order.update_generation);
  TEST_ASSERT_FALSE(order.event_after_update);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A gesture phase sets the gesture flag and moves the camera, and each step
// publishes on its own in MapLibre. One update keeps a frame from showing the
// new camera with a stale gesture flag.
static void a_camera_gesture_update_publishes_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const uint32_t phases[] = {MLN_GESTURE_PHASE_BEGIN, MLN_GESTURE_PHASE_END};
  for (size_t index = 0; index < sizeof(phases) / sizeof(phases[0]); ++index) {
    const uint64_t before = render_update_generation(map);
    mln_camera_update update = mln_camera_update_default();
    update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
    update.gesture_phase = phases[index];
    update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
    update.camera.zoom = 2.0 + (double)index;
    MLN_TEST_AWAIT_OK(
      mln_map_update_camera(map, &update, &completion.descriptor, NULL)
    );
    TEST_ASSERT_EQUAL_UINT64(before + 1, render_update_generation(map));
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Each command inside the group commits and reports its own snapshot, while
// the render update waits for the end.
static void a_group_publishes_one_render_update_at_its_end(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_loaded_map(runtime);
  const uint64_t before = render_update_generation(map);

  begin_group(map);
  add_background_layer(map, "{\"id\":\"a\",\"type\":\"background\"}");
  add_background_layer(map, "{\"id\":\"b\",\"type\":\"background\"}");
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("a"), MLN_BUFFER_LITERAL("background-color"),
    MLN_BUFFER_LITERAL("\"#ff0000\""), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_move_style_layer(
    map, MLN_BUFFER_LITERAL("a"), (mln_buffer_view){0}, &completion.descriptor,
    NULL
  ));
  TEST_ASSERT_TRUE(layer_exists(map, "a"));
  TEST_ASSERT_EQUAL_UINT64(before, render_update_generation(map));

  end_group(map);
  TEST_ASSERT_EQUAL_UINT64(before + 1, render_update_generation(map));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void nested_groups_publish_at_the_outermost_end(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_loaded_map(runtime);
  const uint64_t before = render_update_generation(map);

  begin_group(map);
  begin_group(map);
  add_background_layer(map, "{\"id\":\"a\",\"type\":\"background\"}");
  end_group(map);
  TEST_ASSERT_EQUAL_UINT64(before, render_update_generation(map));
  add_background_layer(map, "{\"id\":\"b\",\"type\":\"background\"}");
  end_group(map);
  TEST_ASSERT_EQUAL_UINT64(before + 1, render_update_generation(map));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_failed_command_leaves_the_group_open(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_loaded_map(runtime);
  const uint64_t before = render_update_generation(map);

  begin_group(map);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "",
    mln_map_remove_style_layer(
      map, MLN_BUFFER_LITERAL("missing"), &completion.descriptor, NULL
    )
  );
  add_background_layer(map, "{\"id\":\"a\",\"type\":\"background\"}");
  TEST_ASSERT_EQUAL_UINT64(before, render_update_generation(map));

  end_group(map);
  TEST_ASSERT_EQUAL_UINT64(before + 1, render_update_generation(map));
  TEST_ASSERT_TRUE(layer_exists(map, "a"));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void an_end_without_an_open_group_fails(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "no open command group",
    mln_map_end_command_group(map, &completion.descriptor, NULL)
  );
  begin_group(map);
  end_group(map);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "no open command group",
    mln_map_end_command_group(map, &completion.descriptor, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A still-image request binds to the update it publishes, which a group would
// hold, so the request fails rather than wait on a stale generation.
static void a_still_image_inside_a_group_fails(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  begin_group(map);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_STATE,
    mln_map_request_still_image(map, &completion.descriptor, NULL)
  );
  end_group(map);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Releasing a map clears the update that an open group holds.
static void a_map_releases_with_an_open_group(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_loaded_map(runtime);

  begin_group(map);
  add_background_layer(map, "{\"id\":\"a\",\"type\":\"background\"}");
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_style_command_publishes_one_render_update);
  RUN_TEST(a_camera_gesture_update_publishes_once);
  RUN_TEST(a_group_publishes_one_render_update_at_its_end);
  RUN_TEST(nested_groups_publish_at_the_outermost_end);
  RUN_TEST(a_failed_command_leaves_the_group_open);
  RUN_TEST(an_end_without_an_open_group_fails);
  RUN_TEST(a_still_image_inside_a_group_fails);
  RUN_TEST(a_map_releases_with_an_open_group);
}
