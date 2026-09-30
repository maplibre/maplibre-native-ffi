// Map option commands: each one's committed value round-trips through the map
// snapshot, each descriptor rejects what it cannot express, the extent changes
// only through resize, and every commit publishes a generation of its own.

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static mln_map_snapshot read_snapshot(mln_map map) {
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(map, &snapshot, NULL)
  );
  return snapshot;
}

// One option command and the snapshot fields it must publish. `input` is the
// value the command submits, and `verify` compares it with the snapshot.
typedef struct snapshot_row {
  const char* label;
  mln_status (*submit)(
    mln_map map, const void* input, const mln_completion* completion
  );
  const void* input;
  void (*verify)(
    const char* label, const mln_map_snapshot* snapshot, const void* input
  );
} snapshot_row;

static mln_status submit_debug_options(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_debug_options(
    map, *(const uint32_t*)input, completion, NULL
  );
}

static void verify_debug_options(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    *(const uint32_t*)input, snapshot->debug_options, label
  );
}

static mln_status submit_rendering_stats(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_rendering_stats_view_enabled(
    map, *(const bool*)input, completion, NULL
  );
}

static void verify_rendering_stats(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  TEST_ASSERT_EQUAL_MESSAGE(
    *(const bool*)input, snapshot->rendering_stats_view_enabled, label
  );
}

static mln_status submit_viewport(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_viewport_options(map, input, completion, NULL);
}

// Compares the fields the command selected. The snapshot reports every field.
static void verify_viewport(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  const mln_map_viewport_options* sent = input;
  const mln_map_viewport_options* got = &snapshot->viewport;
  if ((sent->fields & MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION) != 0U) {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      sent->north_orientation, got->north_orientation, label
    );
  }
  if ((sent->fields & MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE) != 0U) {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      sent->constrain_mode, got->constrain_mode, label
    );
  }
  if ((sent->fields & MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE) != 0U) {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      sent->viewport_mode, got->viewport_mode, label
    );
  }
  if ((sent->fields & MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->frustum_offset.top, got->frustum_offset.top, label
    );
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->frustum_offset.left, got->frustum_offset.left, label
    );
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->frustum_offset.bottom, got->frustum_offset.bottom, label
    );
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->frustum_offset.right, got->frustum_offset.right, label
    );
  }
}

static mln_status submit_tile(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_tile_options(map, input, completion, NULL);
}

static void verify_tile(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  const mln_map_tile_options* sent = input;
  const mln_map_tile_options* got = &snapshot->tile;
  if ((sent->fields & MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA) != 0U) {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      sent->prefetch_zoom_delta, got->prefetch_zoom_delta, label
    );
  }
  if ((sent->fields & MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->lod_min_radius, got->lod_min_radius, label
    );
  }
  if ((sent->fields & MLN_MAP_TILE_OPTION_LOD_SCALE) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(sent->lod_scale, got->lod_scale, label);
  }
  if ((sent->fields & MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->lod_pitch_threshold, got->lod_pitch_threshold, label
    );
  }
  if ((sent->fields & MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->lod_zoom_shift, got->lod_zoom_shift, label
    );
  }
  if ((sent->fields & MLN_MAP_TILE_OPTION_LOD_MODE) != 0U) {
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(sent->lod_mode, got->lod_mode, label);
  }
}

static mln_status submit_projection_mode(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_projection_mode(map, input, completion, NULL);
}

static void verify_projection_mode(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  const mln_projection_mode* sent = input;
  const mln_projection_mode* got = &snapshot->projection_mode;
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    sent->fields, got->fields & sent->fields, label
  );
  if ((sent->fields & MLN_PROJECTION_MODE_AXONOMETRIC) != 0U) {
    TEST_ASSERT_EQUAL_MESSAGE(sent->axonometric, got->axonometric, label);
  }
  if ((sent->fields & MLN_PROJECTION_MODE_X_SKEW) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(sent->x_skew, got->x_skew, label);
  }
  if ((sent->fields & MLN_PROJECTION_MODE_Y_SKEW) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(sent->y_skew, got->y_skew, label);
  }
}

static mln_status submit_bounds(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_bounds(map, input, completion, NULL);
}

static void verify_bounds(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  const mln_bound_options* sent = input;
  const mln_bound_options* got = &snapshot->bounds;
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    sent->fields, got->fields & sent->fields, label
  );
  if ((sent->fields & MLN_BOUND_OPTION_BOUNDS) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->bounds.southwest.latitude, got->bounds.southwest.latitude,
      label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->bounds.southwest.longitude, got->bounds.southwest.longitude,
      label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->bounds.northeast.latitude, got->bounds.northeast.latitude,
      label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->bounds.northeast.longitude, got->bounds.northeast.longitude,
      label
    );
  }
  if ((sent->fields & MLN_BOUND_OPTION_MIN_ZOOM) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(sent->min_zoom, got->min_zoom, label);
  }
  if ((sent->fields & MLN_BOUND_OPTION_MAX_ZOOM) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(sent->max_zoom, got->max_zoom, label);
  }
  if ((sent->fields & MLN_BOUND_OPTION_MIN_PITCH) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->min_pitch, got->min_pitch, label
    );
  }
  if ((sent->fields & MLN_BOUND_OPTION_MAX_PITCH) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->max_pitch, got->max_pitch, label
    );
  }
}

static mln_status submit_free_camera(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_free_camera_options(map, input, completion, NULL);
}

// MapLibre renormalizes the altitude of a free-camera position, so only the
// ground position round-trips.
static void verify_free_camera_position(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  const mln_free_camera_options* sent = input;
  const mln_free_camera_options* got = &snapshot->free_camera;
  TEST_ASSERT_TRUE_MESSAGE(
    (got->fields & MLN_FREE_CAMERA_OPTION_POSITION) != 0U, label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1e-6, sent->position.x, got->position.x, label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1e-6, sent->position.y, got->position.y, label
  );
  TEST_ASSERT_GREATER_THAN_DOUBLE(0.0, got->position.z);
}

static mln_status submit_event_mask(
  mln_map map, const void* input, const mln_completion* completion
) {
  return mln_map_set_event_mask(map, *(const uint64_t*)input, completion, NULL);
}

static void verify_event_mask(
  const char* label, const mln_map_snapshot* snapshot, const void* input
) {
  TEST_ASSERT_EQUAL_UINT64_MESSAGE(
    *(const uint64_t*)input, snapshot->event_mask, label
  );
}

#define VIEWPORT(field_bit, member, value)    \
  (&(const mln_map_viewport_options){         \
    .size = sizeof(mln_map_viewport_options), \
    .fields = (field_bit),                    \
    .member = (value),                        \
  })
#define TILE(field_bit, member, value)    \
  (&(const mln_map_tile_options){         \
    .size = sizeof(mln_map_tile_options), \
    .fields = (field_bit),                \
    .member = (value),                    \
  })

static const uint32_t every_debug_option =
  MLN_MAP_DEBUG_TILE_BORDERS | MLN_MAP_DEBUG_PARSE_STATUS |
  MLN_MAP_DEBUG_TIMESTAMPS | MLN_MAP_DEBUG_COLLISION | MLN_MAP_DEBUG_OVERDRAW |
  MLN_MAP_DEBUG_STENCIL_CLIP | MLN_MAP_DEBUG_DEPTH_BUFFER;
static const uint32_t no_debug_option = 0;
static const bool enabled = true;
static const bool disabled = false;
static const uint64_t camera_event_mask =
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE |
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE;

// Each row commits one command on the same map, and every enum value appears
// in some row, so each mapping to MapLibre and back is exercised. The free
// camera row precedes the constraints, which would move its position.
static const snapshot_row snapshot_rows[] = {
  {"every debug option", submit_debug_options, &every_debug_option,
   verify_debug_options},
  {"no debug option", submit_debug_options, &no_debug_option,
   verify_debug_options},
  {"rendering stats view on", submit_rendering_stats, &enabled,
   verify_rendering_stats},
  {"rendering stats view off", submit_rendering_stats, &disabled,
   verify_rendering_stats},
  {"north right", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION, north_orientation,
     MLN_NORTH_ORIENTATION_RIGHT
   ),
   verify_viewport},
  {"north down", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION, north_orientation,
     MLN_NORTH_ORIENTATION_DOWN
   ),
   verify_viewport},
  {"north left", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION, north_orientation,
     MLN_NORTH_ORIENTATION_LEFT
   ),
   verify_viewport},
  {"north up", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION, north_orientation,
     MLN_NORTH_ORIENTATION_UP
   ),
   verify_viewport},
  {"constrain none", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE, constrain_mode,
     MLN_CONSTRAIN_MODE_NONE
   ),
   verify_viewport},
  {"constrain width and height", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE, constrain_mode,
     MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT
   ),
   verify_viewport},
  {"constrain screen", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE, constrain_mode,
     MLN_CONSTRAIN_MODE_SCREEN
   ),
   verify_viewport},
  {"constrain height only", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE, constrain_mode,
     MLN_CONSTRAIN_MODE_HEIGHT_ONLY
   ),
   verify_viewport},
  {"viewport flipped y", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE, viewport_mode,
     MLN_VIEWPORT_MODE_FLIPPED_Y
   ),
   verify_viewport},
  {"viewport default", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE, viewport_mode,
     MLN_VIEWPORT_MODE_DEFAULT
   ),
   verify_viewport},
  {"frustum offset", submit_viewport,
   VIEWPORT(
     MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET, frustum_offset,
     ((mln_edge_insets){.top = 1.0, .left = 2.0, .bottom = 3.0, .right = 4.0})
   ),
   verify_viewport},
  {"prefetch zoom delta", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA, prefetch_zoom_delta, 3),
   verify_tile},
  {"lod min radius", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS, lod_min_radius, 2.5), verify_tile},
  {"lod scale", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_LOD_SCALE, lod_scale, 1.5), verify_tile},
  {"lod pitch threshold", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD, lod_pitch_threshold, 0.75),
   verify_tile},
  {"lod zoom shift", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT, lod_zoom_shift, -1.25),
   verify_tile},
  {"lod mode distance", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_LOD_MODE, lod_mode, MLN_TILE_LOD_MODE_DISTANCE),
   verify_tile},
  {"lod mode default", submit_tile,
   TILE(MLN_MAP_TILE_OPTION_LOD_MODE, lod_mode, MLN_TILE_LOD_MODE_DEFAULT),
   verify_tile},
  {"axonometric with skew", submit_projection_mode,
   &(const mln_projection_mode){
     .size = sizeof(mln_projection_mode),
     .fields = MLN_PROJECTION_MODE_AXONOMETRIC | MLN_PROJECTION_MODE_X_SKEW |
               MLN_PROJECTION_MODE_Y_SKEW,
     .axonometric = true,
     .x_skew = 0.25,
     .y_skew = -0.125,
   },
   verify_projection_mode},
  {"perspective", submit_projection_mode,
   &(const mln_projection_mode){
     .size = sizeof(mln_projection_mode),
     .fields = MLN_PROJECTION_MODE_AXONOMETRIC,
     .axonometric = false,
   },
   verify_projection_mode},
  {"free camera position", submit_free_camera,
   &(const mln_free_camera_options){
     .size = sizeof(mln_free_camera_options),
     .fields = MLN_FREE_CAMERA_OPTION_POSITION,
     .position = {.x = 0.25, .y = 0.25, .z = 0.5},
   },
   verify_free_camera_position},
  {"zoom and pitch limits", submit_bounds,
   &(const mln_bound_options){
     .size = sizeof(mln_bound_options),
     .fields = MLN_BOUND_OPTION_MIN_ZOOM | MLN_BOUND_OPTION_MAX_ZOOM |
               MLN_BOUND_OPTION_MIN_PITCH | MLN_BOUND_OPTION_MAX_PITCH,
     .min_zoom = 2.0,
     .max_zoom = 15.0,
     .min_pitch = 10.0,
     .max_pitch = 45.0,
   },
   verify_bounds},
  {"geographic bounds", submit_bounds,
   &(const mln_bound_options){
     .size = sizeof(mln_bound_options),
     .fields = MLN_BOUND_OPTION_BOUNDS,
     .bounds =
       {.southwest = {.latitude = -45.0, .longitude = -120.0},
        .northeast = {.latitude = 45.0, .longitude = 120.0}},
   },
   verify_bounds},
  {"unbounded", submit_bounds,
   &(const mln_bound_options){
     .size = sizeof(mln_bound_options),
     .fields = MLN_BOUND_OPTION_UNBOUNDED,
   },
   verify_bounds},
  {"event mask", submit_event_mask, &camera_event_mask, verify_event_mask},
};

// Every command commits with a generation of its own, and the snapshot at or
// past that generation carries the committed value.
static void every_option_command_round_trips_through_the_snapshot(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  uint64_t previous = read_snapshot(map).generation;

  for (size_t index = 0; index < sizeof(snapshot_rows) / sizeof(*snapshot_rows);
       index += 1) {
    const snapshot_row* row = &snapshot_rows[index];
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, row->submit(map, row->input, &completion.descriptor),
      row->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, mln_test_completion_finish(&completion), row->label
    );
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      MLN_COMMAND_DISPOSITION_COMMITTED,
      mln_test_completion_disposition(&completion), row->label
    );
    const uint64_t committed = mln_test_completion_generation(&completion);
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_GREATER_THAN_UINT64_MESSAGE(previous, committed, row->label);
    previous = committed;

    const mln_map_snapshot snapshot = read_snapshot(map);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64_MESSAGE(
      committed, snapshot.generation, row->label
    );
    row->verify(row->label, &snapshot, row->input);
  }

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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_viewport_options(map, NULL, &discard, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_tile_options(map, NULL, &discard, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_projection_mode(map, NULL, &discard, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_debug_options(
      map, UINT32_C(1) << 31, &discard, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_EQUAL_STRING(
    "debug options contain unknown bits", mln_test_last_error()
  );

  // A rejected command publishes nothing, so the snapshot keeps the defaults.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(0, snapshot.debug_options);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_NORTH_ORIENTATION_UP, snapshot.viewport.north_orientation
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_TILE_LOD_MODE_DEFAULT, snapshot.tile.lod_mode);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void map_debug_commands_reject_a_null_map(void) {
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_debug_options(MLN_HANDLE_NULL, 0, &completion, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_rendering_stats_view_enabled(
                                   MLN_HANDLE_NULL, true, &completion, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_dump_debug_logs(MLN_HANDLE_NULL, &completion, NULL)
  );
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

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_resize(
      map, (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.1},
      &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  snapshot = read_snapshot(map);
  TEST_ASSERT_GREATER_THAN_UINT64(initial_generation, snapshot.generation);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.logical_extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.logical_extent.scale_factor);

  // The renderer fixes its pixel ratio at creation, so a resize may change
  // only the width and height.
  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_resize(
      map,
      (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 2.25},
      &rejected, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_EQUAL_STRING(
    "scale factor is fixed at map creation", mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_resize(
      map, (mln_logical_extent){.width = 0, .height = 48, .scale_factor = 1.1},
      &rejected, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  snapshot = read_snapshot(map);
  TEST_ASSERT_EQUAL_UINT32(96, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, snapshot.logical_extent.scale_factor);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_snapshot_get(MLN_HANDLE_NULL, &snapshot, NULL)
  );
  mln_map_snapshot undersized = {.size = sizeof(mln_map_snapshot) - 1};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_snapshot_get(map, &undersized, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_snapshot_get(map, NULL, NULL)
  );
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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_debug_options(map, 0, &hold, NULL)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(&gate), "the runtime worker never parked"
  );

  mln_test_completion replaced = mln_test_completion_default(0);
  mln_test_completion newest = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_resize(
      map, (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.0},
      &replaced.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_resize(
      map, (mln_logical_extent){.width = 64, .height = 32, .scale_factor = 1.0},
      &newest.descriptor, NULL
    )
  );
  mln_test_gate_release(&gate);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&replaced));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_SUPERSEDED,
    mln_test_completion_disposition(&replaced)
  );
  TEST_ASSERT_EQUAL_UINT64(0, mln_test_completion_generation(&replaced));
  mln_test_completion_destroy(&replaced);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&newest));
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
      TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, state->statuses[index]);
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

MLN_TEST_GROUP {
  RUN_TEST(every_option_command_round_trips_through_the_snapshot);
  RUN_TEST(option_descriptors_reject_what_they_cannot_express);
  RUN_TEST(map_debug_commands_reject_a_null_map);
  RUN_TEST(map_extent_snapshot_tracks_resize_and_fixes_scale_factor);
  RUN_TEST(a_replaced_resize_completes_superseded);
  RUN_TEST(generations_increase_across_submitting_threads);
}
