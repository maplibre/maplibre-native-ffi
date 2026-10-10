// Standalone projections: a projection copies the transform the map published
// with its latest snapshot, outlives the map, converts and fits synchronously
// on any thread, and reads its scale at its own camera. The spherical Mercator
// helpers need no map at all.

#include <math.h>

#include "support/test_support.h"

static mln_map_projection create_projection(mln_map map) {
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_map_create_projection(map, &projection, NULL));
  return projection;
}

static mln_camera_options read_camera(mln_map_projection projection) {
  mln_camera_options camera = mln_camera_options_default();
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &camera, NULL));
  return camera;
}

static void projection_outlives_its_source_map_and_runtime(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_map_projection projection = create_projection(map);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  const mln_camera_options source_camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, source_camera.center.latitude);

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));

  // Every call with the retired handle reports invalid state.
  MLN_TEST_INVALID_STATE(mln_map_projection_close(projection, NULL));
  mln_camera_options camera = mln_camera_options_default();
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
}

static void creation_observes_earlier_map_camera_commands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera = mln_camera_options_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center.latitude = 12.0;
  update.camera.center.longitude = 34.0;
  update.camera.zoom = 4.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );

  // The command published its snapshot before its completion ran, so the
  // projection copies the committed transform state.
  mln_map_projection projection = create_projection(map);
  const mln_camera_options camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 12.0, camera.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 34.0, camera.center.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 4.0, camera.zoom);

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
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
  const mln_map_projection before = create_projection(map);

  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_extent){512, 256, 1.0}, &completion.descriptor, NULL
  ));
  mln_camera_update padded = mln_camera_update_default();
  padded.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  padded.camera = mln_camera_options_default();
  padded.camera.fields = MLN_CAMERA_OPTION_PADDING;
  padded.camera.padding = (mln_edge_insets){.top = 40.0, .left = 100.0};
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &padded, &completion.descriptor, NULL)
  );
  mln_projection_mode mode = mln_projection_mode_default();
  mode.fields = MLN_PROJECTION_MODE_AXONOMETRIC;
  mode.axonometric = true;
  MLN_TEST_AWAIT_OK(
    mln_map_set_projection_mode(map, &mode, &completion.descriptor, NULL)
  );
  mln_map_viewport_options viewport = mln_map_viewport_options_default();
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  viewport.north_orientation = MLN_NORTH_ORIENTATION_RIGHT;
  MLN_TEST_AWAIT_OK(
    mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
  );

  // The map has no style, so nothing publishes between these two reads.
  const mln_map_projection projection = create_projection(map);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  TEST_ASSERT_TRUE(snapshot.projection_mode.axonometric);

  // The two paths derive camera options independently, so they agree within a
  // tolerance rather than bytewise.
  const mln_camera_options camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, snapshot.camera.center.latitude, camera.center.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, snapshot.camera.center.longitude, camera.center.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, snapshot.camera.zoom, camera.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, snapshot.camera.bearing, camera.bearing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, snapshot.camera.pitch, camera.pitch);

  const mln_lat_lng center = snapshot.camera.center;
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

  // A projection created before the commands keeps the extent it copied.
  const mln_screen_point earlier = projected_pixel(before, center);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, initial.logical_extent.width / 2.0, earlier.x
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, initial.logical_extent.height / 2.0, earlier.y
  );

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  MLN_TEST_OK(mln_map_projection_close(before, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void creation_rejects_invalid_arguments_and_released_maps(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  MLN_TEST_INVALID(mln_map_create_projection(map, NULL, NULL));
  mln_map_projection occupied = 1;
  MLN_TEST_INVALID(mln_map_create_projection(map, &occupied, NULL));
  TEST_ASSERT_EQUAL_UINT64(1, occupied);
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(
    mln_map_create_projection(MLN_HANDLE_NULL, &projection, NULL)
  );

  mln_test_destroy_map(map);
  MLN_TEST_INVALID_STATE(mln_map_create_projection(map, &projection, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, projection);
  mln_test_destroy_runtime(runtime);
}

static void setters_apply_before_return_and_conversions_round_trip(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);

  // A camera passed by pointer versions itself, unlike one embedded in an
  // update.
  mln_camera_options too_small = {.size = sizeof(mln_camera_options) - 1};
  MLN_TEST_INVALID(mln_map_projection_get_camera(projection, &too_small, NULL));
  MLN_TEST_INVALID(mln_map_projection_get_camera(projection, NULL, NULL));
  MLN_TEST_INVALID(mln_map_projection_set_camera(projection, &too_small, NULL));

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.center.latitude = 20.0;
  camera.center.longitude = 40.0;
  camera.zoom = 6.0;
  MLN_TEST_OK(mln_map_projection_set_camera(projection, &camera, NULL));
  const mln_camera_options committed = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 20.0, committed.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 40.0, committed.center.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 6.0, committed.zoom);

  // A committed camera changes what later conversions observe: the new center
  // converts back to itself through a pixel round trip.
  mln_screen_point center_pixel = {0};
  MLN_TEST_OK(mln_map_projection_pixel_for_lat_lng(
    projection, (mln_lat_lng){.latitude = 20.0, .longitude = 40.0},
    &center_pixel, NULL
  ));
  mln_lat_lng round_trip = {0};
  MLN_TEST_OK(mln_map_projection_lat_lng_for_pixel(
    projection, center_pixel, &round_trip, NULL
  ));
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 20.0, round_trip.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 40.0, round_trip.longitude);
  // A coordinate off the globe is refused and leaves the output untouched.
  mln_screen_point untouched = center_pixel;
  MLN_TEST_INVALID(mln_map_projection_pixel_for_lat_lng(
    projection, (mln_lat_lng){.latitude = NAN, .longitude = 40.0}, &untouched,
    NULL
  ));
  TEST_ASSERT_EQUAL_DOUBLE(center_pixel.x, untouched.x);
  TEST_ASSERT_EQUAL_DOUBLE(center_pixel.y, untouched.y);

  const mln_lat_lng origin = {.latitude = 0.0, .longitude = 0.0};
  mln_screen_point origin_before_fit = {0};
  MLN_TEST_OK(mln_map_projection_pixel_for_lat_lng(
    projection, origin, &origin_before_fit, NULL
  ));

  const mln_lat_lng coordinates[] = {
    {.latitude = 0.0, .longitude = -10.0},
    {.latitude = 0.0, .longitude = 10.0},
  };
  MLN_TEST_OK(mln_map_projection_set_visible_coordinates(
    projection, coordinates, 2, (mln_edge_insets){0}, NULL
  ));
  const mln_camera_options fitted = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, fitted.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, fitted.center.longitude);
  mln_screen_point origin_after_fit = {0};
  MLN_TEST_OK(mln_map_projection_pixel_for_lat_lng(
    projection, origin, &origin_after_fit, NULL
  ));
  // The committed fit moved the camera, so the same coordinate lands on a
  // different pixel than it did before the fit.
  TEST_ASSERT_TRUE(origin_after_fit.x != origin_before_fit.x);

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_lat_lng map_lat_lng_for_pixel(
  mln_map map, mln_screen_point point, bool unwrapped
) {
  mln_test_completion query = mln_test_completion_default(sizeof(mln_lat_lng));
  MLN_TEST_OK(
    unwrapped
      ? mln_map_lat_lng_for_pixel_unwrapped(map, point, &query.descriptor, NULL)
      : mln_map_lat_lng_for_pixel(map, point, &query.descriptor, NULL)
  );
  mln_lat_lng coordinate = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &coordinate, sizeof(coordinate))
  );
  return coordinate;
}

static void map_lat_lngs_for_pixels(
  mln_map map, const mln_screen_point* points, size_t count, bool unwrapped,
  mln_lat_lng* out_coordinates
) {
  mln_test_completion query =
    mln_test_completion_default(count * sizeof(mln_lat_lng));
  MLN_TEST_OK(
    unwrapped
      ? mln_map_lat_lngs_for_pixels_unwrapped(
          map, points, count, &query.descriptor, NULL
        )
      : mln_map_lat_lngs_for_pixels(map, points, count, &query.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_completion_finish_value(
    &query, out_coordinates, count * sizeof(mln_lat_lng)
  ));
}

static void jump_to(mln_map map, double longitude, double zoom) {
  mln_camera_update update = mln_camera_update_default();
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
// unwrapped ones keep in the visible copy. The map's ordered conversions, in
// single and batch form, and a projection of the same camera agree.
static void unwrapped_conversion_preserves_world_copies(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 1024;
  options.initial_extent.height = 512;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  jump_to(map, 179.0, 0.0);
  mln_map_projection projection = create_projection(map);

  MLN_TEST_INVALID(mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection, (mln_screen_point){0}, NULL, NULL
  ));
  mln_lat_lng rejected = {0};
  MLN_TEST_INVALID(mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection, (mln_screen_point){.x = INFINITY}, &rejected, NULL
  ));

  // At zoom 0 the world is 512 pixels wide, so the viewport shows two copies.
  const mln_screen_point points[] = {
    {.x = 0.0, .y = 256.0},
    {.x = 512.0, .y = 256.0},
    {.x = 1024.0, .y = 256.0},
  };
  mln_lat_lng wrapped[3] = {0};
  mln_lat_lng unwrapped[3] = {0};
  map_lat_lngs_for_pixels(map, points, 3, false, wrapped);
  map_lat_lngs_for_pixels(map, points, 3, true, unwrapped);
  for (size_t index = 0; index < 3; index++) {
    TEST_ASSERT_TRUE(wrapped[index].longitude >= -180.0);
    TEST_ASSERT_TRUE(wrapped[index].longitude <= 180.0);
  }
  TEST_ASSERT_DOUBLE_WITHIN(1e-10, 179.0, unwrapped[1].longitude);
  TEST_ASSERT_TRUE(unwrapped[2].longitude - unwrapped[0].longitude > 360.0);

  const mln_lat_lng wrapped_right =
    map_lat_lng_for_pixel(map, points[2], false);
  TEST_ASSERT_TRUE(wrapped_right.longitude >= -180.0);
  TEST_ASSERT_TRUE(wrapped_right.longitude <= 180.0);
  const mln_lat_lng unwrapped_right =
    map_lat_lng_for_pixel(map, points[2], true);
  TEST_ASSERT_EQUAL_DOUBLE(unwrapped[2].longitude, unwrapped_right.longitude);

  // The right edge sits a full world east of the left edge, so the projection
  // folds it back across the antimeridian or keeps the visible copy, exactly
  // as the map does.
  mln_lat_lng projected = {0};
  MLN_TEST_OK(mln_map_projection_lat_lng_for_pixel(
    projection, points[2], &projected, NULL
  ));
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-10, wrapped_right.longitude, projected.longitude
  );
  mln_lat_lng projected_unwrapped = {0};
  MLN_TEST_OK(mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection, points[2], &projected_unwrapped, NULL
  ));
  TEST_ASSERT_TRUE(projected_unwrapped.longitude > 180.0);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-10, unwrapped_right.longitude, projected_unwrapped.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, projected.latitude, projected_unwrapped.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, projected.longitude, projected_unwrapped.longitude - 360.0
  );

  // At zoom 2 the viewport spans less than a world but still crosses the
  // antimeridian, so its edges unwrap onto either side of 180.
  jump_to(map, 179.0, 2.0);
  const mln_screen_point edges[] = {points[0], points[2]};
  mln_lat_lng edges_wrapped[2] = {0};
  mln_lat_lng edges_unwrapped[2] = {0};
  map_lat_lngs_for_pixels(map, edges, 2, false, edges_wrapped);
  map_lat_lngs_for_pixels(map, edges, 2, true, edges_unwrapped);
  TEST_ASSERT_TRUE(edges_unwrapped[0].longitude < edges_unwrapped[1].longitude);
  TEST_ASSERT_TRUE(edges_unwrapped[0].longitude < 180.0);
  TEST_ASSERT_TRUE(edges_unwrapped[1].longitude > 180.0);
  TEST_ASSERT_TRUE(
    edges_unwrapped[1].longitude - edges_unwrapped[0].longitude < 360.0
  );
  TEST_ASSERT_TRUE(edges_wrapped[0].longitude > 0.0);
  TEST_ASSERT_TRUE(edges_wrapped[1].longitude < 0.0);

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A geometry fit frames the coordinates the geometry carries, the same way a
// coordinate fit does. A geometry with nothing to frame leaves the camera
// where it was.
static void visible_geometry_fits_like_its_coordinates(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);
  const mln_edge_insets padding = {
    .top = 10.0, .left = 20.0, .bottom = 10.0, .right = 20.0
  };

  const mln_lat_lng corners[] = {
    {.latitude = -10.0, .longitude = -10.0},
    {.latitude = 10.0, .longitude = 20.0},
  };
  MLN_TEST_OK(mln_map_projection_set_visible_coordinates(
    projection, corners, 2, padding, NULL
  ));
  const mln_camera_options coordinate_fit = read_camera(projection);

  // Reset first, so the geometry fit has to move the camera to agree.
  mln_camera_options reset = mln_camera_options_default();
  reset.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  reset.zoom = 1.0;
  MLN_TEST_OK(mln_map_projection_set_camera(projection, &reset, NULL));
  MLN_TEST_OK(mln_map_projection_set_visible_geometry(
    projection,
    MLN_BUFFER_LITERAL(
      "{\"type\":\"MultiPoint\",\"coordinates\":[[-10,-10],[20,10]]}"
    ),
    padding, NULL
  ));
  const mln_camera_options geometry_fit = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, coordinate_fit.center.latitude, geometry_fit.center.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, coordinate_fit.center.longitude, geometry_fit.center.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, coordinate_fit.zoom, geometry_fit.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 5.0, geometry_fit.center.longitude);

  static const char* const unframeable[] = {
    "",
    "not json",
    "{\"type\":\"Feature\",\"geometry\":null,\"properties\":{}}",
    "{\"type\":\"GeometryCollection\",\"geometries\":[]}",
    "{\"type\":\"MultiPoint\",\"coordinates\":[]}",
  };
  for (size_t index = 0; index < sizeof(unframeable) / sizeof(*unframeable);
       index += 1) {
    const char* geometry = unframeable[index];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_map_projection_set_visible_geometry(
        projection, mln_test_buffer_view(geometry, strlen(geometry)), padding,
        NULL
      ),
      geometry
    );
  }
  MLN_TEST_INVALID(mln_map_projection_set_visible_geometry(
    projection,
    MLN_BUFFER_LITERAL("{\"type\":\"Point\",\"coordinates\":[0,0]}"),
    (mln_edge_insets){.top = NAN}, NULL
  ));
  const mln_camera_options unchanged = read_camera(projection);
  TEST_ASSERT_EQUAL_DOUBLE(geometry_fit.zoom, unchanged.zoom);
  TEST_ASSERT_EQUAL_DOUBLE(
    geometry_fit.center.longitude, unchanged.center.longitude
  );

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  MLN_TEST_INVALID_STATE(mln_map_projection_set_visible_geometry(
    projection,
    MLN_BUFFER_LITERAL("{\"type\":\"Point\",\"coordinates\":[0,0]}"), padding,
    NULL
  ));
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

static double map_meters_per_pixel(mln_map map, double latitude) {
  mln_test_completion query = mln_test_completion_default(sizeof(double));
  MLN_TEST_OK(
    mln_map_meters_per_pixel_at_latitude(map, latitude, &query.descriptor, NULL)
  );
  double meters = 0.0;
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &meters, sizeof(meters))
  );
  return meters;
}

// A projection reads the scale at its own zoom: the map's scale when it was
// created, the same scale after the map zooms, and a halved scale once the
// projection's own camera zooms in by one.
static void meters_per_pixel_follows_the_projection_camera(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 3.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  mln_map_projection projection = create_projection(map);

  const double at_creation = map_meters_per_pixel(map, 45.0);
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
    at_creation * 1e-9, at_creation / 2.0, map_meters_per_pixel(map, 45.0)
  );
  TEST_ASSERT_EQUAL_DOUBLE(
    at_creation, projection_meters_per_pixel(projection, 45.0)
  );

  mln_camera_options camera = mln_camera_options_default();
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

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The origin projects to zero, northing and easting take the signs of latitude
// and longitude, and every coordinate converts back to itself. The scale is
// MapLibre's own earth radius, so no case pins it.
static void projected_meters_round_trip(void) {
  mln_projected_meters meters = {0};
  MLN_TEST_OK(
    mln_projected_meters_for_lat_lng((mln_lat_lng){0}, &meters, NULL)
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, meters.northing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, meters.easting);

  const mln_lat_lng coordinates[] = {
    {.latitude = 37.7749, .longitude = -122.4194},
    {.latitude = -33.8688, .longitude = 151.2093},
    {.latitude = 60.0, .longitude = 0.5},
  };
  for (size_t index = 0; index < sizeof(coordinates) / sizeof(*coordinates);
       index += 1) {
    MLN_TEST_OK(
      mln_projected_meters_for_lat_lng(coordinates[index], &meters, NULL)
    );
    TEST_ASSERT_EQUAL(coordinates[index].latitude > 0.0, meters.northing > 0.0);
    TEST_ASSERT_EQUAL(coordinates[index].longitude > 0.0, meters.easting > 0.0);
    mln_lat_lng round_trip = {0};
    MLN_TEST_OK(mln_lat_lng_for_projected_meters(meters, &round_trip, NULL));
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-9, coordinates[index].latitude, round_trip.latitude
    );
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-9, coordinates[index].longitude, round_trip.longitude
    );
  }
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
  TEST_ASSERT_EQUAL_STRING(
    "projected meter values must be finite", mln_test_last_error()
  );
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
  mln_status close_status;
} projection_thread_probe;

static void projection_foreign_thread(void* argument) {
  projection_thread_probe* probe = argument;
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.center.latitude = 15.0;
  camera.center.longitude = 25.0;
  camera.zoom = 3.0;
  probe->set_camera_status =
    mln_map_projection_set_camera(probe->projection, &camera, NULL);

  camera = mln_camera_options_default();
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

  probe->close_status = mln_map_projection_close(probe->projection, NULL);
}

static void projection_handles_are_callable_from_foreign_threads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  projection_thread_probe probe = {
    .projection = create_projection(map),
    .set_camera_status = MLN_STATUS_INVALID_STATE,
    .get_camera_status = MLN_STATUS_INVALID_STATE,
    .observed_zoom = 0.0,
    .pixel_status = MLN_STATUS_INVALID_STATE,
    .coordinate_status = MLN_STATUS_INVALID_STATE,
    .round_trip_latitude = 0.0,
    .close_status = MLN_STATUS_INVALID_STATE,
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
  MLN_TEST_OK(probe.close_status);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A standalone projection validates what its setters take: a finite camera,
// coordinates in range, nonnegative padding, and at least one coordinate to
// fit. A rejected call leaves the projection's camera as it was.
static void projection_setters_reject_invalid_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);
  const mln_camera_options before = read_camera(projection);

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = NAN;
  MLN_TEST_INVALID(
    mln_map_projection_set_camera(projection, &camera, MLN_TEST_DIAGNOSTIC)
  );
  const mln_lat_lng past_the_pole[2] = {{91.0, 0.0}, {0.0, 10.0}};
  const mln_edge_insets no_padding = {0};
  MLN_TEST_INVALID(mln_map_projection_set_visible_coordinates(
    projection, past_the_pole, 2, no_padding, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "latitude"));
  const mln_lat_lng corners[2] = {{-10.0, -10.0}, {10.0, 10.0}};
  const mln_edge_insets negative_padding = {.top = -1.0};
  MLN_TEST_INVALID(mln_map_projection_set_visible_coordinates(
    projection, corners, 2, negative_padding, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_INVALID(mln_map_projection_set_visible_coordinates(
    projection, corners, 0, no_padding, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "coordinate_count must be greater than 0"),
    mln_test_last_error()
  );

  const mln_camera_options after = read_camera(projection);
  TEST_ASSERT_EQUAL_DOUBLE(before.center.latitude, after.center.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(before.center.longitude, after.center.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(before.zoom, after.zoom);
  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(projection_outlives_its_source_map_and_runtime);
  RUN_TEST(creation_observes_earlier_map_camera_commands);
  RUN_TEST(projection_follows_committed_extent_viewport_and_mode);
  RUN_TEST(creation_rejects_invalid_arguments_and_released_maps);
  RUN_TEST(setters_apply_before_return_and_conversions_round_trip);
  RUN_TEST(projection_setters_reject_invalid_values);
  RUN_TEST(unwrapped_conversion_preserves_world_copies);
  RUN_TEST(visible_geometry_fits_like_its_coordinates);
  RUN_TEST(meters_per_pixel_follows_the_projection_camera);
  RUN_TEST(projection_handles_are_callable_from_foreign_threads);
  RUN_TEST(projected_meters_round_trip);
  RUN_TEST(projected_meters_reject_invalid_arguments);
}
