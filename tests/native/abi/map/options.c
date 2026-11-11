// Map option commands, resize, and the generations their commits publish.

#include <math.h>

#include "support/test_support.h"

static mln_map_snapshot read_snapshot(mln_map map) {
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return snapshot;
}

// Waits for a submitted option command, expects it to commit with a generation
// past `*previous`, and returns the snapshot, which must carry that
// generation.
static mln_map_snapshot settle_commit(
  mln_map map, mln_test_completion* completion, uint64_t* previous,
  const char* label
) {
  MLN_TEST_OK_MESSAGE(mln_test_completion_finish(completion), label);
  const uint64_t committed = mln_test_completion_generation(completion);
  mln_test_completion_destroy(completion);
  TEST_ASSERT_GREATER_THAN_UINT64_MESSAGE(*previous, committed, label);
  *previous = committed;
  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64_MESSAGE(
    committed, snapshot.generation, label
  );
  return snapshot;
}

// Submits an option command through `expression`, which passes
// `&completion.descriptor`, and leaves the committed snapshot in `snapshot`.
#define COMMIT(label, expression)                                    \
  do {                                                               \
    mln_test_completion completion = mln_test_completion_default(0); \
    MLN_TEST_OK_MESSAGE((expression), (label));                      \
    snapshot = settle_commit(map, &completion, &previous, (label));  \
  } while (false)

// Every enum value is committed once, so each mapping to MapLibre and back is
// exercised. The free camera precedes the constraints, which would move its
// position.
static void every_option_command_round_trips_through_the_snapshot(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_snapshot snapshot = read_snapshot(map);
  uint64_t previous = snapshot.generation;

  const uint32_t every_debug_flag =
    MLN_MAP_DEBUG_TILE_BORDERS | MLN_MAP_DEBUG_PARSE_STATUS |
    MLN_MAP_DEBUG_TIMESTAMPS | MLN_MAP_DEBUG_COLLISION |
    MLN_MAP_DEBUG_OVERDRAW | MLN_MAP_DEBUG_STENCIL_CLIP |
    MLN_MAP_DEBUG_DEPTH_BUFFER;
  for (size_t pass = 0; pass < 2; pass += 1) {
    const uint32_t debug = pass == 0 ? every_debug_flag : 0;
    COMMIT(
      "debug flags",
      mln_map_set_debug_flags(map, debug, &completion.descriptor, NULL)
    );
    TEST_ASSERT_EQUAL_UINT32(debug, snapshot.debug_flags);
    const bool stats = pass == 0;
    COMMIT(
      "rendering stats view", mln_map_set_rendering_stats_view_enabled(
                                map, stats, &completion.descriptor, NULL
                              )
    );
    TEST_ASSERT_EQUAL(stats, snapshot.rendering_stats_view_enabled);
    // MapLibre Native has no globe, so the setting changes only the snapshot.
    const bool free_bearing = pass == 0;
    COMMIT(
      "globe free bearing", mln_map_set_globe_free_bearing_enabled(
                              map, free_bearing, &completion.descriptor, NULL
                            )
    );
    TEST_ASSERT_EQUAL(free_bearing, snapshot.globe_free_bearing_enabled);
    TEST_ASSERT_FALSE(snapshot.globe_rendering);
  }

  static const struct {
    const char* label;
    uint32_t field;
    uint32_t value;
  } viewport_rows[] = {
    {"north right", MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
     MLN_NORTH_ORIENTATION_RIGHT},
    {"north down", MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
     MLN_NORTH_ORIENTATION_DOWN},
    {"north left", MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
     MLN_NORTH_ORIENTATION_LEFT},
    {"north up", MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION,
     MLN_NORTH_ORIENTATION_UP},
    {"constrain none", MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
     MLN_CONSTRAIN_MODE_NONE},
    {"constrain width and height", MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
     MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT},
    {"constrain screen", MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
     MLN_CONSTRAIN_MODE_SCREEN},
    {"constrain height only", MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE,
     MLN_CONSTRAIN_MODE_HEIGHT_ONLY},
    {"viewport flipped y", MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE,
     MLN_VIEWPORT_MODE_FLIPPED_Y},
    {"viewport default", MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE,
     MLN_VIEWPORT_MODE_DEFAULT},
  };
  for (size_t index = 0;
       index < sizeof(viewport_rows) / sizeof(viewport_rows[0]); index += 1) {
    mln_map_viewport_options viewport = {
      .size = sizeof(mln_map_viewport_options)
    };
    viewport.fields = viewport_rows[index].field;
    viewport.north_orientation = viewport_rows[index].value;
    viewport.constrain_mode = viewport_rows[index].value;
    viewport.viewport_mode = viewport_rows[index].value;
    const char* label = viewport_rows[index].label;
    COMMIT(
      label,
      mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
    );
    const uint32_t field = viewport_rows[index].field;
    const uint32_t committed =
      field == MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
        ? snapshot.view.viewport.north_orientation
      : field == MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
        ? snapshot.view.viewport.constrain_mode
        : snapshot.view.viewport.viewport_mode;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      viewport_rows[index].value, committed, label
    );
  }
  mln_map_viewport_options viewport = {
    .size = sizeof(mln_map_viewport_options)
  };
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  viewport.frustum_offset = (mln_edge_insets){1.0, 2.0, 3.0, 4.0};
  COMMIT(
    "frustum offset",
    mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_MEMORY(
    &viewport.frustum_offset, &snapshot.view.viewport.frustum_offset,
    sizeof(viewport.frustum_offset)
  );

  // The engine's default is 60 degrees, which it keeps in radians.
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 60.0, snapshot.tile.lod_pitch_threshold);
  mln_map_tile_options tile = {.size = sizeof(mln_map_tile_options)};
  tile.fields =
    MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA |
    MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS | MLN_MAP_TILE_OPTION_LOD_SCALE |
    MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD |
    MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT | MLN_MAP_TILE_OPTION_LOD_MODE;
  tile.prefetch_zoom_delta = 3;
  tile.lod_min_radius = 2.5;
  tile.lod_scale = 1.5;
  tile.lod_pitch_threshold = 45.0;
  tile.lod_zoom_shift = -1.25;
  tile.lod_mode = MLN_TILE_LOD_MODE_DISTANCE;
  for (size_t pass = 0; pass < 2; pass += 1) {
    COMMIT(
      "tile options",
      mln_map_set_tile_options(map, &tile, &completion.descriptor, NULL)
    );
    TEST_ASSERT_EQUAL_UINT32(
      tile.prefetch_zoom_delta, snapshot.tile.prefetch_zoom_delta
    );
    TEST_ASSERT_EQUAL_DOUBLE(tile.lod_min_radius, snapshot.tile.lod_min_radius);
    TEST_ASSERT_EQUAL_DOUBLE(tile.lod_scale, snapshot.tile.lod_scale);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 45.0, snapshot.tile.lod_pitch_threshold);
    TEST_ASSERT_EQUAL_DOUBLE(tile.lod_zoom_shift, snapshot.tile.lod_zoom_shift);
    TEST_ASSERT_EQUAL_UINT32(tile.lod_mode, snapshot.tile.lod_mode);
    tile.fields = MLN_MAP_TILE_OPTION_LOD_MODE;
    tile.lod_mode = MLN_TILE_LOD_MODE_DEFAULT;
  }

  mln_projection_mode_options mode = {
    .size = sizeof(mln_projection_mode_options)
  };
  mode.fields = MLN_PROJECTION_MODE_OPTION_AXONOMETRIC |
                MLN_PROJECTION_MODE_OPTION_X_SKEW |
                MLN_PROJECTION_MODE_OPTION_Y_SKEW;
  mode.axonometric = true;
  mode.x_skew = 0.25;
  mode.y_skew = -0.125;
  for (size_t pass = 0; pass < 2; pass += 1) {
    COMMIT(
      "projection mode",
      mln_map_set_projection_mode(map, &mode, &completion.descriptor, NULL)
    );
    TEST_ASSERT_EQUAL(
      mode.axonometric, snapshot.view.projection_mode.axonometric
    );
    if (pass == 0) {
      TEST_ASSERT_EQUAL_DOUBLE(0.25, snapshot.view.projection_mode.x_skew);
      TEST_ASSERT_EQUAL_DOUBLE(-0.125, snapshot.view.projection_mode.y_skew);
    }
    mode.fields = MLN_PROJECTION_MODE_OPTION_AXONOMETRIC;
    mode.axonometric = false;
  }

  // MapLibre renormalizes the altitude of a free-camera position, so only the
  // ground position round-trips. The orientation is a quarter turn about the
  // vertical axis.
  mln_free_camera_options free_camera = {
    .size = sizeof(mln_free_camera_options)
  };
  free_camera.fields = MLN_FREE_CAMERA_OPTION_POSITION;
  free_camera.position = (mln_vec3){.x = 0.25, .y = 0.25, .z = 0.5};
  COMMIT(
    "free camera position", mln_map_set_free_camera_options(
                              map, &free_camera, &completion.descriptor, NULL
                            )
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.25, snapshot.free_camera.position.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.25, snapshot.free_camera.position.y);
  TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, snapshot.free_camera.position.z);
  free_camera.fields = MLN_FREE_CAMERA_OPTION_ORIENTATION;
  free_camera.orientation =
    (mln_quaternion){0.0, 0.0, 0.7071067811865476, 0.7071067811865476};
  COMMIT(
    "free camera orientation", mln_map_set_free_camera_options(
                                 map, &free_camera, &completion.descriptor, NULL
                               )
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, snapshot.free_camera.orientation.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, snapshot.free_camera.orientation.y);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, free_camera.orientation.z, snapshot.free_camera.orientation.z
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, free_camera.orientation.w, snapshot.free_camera.orientation.w
  );

  mln_bound_options bounds = {.size = sizeof(mln_bound_options)};
  bounds.fields = MLN_BOUND_OPTION_MIN_ZOOM | MLN_BOUND_OPTION_MAX_ZOOM |
                  MLN_BOUND_OPTION_MIN_PITCH | MLN_BOUND_OPTION_MAX_PITCH;
  bounds.min_zoom = 2.0;
  bounds.max_zoom = 15.0;
  bounds.min_pitch = 10.0;
  bounds.max_pitch = 45.0;
  COMMIT(
    "zoom and pitch limits",
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_DOUBLE(2.0, snapshot.bounds.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(15.0, snapshot.bounds.max_zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 10.0, snapshot.bounds.min_pitch);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 45.0, snapshot.bounds.max_pitch);
  bounds.fields = MLN_BOUND_OPTION_BOUNDS;
  bounds.bounds = (mln_lat_lng_bounds){{-45.0, -120.0}, {45.0, 120.0}};
  COMMIT(
    "geographic bounds",
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(snapshot.bounds.unbounded);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, -45.0, snapshot.bounds.bounds.southwest.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, -120.0, snapshot.bounds.bounds.southwest.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, 45.0, snapshot.bounds.bounds.northeast.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, 120.0, snapshot.bounds.bounds.northeast.longitude
  );
  bounds.fields = MLN_BOUND_OPTION_UNBOUNDED;
  COMMIT(
    "unbounded", mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  TEST_ASSERT_TRUE(snapshot.bounds.unbounded);

  const uint64_t camera_events = MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE |
                                 MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED;
  COMMIT(
    "event mask",
    mln_map_set_event_mask(map, camera_events, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(camera_events, snapshot.event_mask);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status submit_viewport_row(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_map_set_viewport_options(
    *(const mln_map*)context, descriptor, &discard, diagnostic
  );
}

static void viewport_undersized(void* descriptor) {
  ((mln_map_viewport_options*)descriptor)->size -= 1;
}
static void viewport_unknown_field(void* descriptor) {
  ((mln_map_viewport_options*)descriptor)->fields = UINT32_C(1) << 31;
}
static void viewport_bad_north(void* descriptor) {
  mln_map_viewport_options* options = descriptor;
  options->fields = MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
  options->north_orientation = MLN_NORTH_ORIENTATION_LEFT + 1;
}
static void viewport_infinite_offset(void* descriptor) {
  mln_map_viewport_options* options = descriptor;
  options->fields = MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  options->frustum_offset.bottom = INFINITY;
}
static void viewport_negative_offset(void* descriptor) {
  mln_map_viewport_options* options = descriptor;
  options->fields = MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  options->frustum_offset.left = -1.0;
}

static const mln_test_validation_case viewport_cases[] = {
  {"undersized", viewport_undersized, MLN_STATUS_INVALID_ARGUMENT,
   "size is too small"},
  {"unknown field", viewport_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"north orientation out of range", viewport_bad_north,
   MLN_STATUS_INVALID_ARGUMENT, "north_orientation is invalid"},
  {"infinite frustum offset", viewport_infinite_offset,
   MLN_STATUS_INVALID_ARGUMENT, "must be finite"},
  {"negative frustum offset", viewport_negative_offset,
   MLN_STATUS_INVALID_ARGUMENT, "greater than or equal to 0"},
};

static mln_status submit_tile_row(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_map_set_tile_options(
    *(const mln_map*)context, descriptor, &discard, diagnostic
  );
}

static void tile_undersized(void* descriptor) {
  ((mln_map_tile_options*)descriptor)->size -= 1;
}
static void tile_unknown_field(void* descriptor) {
  ((mln_map_tile_options*)descriptor)->fields = UINT32_C(1) << 31;
}
static void tile_prefetch_past_a_byte(void* descriptor) {
  mln_map_tile_options* options = descriptor;
  options->fields = MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
  options->prefetch_zoom_delta = 256;
}
static void tile_nan_lod_radius(void* descriptor) {
  mln_map_tile_options* options = descriptor;
  options->fields = MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
  options->lod_min_radius = NAN;
}
static void tile_bad_lod_mode(void* descriptor) {
  mln_map_tile_options* options = descriptor;
  options->fields = MLN_MAP_TILE_OPTION_LOD_MODE;
  options->lod_mode = MLN_TILE_LOD_MODE_DISTANCE + 1;
}

static const mln_test_validation_case tile_cases[] = {
  {"undersized", tile_undersized, MLN_STATUS_INVALID_ARGUMENT,
   "size is too small"},
  {"unknown field", tile_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"prefetch delta past a byte", tile_prefetch_past_a_byte,
   MLN_STATUS_INVALID_ARGUMENT, "at most 255"},
  {"NaN lod min radius", tile_nan_lod_radius, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"lod mode out of range", tile_bad_lod_mode, MLN_STATUS_INVALID_ARGUMENT,
   "lod_mode is invalid"},
};

static mln_status submit_projection_mode_row(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_map_set_projection_mode(
    *(const mln_map*)context, descriptor, &discard, diagnostic
  );
}

static void mode_undersized(void* descriptor) {
  ((mln_projection_mode_options*)descriptor)->size -= 1;
}
static void mode_unknown_field(void* descriptor) {
  ((mln_projection_mode_options*)descriptor)->fields = UINT32_C(1) << 31;
}
static void mode_nan_x_skew(void* descriptor) {
  mln_projection_mode_options* mode = descriptor;
  mode->fields = MLN_PROJECTION_MODE_OPTION_X_SKEW;
  mode->x_skew = NAN;
}

static const mln_test_validation_case projection_mode_cases[] = {
  {"undersized", mode_undersized, MLN_STATUS_INVALID_ARGUMENT, "valid size"},
  {"unknown field", mode_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"NaN x skew", mode_nan_x_skew, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
};

static void option_descriptors_reject_what_they_cannot_express(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_map_viewport_options viewport = {
    .size = sizeof(mln_map_viewport_options)
  };
  mln_test_run_validation_table(
    viewport_cases, sizeof(viewport_cases) / sizeof(*viewport_cases), &viewport,
    sizeof(viewport), submit_viewport_row, &map
  );
  const mln_map_tile_options tile = {.size = sizeof(mln_map_tile_options)};
  mln_test_run_validation_table(
    tile_cases, sizeof(tile_cases) / sizeof(*tile_cases), &tile, sizeof(tile),
    submit_tile_row, &map
  );
  const mln_projection_mode_options mode = {
    .size = sizeof(mln_projection_mode_options)
  };
  mln_test_run_validation_table(
    projection_mode_cases,
    sizeof(projection_mode_cases) / sizeof(*projection_mode_cases), &mode,
    sizeof(mode), submit_projection_mode_row, &map
  );

  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_set_viewport_options(map, NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_tile_options(map, NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_projection_mode(map, NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_debug_flags(MLN_HANDLE_NULL, 0, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_rendering_stats_view_enabled(
    MLN_HANDLE_NULL, true, &discard, NULL
  ));
  MLN_TEST_INVALID(mln_map_set_globe_free_bearing_enabled(
    MLN_HANDLE_NULL, true, &discard, NULL
  ));
  MLN_TEST_INVALID(mln_map_dump_debug_logs(MLN_HANDLE_NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_debug_flags(
    map, UINT32_C(1) << 31, &discard, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_STRING(
    "debug flags contain unknown bits", mln_test_last_error()
  );

  // A rejected command publishes nothing, so the snapshot keeps the defaults.
  MLN_TEST_OK(mln_test_map_fence(map));
  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(0, snapshot.debug_flags);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NORTH_ORIENTATION_UP, snapshot.view.viewport.north_orientation
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_TILE_LOD_MODE_DEFAULT, snapshot.tile.lod_mode);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void map_extent_snapshot_tracks_resize_and_keeps_scale_factor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.initial_extent.width = 512;
  options.initial_extent.scale_factor = 1.1;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(512, snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(256, snapshot.view.extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.view.extent.scale_factor);
  const uint64_t initial_generation = snapshot.generation;

  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_size){.width = 96, .height = 48}, &completion.descriptor,
    NULL
  ));
  MLN_TEST_OK(mln_test_map_fence(map));
  snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_THAN_UINT64(initial_generation, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.view.extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.view.extent.scale_factor);

  const uint64_t render_update = snapshot.latest_render_update_generation;
  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_size){.width = 96, .height = 48}, &completion.descriptor,
    NULL
  ));
  MLN_TEST_OK(mln_test_map_fence(map));
  snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT64(
    render_update, snapshot.latest_render_update_generation
  );

  // A resize changes only the width and height. The scale factor stays.
  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_resize(
    map, (mln_logical_size){.width = 0, .height = 48}, &rejected, NULL
  ));
  MLN_TEST_OK(mln_test_map_fence(map));
  snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.view.extent.scale_factor);

  MLN_TEST_INVALID(mln_map_get_snapshot(MLN_HANDLE_NULL, &snapshot, NULL));
  mln_map_snapshot undersized = {.size = sizeof(mln_map_snapshot) - 1};
  MLN_TEST_INVALID(mln_map_get_snapshot(map, &undersized, NULL));
  MLN_TEST_INVALID(mln_map_get_snapshot(map, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The snapshot reports the LOD pitch threshold in degrees, converted from the
// engine's radians. Sending that value back must not count as a change.
static void reapplying_the_reported_lod_pitch_threshold_changes_nothing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_tile_options tile = {
    .size = sizeof(mln_map_tile_options),
    .fields = MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD,
    .lod_pitch_threshold = 75.0,
  };

  mln_map_snapshot snapshot = read_snapshot(map);
  uint64_t render_update = snapshot.latest_render_update_generation;
  MLN_TEST_AWAIT_OK(
    mln_map_set_tile_options(map, &tile, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_THAN_UINT64(
    render_update, snapshot.latest_render_update_generation
  );

  render_update = snapshot.latest_render_update_generation;
  tile.lod_pitch_threshold = snapshot.tile.lod_pitch_threshold;
  MLN_TEST_AWAIT_OK(
    mln_map_set_tile_options(map, &tile, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT64(
    render_update, snapshot.latest_render_update_generation
  );
  TEST_ASSERT_EQUAL_DOUBLE(
    tile.lod_pitch_threshold, snapshot.tile.lod_pitch_threshold
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void resizes_apply_in_order_and_the_last_sets_the_extent(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_test_completion first = mln_test_completion_default(0);
  mln_test_completion last = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_resize(
    map, (mln_logical_size){.width = 96, .height = 48}, &first.descriptor, NULL
  ));
  MLN_TEST_OK(mln_map_resize(
    map, (mln_logical_size){.width = 64, .height = 32}, &last.descriptor, NULL
  ));

  MLN_TEST_OK(mln_test_completion_finish(&first));
  const uint64_t earlier = mln_test_completion_generation(&first);
  mln_test_completion_destroy(&first);

  MLN_TEST_OK(mln_test_completion_finish(&last));
  const uint64_t committed = mln_test_completion_generation(&last);
  mln_test_completion_destroy(&last);
  TEST_ASSERT_GREATER_THAN_UINT64(earlier, committed);

  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(committed, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(64, snapshot.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(32, snapshot.view.extent.height);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum { submitting_threads = 4, commands_per_thread = 16 };

// What one submitting thread observed. The thread records rather than asserts,
// because an assertion must not unwind a thread the test did not start it on.
typedef struct submitter {
  mln_map map;
  mln_status statuses[commands_per_thread];
  uint64_t generations[commands_per_thread];
  uint64_t snapshot_generations[commands_per_thread];
} submitter;

static void submit_commands(void* argument) {
  submitter* state = argument;
  for (size_t index = 0; index < commands_per_thread; index += 1) {
    mln_test_completion completion = mln_test_completion_default(0);
    mln_status status =
      mln_map_set_debug_flags(state->map, 0, &completion.descriptor, NULL);
    if (status == MLN_STATUS_OK) {
      status = mln_test_completion_finish(&completion);
      state->generations[index] = mln_test_completion_generation(&completion);
    } else {
      completion.descriptor.release_user_data(completion.descriptor.user_data);
    }
    mln_test_completion_destroy(&completion);
    state->statuses[index] = status;
    mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
    if (mln_map_get_snapshot(state->map, &snapshot, NULL) == MLN_STATUS_OK) {
      state->snapshot_generations[index] = snapshot.generation;
    }
  }
}

static int compare_generations(const void* left, const void* right) {
  const uint64_t a = *(const uint64_t*)left;
  const uint64_t b = *(const uint64_t*)right;
  return (a > b) - (a < b);
}

static void generations_increase_across_submitting_threads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  static submitter submitters[submitting_threads];
  mln_test_thread* threads[submitting_threads] = {0};
  for (size_t thread = 0; thread < submitting_threads; thread += 1) {
    submitters[thread] = (submitter){.map = map};
    threads[thread] =
      mln_test_thread_start(submit_commands, &submitters[thread]);
  }
  for (size_t thread = 0; thread < submitting_threads; thread += 1) {
    mln_test_thread_join(threads[thread]);
  }

  uint64_t all[submitting_threads * commands_per_thread];
  size_t count = 0;
  for (size_t thread = 0; thread < submitting_threads; thread += 1) {
    const submitter* state = &submitters[thread];
    for (size_t index = 0; index < commands_per_thread; index += 1) {
      MLN_TEST_OK(state->statuses[index]);
      TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
        state->generations[index], state->snapshot_generations[index]
      );
      if (index > 0) {
        TEST_ASSERT_GREATER_THAN_UINT64(
          state->generations[index - 1], state->generations[index]
        );
      }
      all[count] = state->generations[index];
      count += 1;
    }
  }
  qsort(all, count, sizeof(*all), compare_generations);
  for (size_t index = 1; index < count; index += 1) {
    TEST_ASSERT_GREATER_THAN_UINT64(all[index - 1], all[index]);
  }
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    all[count - 1], read_snapshot(map).generation
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(every_option_command_round_trips_through_the_snapshot);
  RUN_TEST(option_descriptors_reject_what_they_cannot_express);
  RUN_TEST(map_extent_snapshot_tracks_resize_and_keeps_scale_factor);
  RUN_TEST(reapplying_the_reported_lod_pitch_threshold_changes_nothing);
  RUN_TEST(resizes_apply_in_order_and_the_last_sets_the_extent);
  RUN_TEST(generations_increase_across_submitting_threads);
}
