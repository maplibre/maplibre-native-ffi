// Raw C ABI coverage: map option structs use null pointers, undersized
// structs, unknown raw masks/enums, and preinitialized outputs hidden by
// bindings.

#include <stdint.h>

#include "test_support.h"
#include "unity.h"

typedef struct map_fixture {
  mln_runtime* runtime;
  mln_map* map;
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
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 11.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  return camera;
}

// This verifies null, undersized, and unknown-mask camera descriptors that
// typed bindings cannot construct.
static void camera_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_camera(fixture.map, NULL)
  );
  mln_camera_options snapshot = mln_camera_options_default();
  snapshot.size = sizeof(mln_camera_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_camera(fixture.map, &snapshot)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_jump_to(fixture.map, NULL)
  );
  mln_camera_options camera = test_camera();
  camera.size = sizeof(mln_camera_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_jump_to(fixture.map, &camera)
  );
  camera = test_camera();
  camera.fields |= UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_jump_to(fixture.map, &camera)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_ease_to(fixture.map, NULL, NULL)
  );
  camera = test_camera();
  mln_animation_options animation = mln_animation_options_default();
  animation.size = sizeof(mln_animation_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_ease_to(fixture.map, &camera, &animation)
  );
  animation = mln_animation_options_default();
  animation.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_fly_to(fixture.map, &camera, &animation)
  );
  destroy_map_fixture(fixture);
}

// This verifies raw null arrays, null outputs, and undersized fit descriptors
// hidden by binding collections.
static void camera_fitting_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_camera_options camera = mln_camera_options_default();
  mln_camera_fit_options fit = mln_camera_fit_options_default();
  fit.size = sizeof(mln_camera_fit_options) - 1;
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = -10.0, .longitude = -10.0},
    .northeast = {.latitude = 10.0, .longitude = 10.0},
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_lat_lng_bounds(fixture.map, bounds, &fit, &camera)
  );
  const mln_lat_lng coordinate = {.latitude = 0.0, .longitude = 0.0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_lat_lngs(fixture.map, NULL, 1, NULL, &camera)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_lat_lngs(fixture.map, &coordinate, 1, NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_camera_for_geometry(fixture.map, NULL, NULL, &camera)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lng_bounds_for_camera(fixture.map, NULL, NULL)
  );
  destroy_map_fixture(fixture);
}

// This verifies null, undersized, and unknown-mask bound descriptors before
// binding validation applies.
static void camera_bounds_constraints_reject_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_bounds(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_bounds(fixture.map, NULL)
  );
  mln_bound_options options = mln_bound_options_default();
  options.size = sizeof(mln_bound_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_bounds(fixture.map, &options)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_bounds(fixture.map, &options)
  );
  options = mln_bound_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_bounds(fixture.map, &options)
  );
  destroy_map_fixture(fixture);
}

// This verifies raw free-camera output storage, struct-size, and field-mask
// validation.
static void free_camera_options_reject_raw_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_free_camera_options(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_free_camera_options(fixture.map, NULL)
  );
  mln_free_camera_options options = mln_free_camera_options_default();
  options.size = sizeof(mln_free_camera_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_free_camera_options(fixture.map, &options)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_free_camera_options(fixture.map, &options)
  );
  options = mln_free_camera_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_free_camera_options(fixture.map, &options)
  );
  destroy_map_fixture(fixture);
}

// This verifies null, undersized, and unknown-mask projection-mode descriptors
// hidden by binding types.
static void map_projection_mode_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_projection_mode(fixture.map, NULL)
  );
  mln_projection_mode snapshot = mln_projection_mode_default();
  snapshot.size = sizeof(mln_projection_mode) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_projection_mode(fixture.map, &snapshot)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_projection_mode(fixture.map, NULL)
  );
  mln_projection_mode mode = mln_projection_mode_default();
  mode.size = sizeof(mln_projection_mode) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_projection_mode(fixture.map, &mode)
  );
  mode = mln_projection_mode_default();
  mode.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_projection_mode(fixture.map, &mode)
  );
  destroy_map_fixture(fixture);
}

static const mln_lat_lng center = {.latitude = 37.7749, .longitude = -122.4194};

// This verifies raw null scalar and array outputs for coordinate conversion
// entry points.
static void map_coordinate_conversion_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  mln_screen_point point = {0};
  mln_lat_lng coordinate = {0};
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
    mln_map_pixels_for_lat_lngs(fixture.map, NULL, 1, &point)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lngs_for_pixels(fixture.map, NULL, 1, &coordinate)
  );
  destroy_map_fixture(fixture);
}

// This verifies raw projection ownership, preinitialized outputs, and null or
// undersized descriptor handling.
static void standalone_projection_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_create(fixture.map, NULL)
  );
  mln_map_projection* projection = (mln_map_projection*)(uintptr_t)1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_create(fixture.map, &projection)
  );
  projection = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create(fixture.map, &projection)
  );
  TEST_ASSERT_NOT_NULL(projection);
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_get_camera(projection, NULL)
  );
  camera.size = sizeof(mln_camera_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_get_camera(projection, &camera)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_set_camera(projection, NULL)
  );
  const mln_edge_insets padding = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_set_visible_coordinates(projection, NULL, 1, padding)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_set_visible_geometry(projection, NULL, padding)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_pixel_for_lat_lng(projection, center, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_lat_lng_for_pixel(
                                   projection, (mln_screen_point){0}, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_projection_destroy(projection));
  destroy_map_fixture(fixture);
}

// This verifies the free conversion functions reject null output pointers that
// bindings never pass.
static void projected_meters_reject_invalid_arguments(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_projected_meters_for_lat_lng(center, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_lat_lng_for_projected_meters((mln_projected_meters){0}, NULL)
  );
}

// This verifies raw null handles, null outputs, and unknown debug-mask bits
// across the debug entry points.
static void map_debug_options_reject_raw_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  uint32_t out_options = 0;
  bool out_bool = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_debug_options(NULL, 0)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_debug_options(fixture.map, UINT32_C(1) << 31)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_debug_options(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_debug_options(NULL, &out_options)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_rendering_stats_view_enabled(NULL, true)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_rendering_stats_view_enabled(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_rendering_stats_view_enabled(NULL, &out_bool)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_is_fully_loaded(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_is_fully_loaded(NULL, &out_bool)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_dump_debug_logs(NULL)
  );
  destroy_map_fixture(fixture);
}

// This verifies the raw size accessor reports the creation size, follows a
// render session attach and resize, keeps the creation pixel ratio across a
// render target that carries a different scale factor, and rejects each null
// output pointer independently.
static void map_size_tracks_attach_and_resize(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = NULL;
  mln_map_options options = mln_map_options_default();
  options.width = 512;
  options.height = 256;
  options.scale_factor = 1.1;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_create(runtime, &options, &map));

  uint32_t width = 0;
  uint32_t height = 0;
  double scale_factor = 0.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(512, width);
  TEST_ASSERT_EQUAL_UINT32(256, height);
  TEST_ASSERT_TRUE(scale_factor == 1.1);

  // The fixture attaches a 64x64 target at scale factor 1.0, so this also
  // covers the map keeping its own pixel ratio.
  mln_test_render_fixture render = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &render));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(64, width);
  TEST_ASSERT_EQUAL_UINT32(64, height);
  TEST_ASSERT_TRUE(scale_factor == 1.1);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(render.session, 96, 48, 1.0)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(96, width);
  TEST_ASSERT_EQUAL_UINT32(48, height);
  TEST_ASSERT_TRUE(scale_factor == 1.1);
  mln_test_render_fixture_destroy(&render);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_size(NULL, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_size(map, NULL, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_size(map, &width, NULL, &scale_factor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_size(map, &width, &height, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// This verifies raw viewport struct sizes, masks, enum discriminants, and
// output pointers.
static void map_viewport_options_reject_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_viewport_options(fixture.map, NULL)
  );
  mln_map_viewport_options options = mln_map_viewport_options_default();
  options.size = sizeof(mln_map_viewport_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_viewport_options(fixture.map, &options)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_viewport_options(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options)
  );
  options = mln_map_viewport_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options)
  );
  options = mln_map_viewport_options_default();
  options.fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  options.north_orientation = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options)
  );
  options = mln_map_viewport_options_default();
  options.fields = MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
  options.constrain_mode = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options)
  );
  options = mln_map_viewport_options_default();
  options.fields = MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
  options.viewport_mode = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(fixture.map, &options)
  );
  destroy_map_fixture(fixture);
}

// This verifies raw tile-option struct sizes, enum discriminants, and output
// pointers.
static void map_tile_options_reject_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_tile_options(fixture.map, NULL)
  );
  mln_map_tile_options options = mln_map_tile_options_default();
  options.size = sizeof(mln_map_tile_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_tile_options(fixture.map, &options)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_tile_options(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_tile_options(fixture.map, &options)
  );
  options = mln_map_tile_options_default();
  options.fields = UINT32_C(1) << 31;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_tile_options(fixture.map, &options)
  );
  options = mln_map_tile_options_default();
  options.fields = MLN_MAP_TILE_OPTION_LOD_MODE;
  options.lod_mode = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_tile_options(fixture.map, &options)
  );
  destroy_map_fixture(fixture);
}

void run_map_options_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(camera_rejects_invalid_arguments);
  RUN_TEST(camera_fitting_rejects_invalid_arguments);
  RUN_TEST(camera_bounds_constraints_reject_invalid_arguments);
  RUN_TEST(free_camera_options_reject_raw_invalid_arguments);
  RUN_TEST(map_projection_mode_rejects_invalid_arguments);
  RUN_TEST(map_coordinate_conversion_rejects_invalid_arguments);
  RUN_TEST(standalone_projection_rejects_invalid_arguments);
  RUN_TEST(projected_meters_reject_invalid_arguments);
  RUN_TEST(map_debug_options_reject_raw_invalid_arguments);
  RUN_TEST(map_size_tracks_attach_and_resize);
  RUN_TEST(map_viewport_options_reject_invalid_arguments);
  RUN_TEST(map_tile_options_reject_invalid_arguments);
}
