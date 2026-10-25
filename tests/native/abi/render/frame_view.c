// A rendered frame's view: the transform that the frame drew, which builds a
// projection that places a coordinate on the pixel where the frame drew it,
// however the camera, the target, and the map's constraints change later. It
// also shows what a frame gated on a command's render update draws.

#include <math.h>

#include "support/frames.h"
#include "support/style.h"
#include "support/test_support.h"

enum { side = 64 };

// One red circle on black, in a source tiled to its highest zoom so that tile
// quantization stays far below a pixel, and tiled inline so that every frame
// draws it.
static const char circle_style[] =
  "{\"version\":8,\"sources\":{\"dot\":{\"type\":\"geojson\","
  "\"maxzoom\":24,\"data\":{\"type\":\"Point\",\"coordinates\":[20,10]}}},"
  "\"layers\":[{\"id\":\"base\",\"type\":\"background\",\"paint\":"
  "{\"background-color\":\"#000000\"}},{\"id\":\"dot\",\"type\":\"circle\","
  "\"source\":\"dot\",\"paint\":{\"circle-color\":\"#ff0000\","
  "\"circle-radius\":4}}],\"transition\":{\"duration\":0,\"delay\":0}}";

static const mln_lat_lng dot = {.latitude = 10.0, .longitude = 20.0};

static void prepare(mln_map map) {
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(circle_style));
  MLN_TEST_AWAIT_OK(mln_map_set_geojson_source_synchronous_tiling(
    map, MLN_BUFFER_LITERAL("dot"), true, &completion.descriptor, NULL
  ));
}

// Requests one forced frame and returns its result.
static mln_render_frame_result render_frame(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_map_view), result.view.size);
  return result;
}

// Requests a demand that waits for the map's next update and returns the
// frame that draws it.
static mln_render_frame_result render_update(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = token;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  return result;
}

static mln_screen_point pixel_for(
  const mln_map_view* view, mln_lat_lng coordinate
) {
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK_MESSAGE(
    mln_map_projection_create(view, &projection, MLN_TEST_DIAGNOSTIC),
    mln_test_last_error()
  );
  mln_screen_point point = {0};
  MLN_TEST_OK(
    mln_map_projection_pixel_for_lat_lng(projection, coordinate, &point, NULL)
  );
  mln_map_projection_release(projection);
  return point;
}

// Reads the latest frame back and returns the centroid of its red pixels in
// logical pixels from the top-left corner, measured at pixel centers.
static mln_screen_point drawn_dot(
  const mln_test_render_fixture* fixture, const char* label
) {
  static uint8_t pixels[side * side * 4];
  mln_texture_image_info info = {0};
  MLN_TEST_OK(
    mln_test_render_read_back(fixture, &info, pixels, sizeof(pixels))
  );
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(side, info.width);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(side, info.height);
  double x = 0.0;
  double y = 0.0;
  size_t count = 0;
  for (uint32_t row = 0; row < info.height; row += 1) {
    for (uint32_t column = 0; column < info.width; column += 1) {
      const uint8_t* pixel = pixels + (size_t)row * info.stride + column * 4U;
      if (pixel[0] >= 128 && pixel[1] < 64 && pixel[2] < 64) {
        x += column + 0.5;
        y += row + 0.5;
        count += 1;
      }
    }
  }
  TEST_ASSERT_GREATER_THAN_size_t_MESSAGE(0, count, label);
  return (mln_screen_point){.x = x / (double)count, .y = y / (double)count};
}

typedef struct view_row {
  const char* label;
  // The camera center's offset from the dot, in degrees.
  double latitude_offset;
  double longitude_offset;
  double zoom;
  double bearing;
  double pitch;
  mln_edge_insets padding;
  uint32_t north_orientation;
  uint32_t viewport_mode;
  bool axonometric;
  double x_skew;
  double y_skew;
  // Bounds that let the center stay in another world copy, which the map
  // otherwise wraps into the first.
  bool world_copy;
} view_row;

// Applies a row through the map's own commands, under limits raised past the
// map's defaults of zoom 22 and pitch 60.
static void apply(mln_map map, const view_row* row) {
  mln_bound_options bounds = mln_bound_options_default();
  bounds.fields =
    MLN_BOUND_OPTION_MAX_ZOOM | MLN_BOUND_OPTION_MAX_PITCH |
    (row->world_copy ? MLN_BOUND_OPTION_BOUNDS : MLN_BOUND_OPTION_UNBOUNDED);
  bounds.max_zoom = 25.0;
  bounds.max_pitch = 85.0;
  bounds.bounds = (mln_lat_lng_bounds){
    .southwest = {.latitude = -80.0, .longitude = -900.0},
    .northeast = {.latitude = 80.0, .longitude = 900.0},
  };
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  mln_map_viewport_options viewport = mln_map_viewport_options_default();
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION |
                    MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
  viewport.north_orientation = row->north_orientation;
  viewport.viewport_mode = row->viewport_mode;
  MLN_TEST_AWAIT_OK(
    mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
  );
  mln_projection_mode mode = mln_projection_mode_default();
  mode.fields = MLN_PROJECTION_MODE_AXONOMETRIC | MLN_PROJECTION_MODE_X_SKEW |
                MLN_PROJECTION_MODE_Y_SKEW;
  mode.axonometric = row->axonometric;
  mode.x_skew = row->x_skew;
  mode.y_skew = row->y_skew;
  MLN_TEST_AWAIT_OK(
    mln_map_set_projection_mode(map, &mode, &completion.descriptor, NULL)
  );
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                         MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                         MLN_CAMERA_OPTION_PADDING;
  update.camera.center = (mln_lat_lng){
    .latitude = dot.latitude + row->latitude_offset,
    .longitude = dot.longitude + row->longitude_offset,
  };
  update.camera.zoom = row->zoom;
  update.camera.bearing = row->bearing;
  update.camera.pitch = row->pitch;
  update.camera.padding = row->padding;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// The longitude that `pixels` span at `zoom`.
static double degrees(double pixels, double zoom) {
  return pixels * 360.0 / (512.0 * pow(2.0, zoom));
}

// Expects `view` to place the dot within a pixel of `drawn`, and inverse
// conversion to preserve the visible world copy.
static void expect_matches(
  const mln_map_view* view, const view_row* row, mln_screen_point drawn
) {
  const mln_screen_point projected = pixel_for(view, dot);
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1.0, drawn.x, projected.x, row->label);
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1.0, drawn.y, projected.y, row->label);
  if (!row->world_copy) return;
  const mln_lat_lng visible_dot = {
    .latitude = dot.latitude,
    .longitude = dot.longitude + 360.0 * round(row->longitude_offset / 360.0),
  };
  const mln_screen_point unwrapped_projected = pixel_for(view, visible_dot);
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1.0, drawn.x, unwrapped_projected.x, row->label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1.0, drawn.y, unwrapped_projected.y, row->label
  );
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_map_projection_create(view, &projection, NULL));
  mln_lat_lng unwrapped = {0};
  MLN_TEST_OK(mln_map_projection_lat_lng_for_pixel_unwrapped(
    projection, drawn, &unwrapped, NULL
  ));
  mln_map_projection_release(projection);
  const double pixel = degrees(1.0, row->zoom);
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    pixel, visible_dot.longitude, unwrapped.longitude, row->label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    pixel, dot.latitude, unwrapped.latitude, row->label
  );
}

static const view_row rows[] = {
  {.label = "a plain camera",
   .latitude_offset = 0.015,
   .longitude_offset = 0.02,
   .zoom = 9.0},
  {.label = "a pitch above the default maximum",
   .latitude_offset = 0.01,
   .zoom = 9.0,
   .bearing = 30.0,
   .pitch = 75.0},
  {.label = "a zoom above the default maximum",
   .latitude_offset = -2e-7,
   .longitude_offset = 4e-7,
   .zoom = 23.5},
  {.label = "padding",
   .latitude_offset = -0.02,
   .longitude_offset = 0.03,
   .zoom = 9.0,
   .padding = {.top = 20.0, .left = 12.0}},
  {.label = "north to the left",
   .latitude_offset = 0.012,
   .longitude_offset = -0.01,
   .zoom = 9.0,
   .north_orientation = MLN_NORTH_ORIENTATION_LEFT},
  {.label = "a flipped viewport",
   .latitude_offset = 0.015,
   .longitude_offset = 0.01,
   .zoom = 9.0,
   .viewport_mode = MLN_VIEWPORT_MODE_FLIPPED_Y},
  // Axonometric mode ignores padding, which moves the dot only in perspective.
  {.label = "axonometric skew",
   .latitude_offset = 0.01,
   .longitude_offset = -0.015,
   .zoom = 9.0,
   .pitch = 40.0,
   .padding = {.top = 24.0, .left = 16.0},
   .axonometric = true,
   .x_skew = 0.3,
   .y_skew = 0.8},
  {.label = "another world copy",
   .latitude_offset = -0.01,
   .longitude_offset = 360.015,
   .zoom = 9.0,
   .world_copy = true},
  {.label = "two worlds west",
   .latitude_offset = -0.01,
   .longitude_offset = -720.015,
   .zoom = 9.0,
   .world_copy = true},
  {.label = "two worlds east",
   .latitude_offset = -0.01,
   .longitude_offset = 720.015,
   .zoom = 9.0,
   .world_copy = true},
};

enum { row_count = sizeof(rows) / sizeof(rows[0]) };

// Each frame's view converts the dot to where the frame drew it, across every
// part of the transform that moves a pixel. Every view still converts as it
// did after later frames, a resize, and bounds that exclude its camera, since
// the projection takes the view as it is.
static void a_frame_view_places_coordinates_where_the_frame_drew_them(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_map_view views[row_count];
  mln_screen_point drawn[row_count];
  for (size_t index = 0; index < row_count; index += 1) {
    const view_row* row = &rows[index];
    apply(map, row);
    mln_test_render_until_idle(runtime, map, &fixture, NULL);
    const mln_render_frame_result frame = render_frame(&fixture, index + 1);
    drawn[index] = drawn_dot(&fixture, row->label);
    expect_matches(&frame.view, row, drawn[index]);
    views[index] = frame.view;
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, row->zoom, frame.view.camera.zoom, row->label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, row->pitch, frame.view.camera.pitch, row->label
    );
  }
  // The world-copy row's center stayed east of the antimeridian.
  TEST_ASSERT_GREATER_THAN_DOUBLE(
    180.0, views[row_count - 1].camera.center.longitude
  );

  // An unforced demand with nothing new to draw reports no view.
  mln_frame_demand demand = mln_frame_demand_default();
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result unchanged =
    mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, unchanged.disposition);
  TEST_ASSERT_EQUAL_UINT32(0, unchanged.view.size);

  const mln_logical_extent resized = {
    .width = 48,
    .height = 32,
    .scale_factor = 1.0,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session, resized, &completion.descriptor, NULL
    )
  );
  mln_bound_options bounds = mln_bound_options_default();
  bounds.fields = MLN_BOUND_OPTION_BOUNDS | MLN_BOUND_OPTION_MAX_ZOOM;
  bounds.bounds = (mln_lat_lng_bounds){
    .southwest = {.latitude = -60.0, .longitude = -100.0},
    .northeast = {.latitude = -50.0, .longitude = -90.0},
  };
  bounds.max_zoom = 12.0;
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  const mln_render_frame_result later = render_frame(&fixture, 100);
  TEST_ASSERT_EQUAL_UINT32(48, later.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(32, later.view.extent.height);
  for (size_t index = 0; index < row_count; index += 1) {
    expect_matches(&views[index], &rows[index], drawn[index]);
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// During an eased pan, each frame's view matches the drawn camera as it moves.
static void each_frame_of_a_transition_reports_its_own_view(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  prepare(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  apply(map, &rows[0]);
  mln_test_render_until_idle(runtime, map, &fixture, NULL);

  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_CENTER;
  update.camera.center = (mln_lat_lng){
    .latitude = dot.latitude - 0.015, .longitude = dot.longitude - 0.02
  };
  // Far longer than the case, so that every frame below lands mid-pan.
  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_EASING;
  update.animation.duration_ms = 600000;
  update.animation.easing = (mln_unit_bezier){0.0, 0.0, 1.0, 1.0};
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  mln_map_view previous = {0};
  size_t distinct_views = 0;
  const mln_test_deadline deadline = mln_test_deadline_default();
  for (uint64_t token = 1;
       distinct_views < 3 && !mln_test_deadline_passed(deadline); token += 1) {
    const mln_render_frame_result frame = render_update(&fixture, token);
    expect_matches(&frame.view, &rows[0], drawn_dot(&fixture, rows[0].label));
    if (
      frame.view.camera.center.latitude != previous.camera.center.latitude ||
      frame.view.camera.center.longitude != previous.camera.center.longitude
    ) {
      distinct_views += 1;
    }
    previous = frame.view;
  }
  TEST_ASSERT_EQUAL_size_t(3, distinct_views);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Requests render-if-needed demands that wait for a map update until one
// renders an update at or past `generation`, as a host gates presentation on a
// command, and returns that frame.
static mln_render_frame_result render_through_update(
  const mln_test_render_fixture* fixture, uint64_t generation
) {
  const mln_test_deadline deadline = mln_test_deadline_default();
  for (uint64_t token = 1; !mln_test_deadline_passed(deadline); token += 1) {
    const mln_render_frame_result result = render_update(fixture, token);
    if (result.map_update_generation >= generation) return result;
  }
  TEST_FAIL_MESSAGE("no frame rendered the command's render update");
  return (mln_render_frame_result){0};
}

// A committed command's completion names the render update that includes it,
// and the first frame that renders that update or a later one draws it.
static void a_frame_at_a_commands_render_update_draws_the_command(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  render_frame(&fixture, 1);
  mln_render_session_snapshot before = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(mln_render_session_get_snapshot(fixture.session, &before, NULL));

  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center.latitude = -21.0;
  update.camera.center.longitude = 43.0;
  update.camera.zoom = 2.0;
  mln_test_completion jump = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_update_camera(map, &update, &jump.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&jump));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, mln_test_completion_disposition(&jump)
  );
  const uint64_t render_update =
    mln_test_completion_render_update_generation(&jump);
  mln_test_completion_destroy(&jump);
  // The command published its render update before it completed.
  TEST_ASSERT_GREATER_THAN_UINT64(
    before.rendered_update_generation, render_update
  );

  const mln_render_frame_result frame =
    render_through_update(&fixture, render_update);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, -21.0, frame.view.camera.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 43.0, frame.view.camera.center.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 2.0, frame.view.camera.zoom);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A session resize reports the render update of the map's resize, and the
// first frame at that update draws the new extent.
static void a_session_resize_reports_the_render_update_of_its_extent(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  render_frame(&fixture, 1);

  const mln_logical_extent resized = {
    .width = 48,
    .height = 32,
    .scale_factor = 1.0,
  };
  mln_test_completion resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    fixture.session, resized, &resize.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &resize));
  const uint64_t render_update =
    mln_test_completion_render_update_generation(&resize);
  mln_test_completion_destroy(&resize);

  const mln_render_frame_result frame =
    render_through_update(&fixture, render_update);
  TEST_ASSERT_EQUAL_UINT32(48, frame.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(32, frame.view.extent.height);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_frame_view_places_coordinates_where_the_frame_drew_them);
  RUN_TEST(each_frame_of_a_transition_reports_its_own_view);
  RUN_TEST(a_frame_at_a_commands_render_update_draws_the_command);
  RUN_TEST(a_session_resize_reports_the_render_update_of_its_extent);
}
