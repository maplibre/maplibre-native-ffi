// A render session's projection: a standalone copy of the transform its last
// frame rendered with, which later map changes do not reach and which outlives
// the session. It also shows what a frame gated on a command's render update
// draws.

#include "support/frames.h"
#include "support/style.h"
#include "support/test_support.h"

static void render_one_frame(const mln_test_render_fixture* fixture) {
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(fixture)
  );
}

static mln_map_projection create_session_projection(
  const mln_test_render_fixture* fixture
) {
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK_MESSAGE(
    mln_render_session_projection_create(
      fixture->session, &projection, MLN_TEST_DIAGNOSTIC
    ),
    mln_test_last_error()
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, projection);
  return projection;
}

static void expect_no_projection(const mln_test_render_fixture* fixture) {
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_projection_create(fixture->session, &projection, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, projection);
}

static void expect_pixel(mln_map_projection projection, double x, double y) {
  mln_screen_point point = {0};
  MLN_TEST_OK(mln_map_projection_pixel_for_lat_lng(
    projection, (mln_lat_lng){0.0, 0.0}, &point, NULL
  ));
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, x, point.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, y, point.y);
}

static void expect_center(
  mln_map_projection projection, double latitude, double longitude, double zoom
) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &camera, NULL));
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, latitude, camera.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, longitude, camera.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, zoom, camera.zoom);
}

typedef struct projection_call {
  mln_render_session session;
  mln_map_projection preset;
  bool null_output;
} projection_call;

static void stale_session(void* call) {
  ((projection_call*)call)->session = MLN_HANDLE_NULL;
}
static void null_output(void* call) {
  ((projection_call*)call)->null_output = true;
}
static void occupied_output(void* call) {
  ((projection_call*)call)->preset = 1;
}

static mln_status submit_projection(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  (void)context;
  const projection_call* call = descriptor;
  mln_map_projection projection = call->preset;
  const mln_status status = mln_render_session_projection_create(
    call->session, call->null_output ? NULL : &projection, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  }
  return status;
}

// Argument errors take precedence over whether the session has rendered.
static void expect_malformed_requests_rejected(
  const mln_test_render_fixture* fixture, bool rendered
) {
  const mln_test_validation_case cases[] = {
    rendered ? (
                 mln_test_validation_case
               ){"a rendered frame", NULL, MLN_STATUS_OK, NULL}
             : (
                 mln_test_validation_case
               ){"no frame yet", NULL, MLN_STATUS_INVALID_STATE,
                 "no rendered projection"},
    {"null session", stale_session, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null output", null_output, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"occupied output", occupied_output, MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  const projection_call defaults = {.session = fixture->session};
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_projection, NULL
  );
}

// A camera command reaches the session's projection only once a frame renders
// it, and an earlier copy keeps its own transform. A resize retires the
// rendered transform, since it no longer matches the target, until a frame
// renders at the new extent. A detached session has none, and a projection it
// handed out converts coordinates after the session, and then its map and
// runtime, are gone.
static void a_session_projection_copies_the_last_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  expect_no_projection(&fixture);
  expect_malformed_requests_rejected(&fixture, false);

  render_one_frame(&fixture);
  expect_malformed_requests_rejected(&fixture, true);
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 12.0;
  update.camera.longitude = 34.0;
  update.camera.zoom = 3.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  mln_map_projection before = create_session_projection(&fixture);
  expect_center(before, 0.0, 0.0, 0.0);
  expect_pixel(before, 32.0, 32.0);
  render_one_frame(&fixture);
  mln_map_projection after = create_session_projection(&fixture);
  expect_center(after, 12.0, 34.0, 3.0);
  expect_center(before, 0.0, 0.0, 0.0);
  MLN_TEST_OK(mln_map_projection_close(before, NULL));
  MLN_TEST_OK(mln_map_projection_close(after, NULL));

  // Back to the origin, so it lies at the center of the resized target.
  update.camera.latitude = 0.0;
  update.camera.longitude = 0.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 32,
    .height = 16,
    .scale_factor = 1.0,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session, &extent, &completion.descriptor, NULL
    )
  );
  expect_no_projection(&fixture);
  render_one_frame(&fixture);
  mln_map_projection projection = create_session_projection(&fixture);
  expect_pixel(projection, 16.0, 8.0);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  expect_no_projection(&fixture);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  expect_pixel(projection, 16.0, 8.0);
  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
}

// Requests render-if-needed demands that wait for a map update until one
// renders an update at or past `generation`, as a host gates presentation on a
// command.
static void render_through_update(
  const mln_test_render_fixture* fixture, uint64_t generation
) {
  const mln_test_deadline deadline = mln_test_deadline_default();
  for (uint64_t token = 1; !mln_test_deadline_passed(deadline); token += 1) {
    mln_frame_demand demand = mln_frame_demand_default();
    demand.flags =
      MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
    demand.token = token;
    demand.coalescing_boundary = token;
    MLN_TEST_OK(
      mln_render_session_request_frame(fixture->session, &demand, NULL)
    );
    const mln_render_frame_batch batch =
      mln_test_render_wait_for_results(fixture, 1);
    const mln_render_frame_result result =
      mln_test_render_batch_result(batch, 0);
    mln_render_frame_batch_release(batch);
    if (
      result.disposition == MLN_RENDER_RESULT_RENDERED &&
      result.map_update_generation >= generation
    ) {
      return;
    }
  }
  TEST_FAIL_MESSAGE("no frame rendered the command's render update");
}

// A committed command's snapshot names the latest render update, and the
// first frame that renders that update or a later one draws the command.
static void a_frame_at_a_commands_render_update_draws_the_command(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  render_one_frame(&fixture);
  mln_render_session_snapshot before = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(fixture.session, &before, NULL));

  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = -21.0;
  update.camera.longitude = 43.0;
  update.camera.zoom = 2.0;
  mln_test_completion jump = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_update_camera(map, &update, &jump.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&jump));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, mln_test_completion_disposition(&jump)
  );
  const uint64_t committed = mln_test_completion_generation(&jump);
  mln_test_completion_destroy(&jump);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &snapshot, NULL));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(committed, snapshot.generation);
  // The command published its render update before it completed.
  TEST_ASSERT_GREATER_THAN_UINT64(
    before.rendered_update_generation, snapshot.latest_render_update_generation
  );

  render_through_update(&fixture, snapshot.latest_render_update_generation);
  mln_map_projection projection = create_session_projection(&fixture);
  expect_center(projection, -21.0, 43.0, 2.0);
  MLN_TEST_OK(mln_map_projection_close(projection, NULL));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_session_projection_copies_the_last_rendered_frame);
  RUN_TEST(a_frame_at_a_commands_render_update_draws_the_command);
}
