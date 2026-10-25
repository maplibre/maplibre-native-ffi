// Standalone projections: a projection of a view converts as that view
// describes, needs no map, converts synchronously on any thread, and reads its
// scale at its own camera. The spherical Mercator helpers need no map at all.

#include <math.h>

#include "support/test_support.h"

static mln_camera_options read_camera(mln_map_projection projection) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &camera, NULL));
  return camera;
}

static void projection_outlives_its_source_map_and_runtime(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_map_projection projection = mln_test_map_create_projection(map);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  const mln_camera_options source_camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, source_camera.center.latitude);

  mln_map_projection_release(projection);

  // A second release is a no-op, and every other call with the retired handle
  // reports invalid state.
  mln_map_projection_release(projection);
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  MLN_TEST_INVALID_STATE(
    mln_map_projection_get_camera(projection, &camera, NULL)
  );
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 2.0;
  MLN_TEST_INVALID_STATE(
    mln_map_projection_set_camera(projection, &camera, NULL)
  );
  mln_screen_point point = {0};
  MLN_TEST_INVALID_STATE(mln_map_projection_pixel_for_lat_lng(
    projection, (mln_lat_lng){.latitude = 0.0, .longitude = 0.0}, &point, NULL
  ));
  const mln_lat_lng coordinate = {.latitude = 0.0, .longitude = 0.0};
  MLN_TEST_INVALID_STATE(mln_map_projection_pixels_for_lat_lngs(
    projection, &coordinate, 1, &point, NULL
  ));
  mln_lat_lng converted = {0};
  MLN_TEST_INVALID_STATE(mln_map_projection_lat_lngs_for_pixels(
    projection, &point, 1, &converted, NULL
  ));
  MLN_TEST_INVALID_STATE(mln_map_projection_lat_lngs_for_pixels_unwrapped(
    projection, &point, 1, &converted, NULL
  ));
}

static void creation_observes_earlier_map_camera_commands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera = (mln_camera_options){.size = sizeof(mln_camera_options)};
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center.latitude = 12.0;
  update.camera.center.longitude = 34.0;
  update.camera.zoom = 4.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );

  // The command published its snapshot before its completion ran, so a
  // projection of the snapshot's view observes the command.
  mln_map_projection projection = mln_test_map_create_projection(map);
  const mln_camera_options camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 12.0, camera.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 34.0, camera.center.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 4.0, camera.zoom);

  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_screen_point projected_pixel(
  mln_map_projection projection, mln_lat_lng coordinate
) {
  mln_screen_point point = {0};
  MLN_TEST_OK(
    mln_map_projection_pixel_for_lat_lng(projection, coordinate, &point, NULL)
  );
  return point;
}

static void projection_follows_committed_extent_viewport_and_mode(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_snapshot initial = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &initial, NULL));
  const mln_map_projection before = mln_test_map_create_projection(map);

  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_extent){512, 256, 1.0}, &completion.descriptor, NULL
  ));
  mln_camera_update padded = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  padded.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  padded.camera = (mln_camera_options){.size = sizeof(mln_camera_options)};
  padded.camera.fields = MLN_CAMERA_OPTION_PADDING;
  padded.camera.padding = (mln_edge_insets){.top = 40.0, .left = 100.0};
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &padded, &completion.descriptor, NULL)
  );
  mln_projection_mode mode = {.size = sizeof(mln_projection_mode)};
  mode.fields = MLN_PROJECTION_MODE_AXONOMETRIC;
  mode.axonometric = true;
  MLN_TEST_AWAIT_OK(
    mln_map_set_projection_mode(map, &mode, &completion.descriptor, NULL)
  );
  mln_map_viewport_options viewport = {
    .size = sizeof(mln_map_viewport_options)
  };
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  viewport.north_orientation = MLN_NORTH_ORIENTATION_RIGHT;
  MLN_TEST_AWAIT_OK(
    mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
  );

  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  TEST_ASSERT_TRUE(snapshot.view.projection_mode.axonometric);
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_map_projection_create(&snapshot.view, &projection, NULL));

  // The projection rebuilds its transform from the view's degrees and meters,
  // so its camera agrees with the view within a tolerance rather than bytewise.
  const mln_camera_options camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, snapshot.view.camera.center.latitude, camera.center.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, snapshot.view.camera.center.longitude, camera.center.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, snapshot.view.camera.zoom, camera.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, snapshot.view.camera.bearing, camera.bearing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, snapshot.view.camera.pitch, camera.pitch);

  const mln_lat_lng center = snapshot.view.camera.center;
  // Perspective mode draws the center at the middle of the padded area, at
  // (306, 148). Axonometric mode ignores padding, so the center stays at the
  // middle of the extent only when the projection copied that mode.
  const mln_screen_point centered = projected_pixel(projection, center);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 256.0, centered.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 128.0, centered.y);
  // North points right, so a coordinate north of the center lies to its right.
  const mln_screen_point northward = projected_pixel(
    projection,
    (mln_lat_lng){
      .latitude = center.latitude + 10.0, .longitude = center.longitude
    }
  );
  TEST_ASSERT_GREATER_THAN_DOUBLE(centered.x + 1.0, northward.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, centered.y, northward.y);

  // A projection created before the commands keeps the extent of its view.
  const mln_screen_point earlier = projected_pixel(before, center);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, initial.view.extent.width / 2.0, earlier.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, initial.view.extent.height / 2.0, earlier.y);

  mln_map_projection_release(projection);
  mln_map_projection_release(before);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct projection_call {
  mln_map_view view;
  bool null_view;
  bool null_output;
  mln_map_projection preset;
} projection_call;

static void null_view(void* call) {
  ((projection_call*)call)->null_view = true;
}
static void undersized(void* call) {
  ((projection_call*)call)->view.size = sizeof(mln_map_view) - 1;
}
static void latitude_past_the_pole(void* call) {
  ((projection_call*)call)->view.camera.center.latitude = 90.5;
}
static void negative_padding(void* call) {
  ((projection_call*)call)->view.camera.padding.left = -1.0;
}
static void nan_bearing(void* call) {
  ((projection_call*)call)->view.camera.bearing = NAN;
}
static void zoom_past_the_limit(void* call) {
  ((projection_call*)call)->view.camera.zoom = 26.0;
}
static void zero_width(void* call) {
  ((projection_call*)call)->view.extent.width = 0;
}
static void zero_scale_factor(void* call) {
  ((projection_call*)call)->view.extent.scale_factor = 0.0;
}
static void unset_skew(void* call) {
  ((projection_call*)call)->view.projection_mode.fields &=
    ~MLN_PROJECTION_MODE_X_SKEW;
}
static void unset_frustum_offset(void* call) {
  ((projection_call*)call)->view.viewport.fields &=
    ~MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
}
static void unknown_orientation(void* call) {
  ((projection_call*)call)->view.viewport.north_orientation = 9;
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
  const mln_status status = mln_map_projection_create(
    call->null_view ? NULL : &call->view,
    call->null_output ? NULL : &projection, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    mln_map_projection_release(projection);
  } else {
    TEST_ASSERT_EQUAL_UINT64(call->preset, projection);
  }
  return status;
}

// A view is complete: every field it carries is required, and each must be
// one a transform can hold.
static void creation_rejects_incomplete_or_invalid_views(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  const projection_call defaults = {.view = snapshot.view};
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);

  static const mln_test_validation_case cases[] = {
    {"a snapshot's view", NULL, MLN_STATUS_OK, NULL},
    {"null view", null_view, MLN_STATUS_INVALID_ARGUMENT, "view"},
    {"undersized view", undersized, MLN_STATUS_INVALID_ARGUMENT, "size"},
    {"latitude past the pole", latitude_past_the_pole,
     MLN_STATUS_INVALID_ARGUMENT, "within [-90, 90]"},
    {"negative padding", negative_padding, MLN_STATUS_INVALID_ARGUMENT,
     "padding"},
    {"non-finite bearing", nan_bearing, MLN_STATUS_INVALID_ARGUMENT, "finite"},
    {"zoom past the limit", zoom_past_the_limit, MLN_STATUS_INVALID_ARGUMENT,
     "limits"},
    {"zero width", zero_width, MLN_STATUS_INVALID_ARGUMENT, "extent"},
    {"zero scale factor", zero_scale_factor, MLN_STATUS_INVALID_ARGUMENT,
     "extent"},
    {"unset skew", unset_skew, MLN_STATUS_INVALID_ARGUMENT, "every field"},
    {"unset frustum offset", unset_frustum_offset, MLN_STATUS_INVALID_ARGUMENT,
     "every field"},
    {"unknown orientation", unknown_orientation, MLN_STATUS_INVALID_ARGUMENT,
     "north_orientation"},
    {"null output", null_output, MLN_STATUS_INVALID_ARGUMENT, "out_projection"},
    {"occupied output", occupied_output, MLN_STATUS_INVALID_ARGUMENT,
     "out_projection"},
  };
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_projection, NULL
  );
}

// A view carries no camera constraints, so a camera that the map's bounds
// would clamp applies to the projection as it is.
static void a_view_projection_ignores_the_maps_constraints(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_bound_options bounds = {.size = sizeof(mln_bound_options)};
  bounds.fields = MLN_BOUND_OPTION_MAX_ZOOM | MLN_BOUND_OPTION_MAX_PITCH;
  bounds.max_zoom = 4.0;
  bounds.max_pitch = 10.0;
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  mln_map_projection projection = mln_test_map_create_projection(map);

  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_PITCH;
  camera.zoom = 24.0;
  camera.pitch = 70.0;
  MLN_TEST_OK(mln_map_projection_set_camera(projection, &camera, NULL));
  const mln_camera_options applied = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 24.0, applied.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 70.0, applied.pitch);

  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void setters_apply_before_return(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = mln_test_map_create_projection(map);

  // A camera passed by pointer versions itself, unlike one embedded in an
  // update.
  mln_camera_options too_small = {.size = sizeof(mln_camera_options) - 1};
  MLN_TEST_INVALID(mln_map_projection_get_camera(projection, &too_small, NULL));
  MLN_TEST_INVALID(mln_map_projection_get_camera(projection, NULL, NULL));
  MLN_TEST_INVALID(mln_map_projection_set_camera(projection, &too_small, NULL));

  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.center.latitude = 20.0;
  camera.center.longitude = 40.0;
  camera.zoom = 6.0;
  MLN_TEST_OK(mln_map_projection_set_camera(projection, &camera, NULL));
  const mln_camera_options committed = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 20.0, committed.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 40.0, committed.center.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 6.0, committed.zoom);

  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_lat_lng coordinate_for_pixel(
  mln_map_projection projection, mln_screen_point point, bool unwrapped
) {
  mln_lat_lng coordinate = {0};
  MLN_TEST_OK(
    unwrapped ? mln_map_projection_lat_lng_for_pixel_unwrapped(
                  projection, point, &coordinate, NULL
                )
              : mln_map_projection_lat_lng_for_pixel(
                  projection, point, &coordinate, NULL
                )
  );
  return coordinate;
}

// Converts a batch and checks that each element agrees with the single call.
static void coordinates_for_pixels(
  mln_map_projection projection, const mln_screen_point* points, size_t count,
  bool unwrapped, mln_lat_lng* out_coordinates
) {
  MLN_TEST_OK(
    unwrapped ? mln_map_projection_lat_lngs_for_pixels_unwrapped(
                  projection, points, count, out_coordinates, NULL
                )
              : mln_map_projection_lat_lngs_for_pixels(
                  projection, points, count, out_coordinates, NULL
                )
  );
  for (size_t index = 0; index < count; ++index) {
    const mln_lat_lng single =
      coordinate_for_pixel(projection, points[index], unwrapped);
    TEST_ASSERT_EQUAL_DOUBLE(single.latitude, out_coordinates[index].latitude);
    TEST_ASSERT_EQUAL_DOUBLE(
      single.longitude, out_coordinates[index].longitude
    );
  }
}

static void jump_to(mln_map map, double longitude, double zoom) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center.latitude = 0.0;
  update.camera.center.longitude = longitude;
  update.camera.zoom = zoom;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// A viewport wider than one world copy, or one across the antimeridian, shows
// longitudes that the wrapped conversions fold into -180 to 180 and that the
// unwrapped ones keep in the visible copy. Single and batch conversions agree.
static void unwrapped_conversion_preserves_world_copies(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.initial_extent.width = 1024;
  options.initial_extent.height = 512;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  jump_to(map, 179.0, 0.0);
  mln_map_projection projection = mln_test_map_create_projection(map);

  // At zoom 0 the world is 512 pixels wide, so the viewport shows two copies.
  const mln_screen_point points[] = {
    {.x = 0.0, .y = 256.0},
    {.x = 512.0, .y = 256.0},
    {.x = 1024.0, .y = 256.0},
  };
  mln_lat_lng wrapped[3] = {0};
  mln_lat_lng unwrapped[3] = {0};
  coordinates_for_pixels(projection, points, 3, false, wrapped);
  coordinates_for_pixels(projection, points, 3, true, unwrapped);
  for (size_t index = 0; index < 3; index++) {
    TEST_ASSERT_TRUE(wrapped[index].longitude >= -180.0);
    TEST_ASSERT_TRUE(wrapped[index].longitude <= 180.0);
  }
  TEST_ASSERT_DOUBLE_WITHIN(1e-10, 179.0, unwrapped[1].longitude);
  TEST_ASSERT_TRUE(unwrapped[2].longitude - unwrapped[0].longitude > 360.0);

  const mln_lat_lng wrapped_right =
    coordinate_for_pixel(projection, points[2], false);
  TEST_ASSERT_TRUE(wrapped_right.longitude >= -180.0);
  TEST_ASSERT_TRUE(wrapped_right.longitude <= 180.0);
  const mln_lat_lng unwrapped_right =
    coordinate_for_pixel(projection, points[2], true);
  TEST_ASSERT_EQUAL_DOUBLE(unwrapped[2].longitude, unwrapped_right.longitude);

  // The right edge sits a full world east of the left edge, so the wrapped
  // conversion folds it back across the antimeridian and the unwrapped one
  // keeps the visible copy.
  TEST_ASSERT_TRUE(unwrapped_right.longitude > 180.0);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, wrapped_right.latitude, unwrapped_right.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, wrapped_right.longitude, unwrapped_right.longitude - 360.0
  );

  // At zoom 2 the viewport spans less than a world but still crosses the
  // antimeridian, so its edges unwrap onto either side of 180.
  jump_to(map, 179.0, 2.0);
  mln_map_projection_release(projection);
  projection = mln_test_map_create_projection(map);
  const mln_screen_point edges[] = {points[0], points[2]};
  mln_lat_lng edges_wrapped[2] = {0};
  mln_lat_lng edges_unwrapped[2] = {0};
  coordinates_for_pixels(projection, edges, 2, false, edges_wrapped);
  coordinates_for_pixels(projection, edges, 2, true, edges_unwrapped);
  TEST_ASSERT_TRUE(edges_unwrapped[0].longitude < edges_unwrapped[1].longitude);
  TEST_ASSERT_TRUE(edges_unwrapped[0].longitude < 180.0);
  TEST_ASSERT_TRUE(edges_unwrapped[1].longitude > 180.0);
  TEST_ASSERT_TRUE(
    edges_unwrapped[1].longitude - edges_unwrapped[0].longitude < 360.0
  );
  TEST_ASSERT_TRUE(edges_wrapped[0].longitude > 0.0);
  TEST_ASSERT_TRUE(edges_wrapped[1].longitude < 0.0);

  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static double projection_meters_per_pixel(
  mln_map_projection projection, double latitude
) {
  double meters = 0.0;
  MLN_TEST_OK(mln_map_projection_meters_per_pixel_at_latitude(
    projection, latitude, &meters, NULL
  ));
  return meters;
}

static double snapshot_meters_per_pixel(mln_map map, double latitude) {
  mln_map_projection projection = mln_test_map_create_projection(map);
  double meters = projection_meters_per_pixel(projection, latitude);
  mln_map_projection_release(projection);
  return meters;
}

// A projection reads the scale at its own zoom: the map's scale when it was
// created, the same scale after the map zooms, and a halved scale once the
// projection's own camera zooms in by one.
static void meters_per_pixel_follows_the_projection_camera(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 3.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  mln_map_projection projection = mln_test_map_create_projection(map);

  const double at_creation = snapshot_meters_per_pixel(map, 45.0);
  TEST_ASSERT_EQUAL_DOUBLE(
    at_creation, projection_meters_per_pixel(projection, 45.0)
  );
  TEST_ASSERT_GREATER_THAN_DOUBLE(
    at_creation, projection_meters_per_pixel(projection, 0.0)
  );

  update.camera.zoom = 4.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    at_creation * 1e-9, at_creation / 2.0, snapshot_meters_per_pixel(map, 45.0)
  );
  TEST_ASSERT_EQUAL_DOUBLE(
    at_creation, projection_meters_per_pixel(projection, 45.0)
  );

  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 4.0;
  MLN_TEST_OK(mln_map_projection_set_camera(projection, &camera, NULL));
  TEST_ASSERT_DOUBLE_WITHIN(
    at_creation * 1e-9, at_creation / 2.0,
    projection_meters_per_pixel(projection, 45.0)
  );

  double untouched = -1.0;
  MLN_TEST_INVALID(mln_map_projection_meters_per_pixel_at_latitude(
    projection, 90.5, &untouched, NULL
  ));
  MLN_TEST_INVALID(mln_map_projection_meters_per_pixel_at_latitude(
    projection, NAN, &untouched, NULL
  ));
  TEST_ASSERT_EQUAL_DOUBLE(-1.0, untouched);
  MLN_TEST_INVALID(
    mln_map_projection_meters_per_pixel_at_latitude(projection, 0.0, NULL, NULL)
  );

  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The origin projects to zero, northing and easting take the signs of latitude
// and longitude, and a coordinate converts back to itself. The scale is
// MapLibre's own earth radius, so no case pins it.
static void projected_meters_round_trip(void) {
  mln_projected_meters meters = {0};
  MLN_TEST_OK(
    mln_projected_meters_for_lat_lng((mln_lat_lng){0}, &meters, NULL)
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, meters.northing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, meters.easting);

  // Signs that differ, so swapped northing and easting would show.
  const mln_lat_lng coordinate = {.latitude = 37.7749, .longitude = -122.4194};
  MLN_TEST_OK(mln_projected_meters_for_lat_lng(coordinate, &meters, NULL));
  TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, meters.northing);
  TEST_ASSERT_LESS_THAN_DOUBLE(0.0, meters.easting);
  mln_lat_lng round_trip = {0};
  MLN_TEST_OK(mln_lat_lng_for_projected_meters(meters, &round_trip, NULL));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, coordinate.latitude, round_trip.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, coordinate.longitude, round_trip.longitude);
}

static void projected_meters_reject_invalid_arguments(void) {
  const mln_lat_lng center = {.latitude = 37.7749, .longitude = -122.4194};
  mln_projected_meters meters = {.northing = -1.0, .easting = -1.0};
  MLN_TEST_INVALID(mln_projected_meters_for_lat_lng(center, NULL, NULL));
  MLN_TEST_INVALID(mln_projected_meters_for_lat_lng(
    (mln_lat_lng){.latitude = 90.5}, &meters, NULL
  ));
  MLN_TEST_INVALID(mln_projected_meters_for_lat_lng(
    (mln_lat_lng){.longitude = INFINITY}, &meters, NULL
  ));
  TEST_ASSERT_EQUAL_DOUBLE(-1.0, meters.northing);
  MLN_TEST_INVALID(
    mln_lat_lng_for_projected_meters((mln_projected_meters){0}, NULL, NULL)
  );
  mln_lat_lng coordinate = {.latitude = -1.0};
  MLN_TEST_INVALID(mln_lat_lng_for_projected_meters(
    (mln_projected_meters){.northing = NAN}, &coordinate, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "must be finite"));
  TEST_ASSERT_EQUAL_DOUBLE(-1.0, coordinate.latitude);
}

typedef struct projection_thread_probe {
  mln_map_projection projection;
  mln_status set_camera_status;
  mln_status get_camera_status;
  double observed_zoom;
  mln_status pixel_status;
  mln_status coordinate_status;
  double round_trip_latitude;
} projection_thread_probe;

static void projection_foreign_thread(void* argument) {
  projection_thread_probe* probe = argument;
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.center.latitude = 15.0;
  camera.center.longitude = 25.0;
  camera.zoom = 3.0;
  probe->set_camera_status =
    mln_map_projection_set_camera(probe->projection, &camera, NULL);

  camera = (mln_camera_options){.size = sizeof(mln_camera_options)};
  probe->get_camera_status =
    mln_map_projection_get_camera(probe->projection, &camera, NULL);
  probe->observed_zoom = camera.zoom;

  mln_screen_point pixel = {0};
  probe->pixel_status = mln_map_projection_pixel_for_lat_lng(
    probe->projection, (mln_lat_lng){.latitude = 15.0, .longitude = 25.0},
    &pixel, NULL
  );
  mln_lat_lng coordinate = {0};
  probe->coordinate_status = mln_map_projection_lat_lng_for_pixel(
    probe->projection, pixel, &coordinate, NULL
  );
  probe->round_trip_latitude = coordinate.latitude;

  mln_map_projection_release(probe->projection);
}

static void projection_handles_are_callable_from_foreign_threads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  projection_thread_probe probe = {
    .projection = mln_test_map_create_projection(map),
    .set_camera_status = MLN_STATUS_INVALID_STATE,
    .get_camera_status = MLN_STATUS_INVALID_STATE,
    .observed_zoom = 0.0,
    .pixel_status = MLN_STATUS_INVALID_STATE,
    .coordinate_status = MLN_STATUS_INVALID_STATE,
    .round_trip_latitude = 0.0,
  };
  mln_test_thread* thread =
    mln_test_thread_start(projection_foreign_thread, &probe);
  mln_test_thread_join(thread);

  MLN_TEST_OK(probe.set_camera_status);
  MLN_TEST_OK(probe.get_camera_status);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 3.0, probe.observed_zoom);
  MLN_TEST_OK(probe.pixel_status);
  MLN_TEST_OK(probe.coordinate_status);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 15.0, probe.round_trip_latitude);
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  MLN_TEST_INVALID_STATE(
    mln_map_projection_get_camera(probe.projection, &camera, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A rejected camera update leaves the projection's camera as it was.
static void projection_setters_reject_invalid_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = mln_test_map_create_projection(map);
  const mln_camera_options before = read_camera(projection);

  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = NAN;
  MLN_TEST_INVALID(
    mln_map_projection_set_camera(projection, &camera, MLN_TEST_DIAGNOSTIC)
  );
  const mln_camera_options after = read_camera(projection);
  TEST_ASSERT_EQUAL_DOUBLE(before.center.latitude, after.center.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(before.center.longitude, after.center.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(before.zoom, after.zoom);
  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Invalid inputs leave the output untouched. A batch validates its whole input
// before it writes any output, so an invalid last element leaves every output
// as it was.
static void conversions_reject_invalid_inputs_without_writing_outputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = mln_test_map_create_projection(map);
  const mln_screen_point before = {.x = -1.0, .y = -2.0};
  mln_screen_point output = before;
  MLN_TEST_INVALID(mln_map_projection_pixel_for_lat_lng(
    projection, (mln_lat_lng){91.0, 0.0}, &output, NULL
  ));
  MLN_TEST_INVALID(mln_map_projection_pixel_for_lat_lng(
    projection, (mln_lat_lng){0.0, 0.0}, NULL, NULL
  ));
  TEST_ASSERT_EQUAL_MEMORY(&before, &output, sizeof(output));

  const mln_lat_lng original = {-1.0, -2.0};
  mln_lat_lng converted = original;
  MLN_TEST_INVALID(mln_map_projection_lat_lng_for_pixel(
    projection, (mln_screen_point){0.0, NAN}, &converted, NULL
  ));
  MLN_TEST_INVALID(mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection, (mln_screen_point){0.0, NAN}, &converted, NULL
  ));
  MLN_TEST_INVALID(mln_map_projection_lat_lng_for_pixel(
    projection, (mln_screen_point){0.0, 0.0}, NULL, NULL
  ));
  MLN_TEST_INVALID(mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection, (mln_screen_point){0.0, 0.0}, NULL, NULL
  ));
  TEST_ASSERT_EQUAL_MEMORY(&original, &converted, sizeof(converted));

  const mln_lat_lng coordinates[] = {{0.0, 0.0}, {10.0, 20.0}, {91.0, 0.0}};
  mln_screen_point points[] = {{-1.0, -2.0}, {-3.0, -4.0}, {-5.0, -6.0}};
  const mln_screen_point points_before[3] = {
    {-1.0, -2.0}, {-3.0, -4.0}, {-5.0, -6.0}
  };
  MLN_TEST_INVALID(mln_map_projection_pixels_for_lat_lngs(
    projection, coordinates, 3, points, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "latitude"));
  MLN_TEST_INVALID(mln_map_projection_pixels_for_lat_lngs(
    projection, NULL, 1, points, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "coordinates"));
  MLN_TEST_INVALID(mln_map_projection_pixels_for_lat_lngs(
    projection, coordinates, 1, NULL, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "out_points"));
  MLN_TEST_INVALID(mln_map_projection_pixels_for_lat_lngs(
    MLN_HANDLE_NULL, coordinates, 1, points, NULL
  ));
  TEST_ASSERT_EQUAL_MEMORY(points_before, points, sizeof(points));

  typedef mln_status (*convert_points)(
    mln_map_projection, const mln_screen_point*, size_t, mln_lat_lng*,
    mln_diagnostic*
  );
  const convert_points conversions[] = {
    mln_map_projection_lat_lngs_for_pixels,
    mln_map_projection_lat_lngs_for_pixels_unwrapped,
  };
  const mln_screen_point invalid[] = {{0.0, 0.0}, {20.0, 10.0}, {0.0, NAN}};
  mln_lat_lng outputs[] = {{-1.0, -2.0}, {-3.0, -4.0}, {-5.0, -6.0}};
  const mln_lat_lng outputs_before[3] = {
    {-1.0, -2.0}, {-3.0, -4.0}, {-5.0, -6.0}
  };
  for (size_t index = 0; index < 2; ++index) {
    MLN_TEST_INVALID(
      conversions[index](projection, invalid, 3, outputs, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "finite"));
    MLN_TEST_INVALID(
      conversions[index](projection, NULL, 1, outputs, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "points"));
    MLN_TEST_INVALID(
      conversions[index](projection, invalid, 1, NULL, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "out_coordinates"));
    MLN_TEST_INVALID(
      conversions[index](MLN_HANDLE_NULL, invalid, 1, outputs, NULL)
    );
  }
  TEST_ASSERT_EQUAL_MEMORY(outputs_before, outputs, sizeof(outputs));

  mln_map_projection_release(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(conversions_reject_invalid_inputs_without_writing_outputs);
  RUN_TEST(projection_outlives_its_source_map_and_runtime);
  RUN_TEST(creation_observes_earlier_map_camera_commands);
  RUN_TEST(projection_follows_committed_extent_viewport_and_mode);
  RUN_TEST(creation_rejects_incomplete_or_invalid_views);
  RUN_TEST(a_view_projection_ignores_the_maps_constraints);
  RUN_TEST(setters_apply_before_return);
  RUN_TEST(projection_setters_reject_invalid_values);
  RUN_TEST(unwrapped_conversion_preserves_world_copies);
  RUN_TEST(meters_per_pixel_follows_the_projection_camera);
  RUN_TEST(projection_handles_are_callable_from_foreign_threads);
  RUN_TEST(projected_meters_round_trip);
  RUN_TEST(projected_meters_reject_invalid_arguments);
}
