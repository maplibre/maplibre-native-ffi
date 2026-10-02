// Map option commands: each one's committed value round-trips through the map
// snapshot, each descriptor rejects what it cannot express, the extent changes
// only through resize, and every commit publishes a generation of its own.

#include <math.h>

#include "support/test_support.h"

static mln_map_snapshot read_snapshot(mln_map map) {
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &snapshot, NULL));
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
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    MLN_COMMAND_DISPOSITION_COMMITTED,
    mln_test_completion_disposition(completion), label
  );
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

// Every command commits with a generation of its own, and the snapshot at or
// past that generation carries the committed value. Every enum value is
// committed once, so each mapping to MapLibre and back is exercised. The free
// camera precedes the constraints, which would move its position.
static void every_option_command_round_trips_through_the_snapshot(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_snapshot snapshot = read_snapshot(map);
  uint64_t previous = snapshot.generation;

  const uint32_t every_debug_option =
    MLN_MAP_DEBUG_TILE_BORDERS | MLN_MAP_DEBUG_PARSE_STATUS |
    MLN_MAP_DEBUG_TIMESTAMPS | MLN_MAP_DEBUG_COLLISION |
    MLN_MAP_DEBUG_OVERDRAW | MLN_MAP_DEBUG_STENCIL_CLIP |
    MLN_MAP_DEBUG_DEPTH_BUFFER;
  for (size_t pass = 0; pass < 2; pass += 1) {
    const uint32_t debug = pass == 0 ? every_debug_option : 0;
    COMMIT(
      "debug options",
      mln_map_set_debug_options(map, debug, &completion.descriptor, NULL)
    );
    TEST_ASSERT_EQUAL_UINT32(debug, snapshot.debug_options);
    const bool stats = pass == 0;
    COMMIT(
      "rendering stats view", mln_map_set_rendering_stats_view_enabled(
                                map, stats, &completion.descriptor, NULL
                              )
    );
    TEST_ASSERT_EQUAL(stats, snapshot.rendering_stats_view_enabled);
  }

  // Each viewport enum field, then the frustum offset.
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
    mln_map_viewport_options viewport = mln_map_viewport_options_default();
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
        ? snapshot.viewport.north_orientation
      : field == MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
        ? snapshot.viewport.constrain_mode
        : snapshot.viewport.viewport_mode;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      viewport_rows[index].value, committed, label
    );
  }
  mln_map_viewport_options viewport = mln_map_viewport_options_default();
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
  viewport.frustum_offset = (mln_edge_insets){1.0, 2.0, 3.0, 4.0};
  COMMIT(
    "frustum offset",
    mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_MEMORY(
    &viewport.frustum_offset, &snapshot.viewport.frustum_offset,
    sizeof(viewport.frustum_offset)
  );

  mln_map_tile_options tile = mln_map_tile_options_default();
  tile.fields =
    MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA |
    MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS | MLN_MAP_TILE_OPTION_LOD_SCALE |
    MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD |
    MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT | MLN_MAP_TILE_OPTION_LOD_MODE;
  tile.prefetch_zoom_delta = 3;
  tile.lod_min_radius = 2.5;
  tile.lod_scale = 1.5;
  tile.lod_pitch_threshold = 0.75;
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
    TEST_ASSERT_EQUAL_DOUBLE(
      tile.lod_pitch_threshold, snapshot.tile.lod_pitch_threshold
    );
    TEST_ASSERT_EQUAL_DOUBLE(tile.lod_zoom_shift, snapshot.tile.lod_zoom_shift);
    TEST_ASSERT_EQUAL_UINT32(tile.lod_mode, snapshot.tile.lod_mode);
    // Only the LOD mode changes in the second pass.
    tile.fields = MLN_MAP_TILE_OPTION_LOD_MODE;
    tile.lod_mode = MLN_TILE_LOD_MODE_DEFAULT;
  }

  mln_projection_mode mode = mln_projection_mode_default();
  mode.fields = MLN_PROJECTION_MODE_AXONOMETRIC | MLN_PROJECTION_MODE_X_SKEW |
                MLN_PROJECTION_MODE_Y_SKEW;
  mode.axonometric = true;
  mode.x_skew = 0.25;
  mode.y_skew = -0.125;
  for (size_t pass = 0; pass < 2; pass += 1) {
    COMMIT(
      "projection mode",
      mln_map_set_projection_mode(map, &mode, &completion.descriptor, NULL)
    );
    TEST_ASSERT_BITS_HIGH(mode.fields, snapshot.projection_mode.fields);
    TEST_ASSERT_EQUAL(mode.axonometric, snapshot.projection_mode.axonometric);
    if (pass == 0) {
      TEST_ASSERT_EQUAL_DOUBLE(0.25, snapshot.projection_mode.x_skew);
      TEST_ASSERT_EQUAL_DOUBLE(-0.125, snapshot.projection_mode.y_skew);
    }
    // Then perspective.
    mode.fields = MLN_PROJECTION_MODE_AXONOMETRIC;
    mode.axonometric = false;
  }

  // MapLibre renormalizes the altitude of a free-camera position, so only the
  // ground position round-trips. The orientation is a quarter turn about the
  // vertical axis.
  mln_free_camera_options free_camera = mln_free_camera_options_default();
  free_camera.fields = MLN_FREE_CAMERA_OPTION_POSITION;
  free_camera.position = (mln_vec3){.x = 0.25, .y = 0.25, .z = 0.5};
  COMMIT(
    "free camera position", mln_map_set_free_camera_options(
                              map, &free_camera, &completion.descriptor, NULL
                            )
  );
  TEST_ASSERT_BITS_HIGH(
    MLN_FREE_CAMERA_OPTION_POSITION, snapshot.free_camera.fields
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
  TEST_ASSERT_BITS_HIGH(
    MLN_FREE_CAMERA_OPTION_ORIENTATION, snapshot.free_camera.fields
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, snapshot.free_camera.orientation.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, snapshot.free_camera.orientation.y);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, free_camera.orientation.z, snapshot.free_camera.orientation.z
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, free_camera.orientation.w, snapshot.free_camera.orientation.w
  );

  mln_bound_options bounds = mln_bound_options_default();
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
  TEST_ASSERT_BITS_HIGH(bounds.fields, snapshot.bounds.fields);
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
  TEST_ASSERT_BITS_HIGH(MLN_BOUND_OPTION_BOUNDS, snapshot.bounds.fields);
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
  TEST_ASSERT_BITS_HIGH(MLN_BOUND_OPTION_UNBOUNDED, snapshot.bounds.fields);

  const uint64_t camera_events =
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE;
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
static void viewport_bad_constrain(void* descriptor) {
  mln_map_viewport_options* options = descriptor;
  options->fields = MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
  options->constrain_mode = MLN_CONSTRAIN_MODE_SCREEN + 1;
}
static void viewport_bad_mode(void* descriptor) {
  mln_map_viewport_options* options = descriptor;
  options->fields = MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
  options->viewport_mode = MLN_VIEWPORT_MODE_FLIPPED_Y + 1;
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
  {"constrain mode out of range", viewport_bad_constrain,
   MLN_STATUS_INVALID_ARGUMENT, "constrain_mode is invalid"},
  {"viewport mode out of range", viewport_bad_mode, MLN_STATUS_INVALID_ARGUMENT,
   "viewport_mode is invalid"},
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
static void tile_infinite_lod_scale(void* descriptor) {
  mln_map_tile_options* options = descriptor;
  options->fields = MLN_MAP_TILE_OPTION_LOD_SCALE;
  options->lod_scale = INFINITY;
}
static void tile_nan_lod_pitch(void* descriptor) {
  mln_map_tile_options* options = descriptor;
  options->fields = MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
  options->lod_pitch_threshold = NAN;
}
static void tile_nan_lod_shift(void* descriptor) {
  mln_map_tile_options* options = descriptor;
  options->fields = MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
  options->lod_zoom_shift = NAN;
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
  {"infinite lod scale", tile_infinite_lod_scale, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"NaN lod pitch threshold", tile_nan_lod_pitch, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"NaN lod zoom shift", tile_nan_lod_shift, MLN_STATUS_INVALID_ARGUMENT,
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
  ((mln_projection_mode*)descriptor)->size -= 1;
}
static void mode_unknown_field(void* descriptor) {
  ((mln_projection_mode*)descriptor)->fields = UINT32_C(1) << 31;
}
static void mode_nan_x_skew(void* descriptor) {
  mln_projection_mode* mode = descriptor;
  mode->fields = MLN_PROJECTION_MODE_X_SKEW;
  mode->x_skew = NAN;
}
static void mode_infinite_y_skew(void* descriptor) {
  mln_projection_mode* mode = descriptor;
  mode->fields = MLN_PROJECTION_MODE_Y_SKEW;
  mode->y_skew = -INFINITY;
}

static const mln_test_validation_case projection_mode_cases[] = {
  {"undersized", mode_undersized, MLN_STATUS_INVALID_ARGUMENT, "valid size"},
  {"unknown field", mode_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"NaN x skew", mode_nan_x_skew, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"infinite y skew", mode_infinite_y_skew, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
};

static void option_descriptors_reject_what_they_cannot_express(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_map_viewport_options viewport = mln_map_viewport_options_default();
  mln_test_run_validation_table(
    viewport_cases, sizeof(viewport_cases) / sizeof(*viewport_cases), &viewport,
    sizeof(viewport), submit_viewport_row, &map
  );
  const mln_map_tile_options tile = mln_map_tile_options_default();
  mln_test_run_validation_table(
    tile_cases, sizeof(tile_cases) / sizeof(*tile_cases), &tile, sizeof(tile),
    submit_tile_row, &map
  );
  const mln_projection_mode mode = mln_projection_mode_default();
  mln_test_run_validation_table(
    projection_mode_cases,
    sizeof(projection_mode_cases) / sizeof(*projection_mode_cases), &mode,
    sizeof(mode), submit_projection_mode_row, &map
  );

  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_set_viewport_options(map, NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_tile_options(map, NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_projection_mode(map, NULL, &discard, NULL));
  MLN_TEST_INVALID(
    mln_map_set_debug_options(MLN_HANDLE_NULL, 0, &discard, NULL)
  );
  MLN_TEST_INVALID(mln_map_set_rendering_stats_view_enabled(
    MLN_HANDLE_NULL, true, &discard, NULL
  ));
  MLN_TEST_INVALID(mln_map_dump_debug_logs(MLN_HANDLE_NULL, &discard, NULL));
  MLN_TEST_INVALID(mln_map_set_debug_options(
    map, UINT32_C(1) << 31, &discard, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_STRING(
    "debug options contain unknown bits", mln_test_last_error()
  );

  // A rejected command publishes nothing, so the snapshot keeps the defaults.
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(0, snapshot.debug_options);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NORTH_ORIENTATION_UP, snapshot.viewport.north_orientation
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_TILE_LOD_MODE_DEFAULT, snapshot.tile.lod_mode);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void map_extent_snapshot_tracks_resize_and_fixes_scale_factor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 512, .height = 256, .scale_factor = 1.1};
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(512, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(256, snapshot.logical_extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.logical_extent.scale_factor);
  const uint64_t initial_generation = snapshot.generation;

  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.1},
    &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_THAN_UINT64(initial_generation, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.logical_extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.logical_extent.scale_factor);

  // The renderer fixes its pixel ratio at creation, so a resize may change
  // only the width and height.
  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_resize(
    map, (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 2.25},
    &rejected, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_STRING(
    "scale factor is fixed at map creation", mln_test_last_error()
  );
  MLN_TEST_INVALID(mln_map_resize(
    map, (mln_logical_extent){.width = 0, .height = 48, .scale_factor = 1.1},
    &rejected, NULL
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.logical_extent.scale_factor);

  MLN_TEST_INVALID(mln_map_snapshot_get(MLN_HANDLE_NULL, &snapshot, NULL));
  mln_map_snapshot undersized = {.size = sizeof(mln_map_snapshot) - 1};
  MLN_TEST_INVALID(mln_map_snapshot_get(map, &undersized, NULL));
  MLN_TEST_INVALID(mln_map_snapshot_get(map, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A resize a later resize replaces never reaches the map: it completes
// MLN_STATUS_OK with MLN_COMMAND_DISPOSITION_SUPERSEDED and no generation,
// while the newest one commits and publishes the extent.
static void a_replaced_resize_completes_superseded(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  // Holds the runtime worker inside a completion callback, so the two resizes
  // queue behind it and the worker reaches them together.
  mln_test_gate gate;
  mln_test_gate_init(&gate);
  const mln_completion hold = mln_test_gate_completion(&gate);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &hold, NULL));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(&gate), "the runtime worker never parked"
  );

  mln_test_completion replaced = mln_test_completion_default(0);
  mln_test_completion newest = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_resize(
    map, (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.0},
    &replaced.descriptor, NULL
  ));
  MLN_TEST_OK(mln_map_resize(
    map, (mln_logical_extent){.width = 64, .height = 32, .scale_factor = 1.0},
    &newest.descriptor, NULL
  ));
  mln_test_gate_release(&gate);

  MLN_TEST_OK(mln_test_completion_finish(&replaced));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_SUPERSEDED,
    mln_test_completion_disposition(&replaced)
  );
  TEST_ASSERT_EQUAL_UINT64(0, mln_test_completion_generation(&replaced));
  mln_test_completion_destroy(&replaced);

  MLN_TEST_OK(mln_test_completion_finish(&newest));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, mln_test_completion_disposition(&newest)
  );
  const uint64_t committed = mln_test_completion_generation(&newest);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, committed);
  mln_test_completion_destroy(&newest);

  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(committed, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(64, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(32, snapshot.logical_extent.height);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum { submitting_threads = 4, commands_per_thread = 16 };

// What one submitting thread observed. The thread records rather than asserts,
// because an assertion must not unwind a thread the test did not start it on.
typedef struct submitter {
  mln_map map;
  mln_status statuses[commands_per_thread];
  uint32_t dispositions[commands_per_thread];
  uint64_t generations[commands_per_thread];
  uint64_t snapshot_generations[commands_per_thread];
} submitter;

static void submit_commands(void* argument) {
  submitter* state = argument;
  for (size_t index = 0; index < commands_per_thread; index += 1) {
    mln_test_completion completion = mln_test_completion_default(0);
    mln_status status =
      mln_map_set_debug_options(state->map, 0, &completion.descriptor, NULL);
    if (status == MLN_STATUS_OK) {
      status = mln_test_completion_finish(&completion);
      state->dispositions[index] = mln_test_completion_disposition(&completion);
      state->generations[index] = mln_test_completion_generation(&completion);
    } else {
      completion.descriptor.release_user_data(completion.descriptor.user_data);
    }
    mln_test_completion_destroy(&completion);
    state->statuses[index] = status;
    mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
    if (mln_map_snapshot_get(state->map, &snapshot, NULL) == MLN_STATUS_OK) {
      state->snapshot_generations[index] = snapshot.generation;
    }
  }
}

static int compare_generations(const void* left, const void* right) {
  const uint64_t a = *(const uint64_t*)left;
  const uint64_t b = *(const uint64_t*)right;
  return (a > b) - (a < b);
}

// Commands from several threads commit in one order: each commit carries a
// generation no other commit shares, a thread's own commits carry increasing
// generations, and a snapshot any of those threads reads after its commit
// observes it.
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
      TEST_ASSERT_EQUAL_UINT32(
        MLN_COMMAND_DISPOSITION_COMMITTED, state->dispositions[index]
      );
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

// A creation descriptor whose extent has no area or no finite scale factor is
// rejected before any map exists.
static void map_creation_rejects_a_degenerate_extent(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 0;
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_test_map_create_status(runtime, &options, &map));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "initial extent"));
  options = mln_map_options_default();
  options.initial_extent.scale_factor = INFINITY;
  MLN_TEST_INVALID(mln_test_map_create_status(runtime, &options, &map));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(every_option_command_round_trips_through_the_snapshot);
  RUN_TEST(option_descriptors_reject_what_they_cannot_express);
  RUN_TEST(map_creation_rejects_a_degenerate_extent);
  RUN_TEST(map_extent_snapshot_tracks_resize_and_fixes_scale_factor);
  RUN_TEST(a_replaced_resize_completes_superseded);
  RUN_TEST(generations_increase_across_submitting_threads);
}
