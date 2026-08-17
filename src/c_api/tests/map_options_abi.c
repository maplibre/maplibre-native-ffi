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
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 11.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  return camera;
}

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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_is_gesture_in_progress(fixture.map, NULL)
  );
  bool in_progress = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_is_gesture_in_progress(MLN_HANDLE_NULL, &in_progress)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_gesture_in_progress(MLN_HANDLE_NULL, true)
  );
  destroy_map_fixture(fixture);
}

typedef struct transition_event_tally {
  uint32_t finished_count;
  uint64_t last_transition_id;
  bool did_change_followed_finish;
  int32_t last_did_change_code;
} transition_event_tally;

static transition_event_tally drain_transition_events(mln_runtime runtime) {
  transition_event_tally tally = {0, 0, false, -1};
  for (;;) {
    mln_runtime_event_batch batch = mln_runtime_event_batch_default();
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
    );
    if (batch.event_count == 0) {
      return tally;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      if (event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED) {
        TEST_ASSERT_EQUAL_UINT32(
          MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED,
          event->payload_type
        );
        tally.finished_count += 1;
        tally.last_transition_id =
          event->payload.camera_transition_finished.transition_id;
      } else if (event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE) {
        tally.last_did_change_code = event->code;
        if (tally.finished_count > 0) {
          tally.did_change_followed_finish = true;
        }
      }
    }
  }
}

static mln_animation_options transition_animation(
  uint64_t transition_id, double duration_ms
) {
  mln_animation_options animation = mln_animation_options_default();
  animation.fields =
    MLN_ANIMATION_OPTION_TRANSITION_ID | MLN_ANIMATION_OPTION_DURATION;
  animation.transition_id = transition_id;
  animation.duration_ms = duration_ms;
  return animation;
}

static void camera_transition_id_reports_every_terminal_outcome(void) {
  map_fixture fixture = create_map_fixture();
  mln_camera_options camera = test_camera();

  // A zero-duration ease resolves inside the call, so the event is queued
  // before mln_map_ease_to() returns and lands ahead of the did-change event.
  mln_animation_options animation = transition_animation(7, 0.0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_ease_to(fixture.map, &camera, &animation)
  );
  transition_event_tally tally = drain_transition_events(fixture.runtime);
  TEST_ASSERT_EQUAL_UINT32(1, tally.finished_count);
  TEST_ASSERT_EQUAL_UINT64(7, tally.last_transition_id);
  TEST_ASSERT_TRUE(tally.did_change_followed_finish);
  TEST_ASSERT_EQUAL_INT32(
    MLN_CAMERA_CHANGE_MODE_IMMEDIATE, tally.last_did_change_code
  );

  // A superseded transition reports its end, and so does the superseding one
  // when it is cancelled.
  camera.zoom = 12.0;
  animation = transition_animation(11, 5000.0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_ease_to(fixture.map, &camera, &animation)
  );
  tally = drain_transition_events(fixture.runtime);
  TEST_ASSERT_EQUAL_UINT32(0, tally.finished_count);

  camera.zoom = 13.0;
  animation = transition_animation(12, 5000.0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_ease_to(fixture.map, &camera, &animation)
  );
  tally = drain_transition_events(fixture.runtime);
  TEST_ASSERT_EQUAL_UINT32(1, tally.finished_count);
  TEST_ASSERT_EQUAL_UINT64(11, tally.last_transition_id);
  TEST_ASSERT_EQUAL_INT32(
    MLN_CAMERA_CHANGE_MODE_ANIMATED, tally.last_did_change_code
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_cancel_transitions(fixture.map));
  tally = drain_transition_events(fixture.runtime);
  TEST_ASSERT_EQUAL_UINT32(1, tally.finished_count);
  TEST_ASSERT_EQUAL_UINT64(12, tally.last_transition_id);

  // Omitting the field leaves the transition silent.
  animation = mln_animation_options_default();
  camera.zoom = 14.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_ease_to(fixture.map, &camera, &animation)
  );
  tally = drain_transition_events(fixture.runtime);
  TEST_ASSERT_EQUAL_UINT32(0, tally.finished_count);

  destroy_map_fixture(fixture);
}

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
    mln_map_camera_for_geometry(
      fixture.map, (mln_buffer_view){0}, NULL, &camera
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_lat_lng_bounds_for_camera(fixture.map, NULL, NULL)
  );
  destroy_map_fixture(fixture);
}

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
  options = mln_bound_options_default();
  options.fields = MLN_BOUND_OPTION_BOUNDS | MLN_BOUND_OPTION_UNBOUNDED;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_bounds(fixture.map, &options)
  );
  destroy_map_fixture(fixture);
}

static bool near_longitude(double actual, double expected) {
  const double delta = actual - expected;
  return delta > -1e-6 && delta < 1e-6;
}

static double jumped_longitude(mln_map map, double longitude) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 0.0;
  camera.longitude = longitude;
  camera.zoom = 2.0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_jump_to(map, &camera));
  mln_camera_options snapshot = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_get_camera(map, &snapshot));
  return snapshot.longitude;
}

// The unbounded state is distinct from world bounds, which the
// southwest/northeast pair alone cannot express.
static void camera_bounds_distinguish_unbounded_from_world(void) {
  map_fixture fixture = create_map_fixture();

  mln_bound_options snapshot = mln_bound_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_bounds(fixture.map, &snapshot)
  );
  TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
  TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
  // An unbounded map wraps across the antimeridian.
  TEST_ASSERT_TRUE(
    near_longitude(jumped_longitude(fixture.map, 200.0), -160.0)
  );

  mln_bound_options world = mln_bound_options_default();
  world.fields = MLN_BOUND_OPTION_BOUNDS;
  world.bounds.southwest.latitude = -90.0;
  world.bounds.southwest.longitude = -180.0;
  world.bounds.northeast.latitude = 90.0;
  world.bounds.northeast.longitude = 180.0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_set_bounds(fixture.map, &world));

  snapshot = mln_bound_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_bounds(fixture.map, &snapshot)
  );
  TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
  TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
  TEST_ASSERT_TRUE(near_longitude(snapshot.bounds.northeast.longitude, 180.0));
  // World bounds clamp at the antimeridian instead of wrapping.
  TEST_ASSERT_TRUE(near_longitude(jumped_longitude(fixture.map, 200.0), 180.0));

  mln_bound_options unbounded = mln_bound_options_default();
  unbounded.fields = MLN_BOUND_OPTION_UNBOUNDED;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_bounds(fixture.map, &unbounded)
  );

  snapshot = mln_bound_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_bounds(fixture.map, &snapshot)
  );
  TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
  TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
  // Releasing the constraint restores antimeridian wrapping.
  TEST_ASSERT_TRUE(
    near_longitude(jumped_longitude(fixture.map, 200.0), -160.0)
  );

  destroy_map_fixture(fixture);
}

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

typedef struct projection_thread_probe {
  mln_map_projection projection;
  mln_status camera_status;
  mln_status set_camera_status;
  mln_status conversion_status;
  mln_status destroy_status;
} projection_thread_probe;

static void use_projection_from_another_thread(void* argument) {
  projection_thread_probe* probe = argument;
  mln_camera_options camera = mln_camera_options_default();
  probe->camera_status =
    mln_map_projection_get_camera(probe->projection, &camera);
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 3.0;
  probe->set_camera_status =
    mln_map_projection_set_camera(probe->projection, &camera);
  mln_screen_point point = {0};
  probe->conversion_status =
    mln_map_projection_pixel_for_lat_lng(probe->projection, center, &point);
  probe->destroy_status = mln_map_projection_destroy(probe->projection);
  mln_test_release_thread_gpu_resources();
}

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

static void standalone_projection_rejects_invalid_arguments(void) {
  map_fixture fixture = create_map_fixture();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_create(fixture.map, NULL)
  );
  mln_map_projection projection = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_create(fixture.map, &projection)
  );
  projection = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create(fixture.map, &projection)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, projection);
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
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_set_visible_geometry(
                                   projection, (mln_buffer_view){0}, padding
                                 )
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

static void standalone_projection_is_usable_from_another_thread(void) {
  map_fixture fixture = create_map_fixture();
  projection_thread_probe probe = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create(fixture.map, &probe.projection)
  );
  mln_test_destroy_map(fixture.map);
  mln_test_destroy_runtime(fixture.runtime);

  mln_test_thread* worker =
    mln_test_thread_start(use_projection_from_another_thread, &probe);
  TEST_ASSERT_NOT_NULL(worker);
  mln_test_thread_join(worker);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.camera_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.set_camera_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.conversion_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.destroy_status);
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_get_camera(probe.projection, &camera)
  );
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
  uint32_t out_options = 0;
  bool out_bool = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_debug_options(MLN_HANDLE_NULL, 0)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_debug_options(fixture.map, UINT32_C(1) << 31)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_debug_options(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_debug_options(MLN_HANDLE_NULL, &out_options)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_rendering_stats_view_enabled(MLN_HANDLE_NULL, true)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_rendering_stats_view_enabled(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_rendering_stats_view_enabled(MLN_HANDLE_NULL, &out_bool)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_is_fully_loaded(fixture.map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_is_fully_loaded(MLN_HANDLE_NULL, &out_bool)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_dump_debug_logs(MLN_HANDLE_NULL)
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

static void map_size_tracks_attach_and_resize(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.width = 512;
  options.height = 256;
  options.scale_factor = 1.1;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  uint32_t width = 0;
  uint32_t height = 0;
  double scale_factor = 0.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(512, width);
  TEST_ASSERT_EQUAL_UINT32(256, height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, scale_factor);

  // The fixture attaches a 64x64 target at scale factor 1.0, and the map keeps
  // its own pixel ratio. A render session queues the size for the map's owner
  // thread, so the map keeps its previous size until the host pumps.
  mln_test_render_fixture render = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &render));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(512, width);
  TEST_ASSERT_EQUAL_UINT32(256, height);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(64, width);
  TEST_ASSERT_EQUAL_UINT32(64, height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, scale_factor);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(render.session, 96, 48, 1.0)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(96, width);
  TEST_ASSERT_EQUAL_UINT32(48, height);
  TEST_ASSERT_EQUAL_DOUBLE(1.1, scale_factor);
  mln_test_render_fixture_destroy(&render);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_size(MLN_HANDLE_NULL, &width, &height, &scale_factor)
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
  RUN_TEST(camera_transition_id_reports_every_terminal_outcome);
  RUN_TEST(camera_fitting_rejects_invalid_arguments);
  RUN_TEST(camera_bounds_constraints_reject_invalid_arguments);
  RUN_TEST(camera_bounds_distinguish_unbounded_from_world);
  RUN_TEST(free_camera_options_reject_raw_invalid_arguments);
  RUN_TEST(map_projection_mode_rejects_invalid_arguments);
  RUN_TEST(map_coordinate_conversion_rejects_invalid_arguments);
  RUN_TEST(standalone_projection_rejects_invalid_arguments);
  RUN_TEST(standalone_projection_is_usable_from_another_thread);
  RUN_TEST(projected_meters_reject_invalid_arguments);
  RUN_TEST(map_debug_options_reject_raw_invalid_arguments);
  RUN_TEST(map_options_default_leaves_fast_pfor_decoding_off);
  RUN_TEST(map_size_tracks_attach_and_resize);
  RUN_TEST(map_viewport_options_reject_invalid_arguments);
  RUN_TEST(map_tile_options_reject_invalid_arguments);
}
