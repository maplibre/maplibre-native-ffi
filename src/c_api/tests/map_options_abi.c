// Raw C ABI coverage: map option structs use null pointers, undersized
// structs, unknown raw masks/enums, and preinitialized outputs hidden by
// bindings.

#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

typedef struct map_fixture {
  mln_runtime runtime;
  mln_map map;
} map_fixture;

static map_fixture create_map_fixture(void) {
  map_fixture fixture = {.runtime = mln_test_create_runtime()};
  fixture.map = mln_test_create_map(fixture.runtime);
  return fixture;
}

static void destroy_map_fixture(map_fixture fixture) {
  mln_test_destroy_map(fixture.map);
  mln_test_destroy_runtime(fixture.runtime);
}

static mln_camera_options test_camera(void) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                  MLN_CAMERA_OPTION_PADDING | MLN_CAMERA_OPTION_ANCHOR;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 11.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  camera.padding = (mln_edge_insets){1.0, 2.0, 3.0, 4.0};
  camera.anchor = (mln_screen_point){25.0, 30.0};
  return camera;
}

#define EXPECT_COMMAND_COMPLETES(expected_status, expression)        \
  do {                                                               \
    mln_test_completion completion = mln_test_completion_default(0); \
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, (expression));              \
    TEST_ASSERT_EQUAL_INT(                                           \
      (expected_status), mln_test_completion_finish(&completion)     \
    );                                                               \
    mln_test_completion_destroy(&completion);                        \
  } while (false)

static map_fixture create_static_map_fixture(void) {
  map_fixture fixture = {.runtime = mln_test_create_runtime()};
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  fixture.map = mln_test_create_map_with_options(fixture.runtime, &options);
  return fixture;
}

static mln_test_completion start_pending_still_image(map_fixture fixture) {
  mln_test_completion pending = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_request_still_image(fixture.map, &pending.descriptor)
  );

  mln_test_completion duplicate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_request_still_image(fixture.map, &duplicate.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_test_completion_finish(&duplicate)
  );
  mln_test_completion_destroy(&duplicate);
  return pending;
}
static mln_map_snapshot read_settled_snapshot(map_fixture fixture) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_runtime_barrier(fixture.runtime)
  );
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(fixture.map, &snapshot)
  );
  return snapshot;
}

static mln_bound_options read_bounds(map_fixture fixture) {
  return read_settled_snapshot(fixture).bounds;
}

static void camera_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  uint64_t generation = 0;
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_snapshot_get(fixture.map, NULL, &generation)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_snapshot_get(fixture.map, &camera, NULL)
  );
  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_update_camera(fixture.map, NULL, &rejected)
  );
  mln_camera_update update = mln_camera_update_default();
  update.size -= 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_update_camera(fixture.map, &update, &rejected)
  );
  update = mln_camera_update_default();
  update.mode = UINT32_MAX;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_update_camera(fixture.map, &update, &rejected)
  );
  destroy_map_fixture(fixture);
}

static void camera_snapshot_command_copy_and_disposition_are_ordered(void) {
  map_fixture fixture = create_map_fixture();
  mln_test_drain_all(fixture.runtime);

  mln_map_snapshot before = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(fixture.map, &before)
  );

  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  update.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  update.animation.fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
  update.animation.transition_id = UINT64_C(77);
  mln_test_completion command = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_update_camera(fixture.map, &update, &command.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&command));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, mln_test_completion_disposition(&command)
  );
  const uint64_t command_generation = mln_test_completion_generation(&command);
  TEST_ASSERT_GREATER_THAN_UINT64(before.generation, command_generation);
  mln_test_completion_destroy(&command);

  update.camera.longitude = 12.0;
  update.camera.zoom = 1.0;
  update.camera.padding.left = 999.0;

  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_camera_query(fixture.map, &query.descriptor)
  );
  mln_camera_query_result result = {.size = sizeof(mln_camera_query_result)};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&query));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&query, &result, sizeof(result))
  );
  mln_test_completion_destroy(&query);
  TEST_ASSERT_EQUAL_DOUBLE(-122.4194, result.camera.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(11.0, result.camera.zoom);
  TEST_ASSERT_EQUAL_DOUBLE(2.0, result.camera.padding.left);
  TEST_ASSERT_GREATER_THAN_UINT64(before.generation, result.generation);

  mln_map_snapshot after = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(fixture.map, &after)
  );
  TEST_ASSERT_GREATER_THAN_UINT64(before.generation, after.generation);
  TEST_ASSERT_EQUAL_DOUBLE(-122.4194, after.camera.longitude);
  destroy_map_fixture(fixture);
}

static void relative_camera_commands_compose_in_runtime_order(void) {
  map_fixture fixture = create_map_fixture();
  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_update_camera(fixture.map, &update, &completion.descriptor)
  );

  const mln_screen_point anchor = {.x = 25.0, .y = 30.0};
  mln_camera_delta delta = mln_camera_delta_default();
  delta.offset = (mln_screen_point){.x = 5.0, .y = -3.0};
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_apply_camera_delta(fixture.map, &delta, &completion.descriptor)
  );
  delta.kind = MLN_CAMERA_DELTA_SCALE;
  delta.amount = 2.0;
  delta.has_anchor = true;
  delta.anchor = anchor;
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_apply_camera_delta(fixture.map, &delta, &completion.descriptor)
  );
  delta.kind = MLN_CAMERA_DELTA_BEARING;
  delta.amount = 15.0;
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_apply_camera_delta(fixture.map, &delta, &completion.descriptor)
  );
  delta.kind = MLN_CAMERA_DELTA_PITCH;
  delta.amount = 5.0;
  delta.has_anchor = false;
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_apply_camera_delta(fixture.map, &delta, &completion.descriptor)
  );

  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_camera_query(fixture.map, &query.descriptor)
  );
  mln_camera_query_result result = {.size = sizeof(mln_camera_query_result)};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&query));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&query, &result, sizeof(result))
  );
  mln_test_completion_destroy(&query);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, result.camera.zoom);
  TEST_ASSERT_EQUAL_DOUBLE(27.0, result.camera.bearing);
  TEST_ASSERT_EQUAL_DOUBLE(35.0, result.camera.pitch);

  delta.kind = MLN_CAMERA_DELTA_SCALE;
  delta.amount = 0.0;
  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_apply_camera_delta(fixture.map, &delta, &rejected)
  );

  delta = mln_camera_delta_default();
  delta.kind = UINT32_MAX;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_apply_camera_delta(fixture.map, &delta, &rejected)
  );

  delta = mln_camera_delta_default();
  delta.has_anchor = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_apply_camera_delta(fixture.map, &delta, &rejected)
  );
  destroy_map_fixture(fixture);
}

static void camera_fitting_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_completion operation = mln_test_discard_completion();
  mln_camera_fit_options fit = mln_camera_fit_options_default();
  fit.size = sizeof(mln_camera_fit_options) - 1;
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = -10.0, .longitude = -10.0},
    .northeast = {.latitude = 10.0, .longitude = 10.0},
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_lat_lng_bounds(fixture.map, bounds, &fit, &operation)
  );
  const mln_lat_lng coordinate = {.latitude = 0.0, .longitude = 0.0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_lat_lngs(fixture.map, NULL, 1, NULL, &operation)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_lat_lngs(fixture.map, &coordinate, 1, NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_geometry(
      fixture.map, (mln_buffer_view){0}, NULL, &operation
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lng_bounds_for_camera(fixture.map, NULL, &operation)
  );
  destroy_map_fixture(fixture);
}

static void camera_bounds_constraints_reject_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_bound_options options = mln_bound_options_default();
  options.size = sizeof(mln_bound_options) - 1;
  mln_completion command = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_bounds(fixture.map, NULL, &command)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_bounds(fixture.map, &options, &command)
  );
  options = mln_bound_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_bounds(fixture.map, &options, &command)
  );
  options = mln_bound_options_default();
  options.fields = MLN_BOUND_OPTION_BOUNDS | MLN_BOUND_OPTION_UNBOUNDED;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_bounds(fixture.map, &options, &command)
  );
  destroy_map_fixture(fixture);
}

static bool near_longitude(double actual, double expected) {
  const double delta = actual - expected;
  return delta > -1e-6 && delta < 1e-6;
}

static double jumped_longitude(mln_map map, double longitude) {
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 0.0;
  update.camera.longitude = longitude;
  update.camera.zoom = 2.0;
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK, mln_map_update_camera(map, &update, &completion.descriptor)
  );
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_camera_query(map, &query.descriptor)
  );
  mln_camera_query_result result = {.size = sizeof(mln_camera_query_result)};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&query));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&query, &result, sizeof(result))
  );
  mln_test_completion_destroy(&query);
  return result.camera.longitude;
}

// The unbounded state is distinct from world bounds, which the
// southwest/northeast pair alone cannot express.
static void camera_bounds_distinguish_unbounded_from_world(void) {
  map_fixture fixture = create_map_fixture();

  mln_bound_options snapshot = read_bounds(fixture);
  TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
  TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
  TEST_ASSERT_TRUE(
    near_longitude(jumped_longitude(fixture.map, 200.0), -160.0)
  );

  mln_bound_options world = mln_bound_options_default();
  world.fields = MLN_BOUND_OPTION_BOUNDS;
  world.bounds.southwest.latitude = -90.0;
  world.bounds.southwest.longitude = -180.0;
  world.bounds.northeast.latitude = 90.0;
  world.bounds.northeast.longitude = 180.0;
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_set_bounds(fixture.map, &world, &completion.descriptor)
  );

  snapshot = read_bounds(fixture);
  TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
  TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
  TEST_ASSERT_TRUE(near_longitude(snapshot.bounds.northeast.longitude, 180.0));
  TEST_ASSERT_TRUE(near_longitude(jumped_longitude(fixture.map, 200.0), 180.0));

  mln_bound_options unbounded = mln_bound_options_default();
  unbounded.fields = MLN_BOUND_OPTION_UNBOUNDED;
  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_set_bounds(fixture.map, &unbounded, &completion.descriptor)
  );

  snapshot = read_bounds(fixture);
  TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
  TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
  TEST_ASSERT_TRUE(
    near_longitude(jumped_longitude(fixture.map, 200.0), -160.0)
  );

  destroy_map_fixture(fixture);
}

static void free_camera_options_reject_raw_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_free_camera_options options = mln_free_camera_options_default();
  options.size = sizeof(mln_free_camera_options) - 1;
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_free_camera_options(fixture.map, NULL, &completion)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_free_camera_options(fixture.map, &options, &completion)
  );
  options = mln_free_camera_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_free_camera_options(fixture.map, &options, &completion)
  );
  destroy_map_fixture(fixture);
}

static void map_projection_mode_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_projection_mode(fixture.map, NULL, &completion)
  );
  mln_projection_mode mode = mln_projection_mode_default();
  mode.size = sizeof(mln_projection_mode) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_projection_mode(fixture.map, &mode, &completion)
  );
  mode = mln_projection_mode_default();
  mode.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_projection_mode(fixture.map, &mode, &completion)
  );
  destroy_map_fixture(fixture);
}

static const mln_lat_lng center = {.latitude = 37.7749, .longitude = -122.4194};

static void map_coordinate_conversion_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_completion operation = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_pixel_for_lat_lng(fixture.map, center, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lng_for_pixel(
      fixture.map, (mln_screen_point){.x = 0.0, .y = 0.0}, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lng_for_pixel_unwrapped(
      fixture.map, (mln_screen_point){.x = 0.0, .y = 0.0}, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_pixels_for_lat_lngs(fixture.map, NULL, 1, &operation)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lngs_for_pixels(fixture.map, NULL, 1, &operation)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lngs_for_pixels_unwrapped(fixture.map, NULL, 1, &operation)
  );
  destroy_map_fixture(fixture);
}

static void unwrapped_coordinate_conversion_preserves_world_copies(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 1024;
  options.initial_extent.height = 512;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 0.0;
  update.camera.longitude = 179.0;
  update.camera.zoom = 0.0;
  mln_test_completion jump = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_update_camera(map, &update, &jump.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&jump));
  mln_test_completion_destroy(&jump);

  const mln_screen_point points[] = {
    {.x = 0.0, .y = 256.0},
    {.x = 512.0, .y = 256.0},
    {.x = 1024.0, .y = 256.0},
  };
  mln_lat_lng wrapped[3] = {0};
  mln_lat_lng unwrapped[3] = {0};
  mln_test_completion wrapped_batch =
    mln_test_completion_default(3 * sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lngs_for_pixels(map, points, 3, &wrapped_batch.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&wrapped_batch)
  );
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&wrapped_batch, wrapped, sizeof(wrapped))
  );
  mln_test_completion_destroy(&wrapped_batch);

  mln_test_completion unwrapped_batch =
    mln_test_completion_default(3 * sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_lat_lngs_for_pixels_unwrapped(
                     map, points, 3, &unwrapped_batch.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&unwrapped_batch)
  );
  TEST_ASSERT_TRUE(mln_test_completion_copy_value(
    &unwrapped_batch, unwrapped, sizeof(unwrapped)
  ));
  mln_test_completion_destroy(&unwrapped_batch);

  for (size_t index = 0; index < 3; index++) {
    TEST_ASSERT_TRUE(wrapped[index].longitude >= -180.0);
    TEST_ASSERT_TRUE(wrapped[index].longitude <= 180.0);
  }
  TEST_ASSERT_DOUBLE_WITHIN(1e-10, 179.0, unwrapped[1].longitude);
  TEST_ASSERT_TRUE(unwrapped[2].longitude - unwrapped[0].longitude > 360.0);

  mln_lat_lng wrapped_right = {0};
  mln_test_completion wrapped_single =
    mln_test_completion_default(sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lng_for_pixel(map, points[2], &wrapped_single.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&wrapped_single)
  );
  TEST_ASSERT_TRUE(mln_test_completion_copy_value(
    &wrapped_single, &wrapped_right, sizeof(wrapped_right)
  ));
  mln_test_completion_destroy(&wrapped_single);
  TEST_ASSERT_TRUE(wrapped_right.longitude >= -180.0);
  TEST_ASSERT_TRUE(wrapped_right.longitude <= 180.0);

  mln_lat_lng unwrapped_right = {0};
  mln_test_completion unwrapped_single =
    mln_test_completion_default(sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_lat_lng_for_pixel_unwrapped(
                     map, points[2], &unwrapped_single.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&unwrapped_single)
  );
  TEST_ASSERT_TRUE(mln_test_completion_copy_value(
    &unwrapped_single, &unwrapped_right, sizeof(unwrapped_right)
  ));
  mln_test_completion_destroy(&unwrapped_single);
  TEST_ASSERT_EQUAL_DOUBLE(unwrapped[2].longitude, unwrapped_right.longitude);

  update.camera.zoom = 2.0;
  mln_test_completion zoomed = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_update_camera(map, &update, &zoomed.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&zoomed));
  mln_test_completion_destroy(&zoomed);

  const mln_screen_point antimeridian_points[] = {points[0], points[2]};
  mln_lat_lng antimeridian_wrapped[2] = {0};
  mln_lat_lng antimeridian_unwrapped[2] = {0};
  mln_test_completion antimeridian_wrapped_batch =
    mln_test_completion_default(2 * sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lngs_for_pixels(
      map, antimeridian_points, 2, &antimeridian_wrapped_batch.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&antimeridian_wrapped_batch)
  );
  TEST_ASSERT_TRUE(mln_test_completion_copy_value(
    &antimeridian_wrapped_batch, antimeridian_wrapped,
    sizeof(antimeridian_wrapped)
  ));
  mln_test_completion_destroy(&antimeridian_wrapped_batch);

  mln_test_completion antimeridian_unwrapped_batch =
    mln_test_completion_default(2 * sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lngs_for_pixels_unwrapped(
      map, antimeridian_points, 2, &antimeridian_unwrapped_batch.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&antimeridian_unwrapped_batch)
  );
  TEST_ASSERT_TRUE(mln_test_completion_copy_value(
    &antimeridian_unwrapped_batch, antimeridian_unwrapped,
    sizeof(antimeridian_unwrapped)
  ));
  mln_test_completion_destroy(&antimeridian_unwrapped_batch);

  TEST_ASSERT_TRUE(
    antimeridian_unwrapped[0].longitude < antimeridian_unwrapped[1].longitude
  );
  TEST_ASSERT_TRUE(antimeridian_unwrapped[0].longitude < 180.0);
  TEST_ASSERT_TRUE(antimeridian_unwrapped[1].longitude > 180.0);
  TEST_ASSERT_TRUE(
    antimeridian_unwrapped[1].longitude - antimeridian_unwrapped[0].longitude <
    360.0
  );
  TEST_ASSERT_TRUE(antimeridian_wrapped[0].longitude > 0.0);
  TEST_ASSERT_TRUE(antimeridian_wrapped[1].longitude < 0.0);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void projected_meters_reject_invalid_arguments(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_projected_meters_for_lat_lng(center, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_lat_lng_for_projected_meters((mln_projected_meters){0}, NULL)
  );
}

static void map_debug_options_reject_raw_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_debug_options(MLN_HANDLE_NULL, 0, &completion)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_debug_options(fixture.map, UINT32_C(1) << 31, &completion)
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_rendering_stats_view_enabled(MLN_HANDLE_NULL, true, &completion)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_dump_debug_logs(MLN_HANDLE_NULL, &completion)
  );
  destroy_map_fixture(fixture);
}

// FastPFOR decoding stays off unless a host asks for it, and a map accepts the
// opt-in.
static void map_options_default_leaves_fast_pfor_decoding_off(void) {
  const mln_map_options defaults = mln_map_options_default();
  TEST_ASSERT_FALSE(defaults.fast_pfor_enabled);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.fast_pfor_enabled = true;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void map_extent_snapshot_tracks_resize_and_scale_factor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 512, .height = 256, .scale_factor = 1.1};
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_snapshot_get(map, &snapshot));
  TEST_ASSERT_EQUAL_UINT32(512, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(256, snapshot.logical_extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.logical_extent.scale_factor);
  const uint64_t initial_generation = snapshot.generation;

  EXPECT_COMMAND_COMPLETES(
    MLN_STATUS_OK,
    mln_map_resize(
      map,
      (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 2.25},
      &completion.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  snapshot = (mln_map_snapshot){.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_snapshot_get(map, &snapshot));
  TEST_ASSERT_GREATER_THAN_UINT64(initial_generation, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.logical_extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(2.25, snapshot.logical_extent.scale_factor);

  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_resize(
      map, (mln_logical_extent){.width = 0, .height = 48, .scale_factor = 2.25},
      &rejected
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_snapshot_get(MLN_HANDLE_NULL, &snapshot)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void map_close_cancels_observed_pending_still_image(void) {
  map_fixture fixture = create_static_map_fixture();
  mln_test_completion pending = start_pending_still_image(fixture);

  mln_test_destroy_map(fixture.map);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_CANCELLED, mln_test_completion_finish(&pending)
  );
  mln_test_completion_destroy(&pending);
  mln_test_destroy_runtime(fixture.runtime);
}

static void map_viewport_options_reject_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_map_viewport_options options = mln_map_viewport_options_default();
  options.size = sizeof(mln_map_viewport_options) - 1;
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, NULL, &completion)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options, &completion)
  );
  options = mln_map_viewport_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options, &completion)
  );
  options = mln_map_viewport_options_default();
  options.fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  options.north_orientation = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options, &completion)
  );
  options = mln_map_viewport_options_default();
  options.fields = MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
  options.constrain_mode = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options, &completion)
  );
  options = mln_map_viewport_options_default();
  options.fields = MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
  options.viewport_mode = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options, &completion)
  );
  destroy_map_fixture(fixture);
}

static void map_tile_options_reject_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_map_tile_options options = mln_map_tile_options_default();
  options.size = sizeof(mln_map_tile_options) - 1;
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_tile_options(fixture.map, NULL, &completion)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_tile_options(fixture.map, &options, &completion)
  );
  options = mln_map_tile_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_tile_options(fixture.map, &options, &completion)
  );
  options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_MODE;
  options.lod_mode = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_tile_options(fixture.map, &options, &completion)
  );
  destroy_map_fixture(fixture);
}

// A committed completion carries the snapshot generation it published.
static void committed_command_generation_matches_snapshot(void) {
  map_fixture fixture = create_map_fixture();
  mln_test_drain_all(fixture.runtime);

  mln_test_completion completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_debug_options(
      fixture.map, MLN_MAP_DEBUG_TILE_BORDERS, &completion.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const uint64_t committed = mln_test_completion_generation(&completion);
  mln_test_completion_destroy(&completion);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, committed);

  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(fixture.map, &snapshot)
  );
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(committed, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(MLN_MAP_DEBUG_TILE_BORDERS, snapshot.debug_options);
  destroy_map_fixture(fixture);
}

// Committed tile and viewport option commands are visible in the snapshot
// published at or past the commit's generation.
static void committed_option_commands_are_visible_in_snapshot(void) {
  map_fixture fixture = create_map_fixture();
  mln_test_drain_all(fixture.runtime);

  mln_map_tile_options tile = mln_map_tile_options_default();
  tile.fields = MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
  tile.prefetch_zoom_delta = 3;
  mln_test_completion completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_tile_options(fixture.map, &tile, &completion.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  uint64_t committed = mln_test_completion_generation(&completion);
  mln_test_completion_destroy(&completion);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(fixture.map, &snapshot)
  );
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(committed, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(3, snapshot.tile.prefetch_zoom_delta);

  mln_map_viewport_options viewport = mln_map_viewport_options_default();
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  viewport.north_orientation = MLN_NORTH_ORIENTATION_RIGHT;
  completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_viewport_options(fixture.map, &viewport, &completion.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  committed = mln_test_completion_generation(&completion);
  mln_test_completion_destroy(&completion);
  snapshot = (mln_map_snapshot){.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(fixture.map, &snapshot)
  );
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(committed, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NORTH_ORIENTATION_RIGHT, snapshot.viewport.north_orientation
  );
  destroy_map_fixture(fixture);
}

void run_map_options_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(camera_rejects_invalid_arguments);
  RUN_TEST(camera_snapshot_command_copy_and_disposition_are_ordered);
  RUN_TEST(relative_camera_commands_compose_in_runtime_order);
  RUN_TEST(camera_fitting_rejects_invalid_arguments);
  RUN_TEST(camera_bounds_constraints_reject_invalid_arguments);
  RUN_TEST(camera_bounds_distinguish_unbounded_from_world);
  RUN_TEST(free_camera_options_reject_raw_invalid_arguments);
  RUN_TEST(map_projection_mode_rejects_invalid_arguments);
  RUN_TEST(map_coordinate_conversion_rejects_invalid_arguments);
  RUN_TEST(unwrapped_coordinate_conversion_preserves_world_copies);
  RUN_TEST(projected_meters_reject_invalid_arguments);
  RUN_TEST(map_debug_options_reject_raw_invalid_arguments);
  RUN_TEST(map_options_default_leaves_fast_pfor_decoding_off);
  RUN_TEST(map_extent_snapshot_tracks_resize_and_scale_factor);
  RUN_TEST(map_close_cancels_observed_pending_still_image);
  RUN_TEST(map_viewport_options_reject_invalid_arguments);
  RUN_TEST(map_tile_options_reject_invalid_arguments);
  RUN_TEST(committed_command_generation_matches_snapshot);
  RUN_TEST(committed_option_commands_are_visible_in_snapshot);
}
