// A render session's projection: a standalone copy of the transform its last
// frame rendered with, which later map changes do not reach and which outlives
// the session.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/tables.h"
#include "support/test_support.h"
#include "unity.h"

static void render_one_frame(const mln_test_render_fixture* fixture) {
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(fixture)
  );
}

static mln_map_projection create_session_projection(
  const mln_test_render_fixture* fixture
) {
  mln_map_projection projection = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    MLN_STATUS_OK,
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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_projection_create(fixture->session, &projection, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, projection);
}

static mln_screen_point pixel_of(
  mln_map_projection projection, double latitude, double longitude
) {
  mln_screen_point point = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_pixel_for_lat_lng(
      projection, (mln_lat_lng){.latitude = latitude, .longitude = longitude},
      &point, NULL
    )
  );
  return point;
}

static void jump_to(
  mln_map map, double latitude, double longitude, double zoom
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera = mln_camera_options_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = latitude;
  update.camera.longitude = longitude;
  update.camera.zoom = zoom;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static void expect_center(
  mln_map_projection projection, double latitude, double longitude, double zoom
) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_get_camera(projection, &camera, NULL)
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, latitude, camera.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, longitude, camera.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, zoom, camera.zoom);
}

// A camera command reaches the session's projection only once a frame renders
// it.
static void a_session_projection_copies_the_last_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  expect_no_projection(&fixture);

  render_one_frame(&fixture);
  jump_to(map, 12.0, 34.0, 3.0);
  mln_map_projection before = create_session_projection(&fixture);
  expect_center(before, 0.0, 0.0, 0.0);

  render_one_frame(&fixture);
  mln_map_projection after = create_session_projection(&fixture);
  expect_center(after, 12.0, 34.0, 3.0);
  // The earlier copy kept its own transform.
  expect_center(before, 0.0, 0.0, 0.0);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_projection_close(before, NULL));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_projection_close(after, NULL));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The projection a session hands out converts coordinates after the session,
// and then its map and runtime, are gone.
static void a_session_projection_outlives_its_session(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  render_one_frame(&fixture);
  mln_map_projection projection = create_session_projection(&fixture);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  const mln_screen_point center = pixel_of(projection, 0.0, 0.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 32.0, center.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 32.0, center.y);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_close(projection, NULL)
  );
}

// A resize retires the rendered transform, since it no longer matches the
// target, until a frame renders at the new extent. A detached session has none.
static void a_resize_or_detach_withholds_the_projection(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  render_one_frame(&fixture);

  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 32,
    .height = 16,
    .scale_factor = 1.0,
  };
  mln_test_completion resize = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(
                     fixture.session, &extent, &resize.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &resize)
  );
  mln_test_completion_destroy(&resize);
  expect_no_projection(&fixture);

  render_one_frame(&fixture);
  mln_map_projection projection = create_session_projection(&fixture);
  const mln_screen_point center = pixel_of(projection, 0.0, 0.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 16.0, center.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 8.0, center.y);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_close(projection, NULL)
  );

  mln_test_completion detach = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_detach(fixture.session, &detach.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &detach)
  );
  mln_test_completion_destroy(&detach);
  expect_no_projection(&fixture);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
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
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_map_projection_close(projection, NULL)
    );
  }
  return status;
}

// Argument errors take precedence over whether the session has rendered.
static void malformed_projection_requests_are_rejected(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  static const mln_test_validation_case before_a_frame[] = {
    {"no frame yet", NULL, MLN_STATUS_INVALID_STATE, "no rendered projection"},
    {"null session", stale_session, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null output", null_output, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"occupied output", occupied_output, MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  static const mln_test_validation_case after_a_frame[] = {
    {"a rendered frame", NULL, MLN_STATUS_OK, NULL},
    {"null session", stale_session, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null output", null_output, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"occupied output", occupied_output, MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  const projection_call defaults = {.session = fixture.session};
  mln_test_run_validation_table(
    before_a_frame, sizeof(before_a_frame) / sizeof(before_a_frame[0]),
    &defaults, sizeof(defaults), submit_projection, NULL
  );
  render_one_frame(&fixture);
  mln_test_run_validation_table(
    after_a_frame, sizeof(after_a_frame) / sizeof(after_a_frame[0]), &defaults,
    sizeof(defaults), submit_projection, NULL
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_session_projection_copies_the_last_rendered_frame);
  RUN_TEST(a_session_projection_outlives_its_session);
  RUN_TEST(a_resize_or_detach_withholds_the_projection);
  RUN_TEST(malformed_projection_requests_are_rejected);
}
