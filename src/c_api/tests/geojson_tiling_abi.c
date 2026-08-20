// Raw C ABI coverage: replacing prepared GeoJSON data while asynchronous tile
// slices are in flight.
// https://github.com/maplibre/maplibre-native-ffi/issues/644

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// Ten datasets per batch: the sequenced-scheduler round-robin has ten slots,
// and a batch is created before its replacement loop runs, so every dataset
// in a batch gets its own worker. Each batch then dies worker by worker in
// the loop, and every death is a chance for the replaced dataset's slices to
// still be in flight.
#define TILING_BATCH_SIZE 10
#define TILING_BATCHES 5

// A few thousand points match the field repro's dataset shape; the exact
// size matters less than replacing faster than the slicing worker drains.
#define TILING_POINT_COUNT 8000

// The style renders without the network; the layer makes the source's tiles
// visible so every replacement re-requests them.
static const char tiling_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":"
  "[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#000000\"}}]}";

static const char tiling_layer_json[] =
  "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"points\","
  "\"paint\":{\"fill-color\":\"#ff0000\"}}";

// Zero tolerance keeps every pyramid level slicing the full dataset instead
// of simplifying it away, which stretches each dataset's slice backlog.
static mln_geojson_source_options tiling_data_options(void) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields = MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
  options.tolerance = 0.0;
  return options;
}

// Builds a FeatureCollection of TILING_POINT_COUNT points around the camera
// center. Returns malloc'd bytes the caller frees, or null on allocation
// failure.
static char* build_feature_collection(void) {
  // A point serializes to at most "[-123.006,37.006]," plus about 80 bytes of
  // feature structure.
  const size_t capacity = (size_t)TILING_POINT_COUNT * 104 + 64;
  char* json = malloc(capacity);
  if (json == NULL) {
    return NULL;
  }
  size_t used = (size_t)snprintf(
    json, capacity, "{\"type\":\"FeatureCollection\",\"features\":["
  );
  for (unsigned int index = 0; index < TILING_POINT_COUNT; index += 1) {
    const double longitude = -123.0 + (double)(index % 100) * 0.01;
    const double latitude = 37.0 + (double)(index / 100) * 0.01;
    const int written = snprintf(
      json + used, capacity - used,
      "%s{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
      "\"coordinates\":[%.3f,%.3f]},\"properties\":{}}",
      index == 0 ? "" : ",", longitude, latitude
    );
    if (written <= 0 || (size_t)written >= capacity - used) {
      free(json);
      return NULL;
    }
    used += (size_t)written;
  }
  (void)snprintf(json + used, capacity - used, "]}");
  return json;
}

typedef struct tiling_render_probe {
  mln_map map;
  atomic_bool stop;
  atomic_bool finished;
  bool attached;
  mln_status render_status;
} tiling_render_probe;

// Static so a render thread that misses the shutdown deadline still reads
// valid state; the test fails its assertion instead of dangling.
static tiling_render_probe probe;

static void render_until_stopped(void* argument) {
  (void)argument;
  mln_test_render_fixture fixture = {0};
  probe.attached = mln_test_render_fixture_create(probe.map, &fixture);
  if (!probe.attached) {
    atomic_store(&probe.finished, true);
    return;
  }

  // The target stays small so each render applies the replacement quickly,
  // dropping the previous dataset's references while its slices are in
  // flight.
  probe.render_status = MLN_STATUS_OK;
  while (!atomic_load(&probe.stop)) {
    mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
    bool needs_repaint = false;
    const mln_status status = mln_render_session_render_update(
      fixture.session, &result, &needs_repaint
    );
    if (status != MLN_STATUS_OK) {
      probe.render_status = status;
      break;
    }
    if (result != MLN_RENDER_RESULT_RENDERED) {
      mln_test_sleep_millisecond();
    }
  }

  mln_test_render_fixture_destroy(&fixture);
  atomic_store(&probe.finished, true);
}

// Default options tile asynchronously, so every replacement schedules slice
// tasklets on the dataset's sequenced worker. The loop completing is the
// assertion: the failure mode is a SIGABRT that kills the suite.
static void replacing_data_during_async_tiling_survives(void) {
  const mln_geojson_source_options options = tiling_data_options();

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(tiling_style_json))
  );

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 37.4;
  camera.longitude = -122.5;
  camera.zoom = 7.0;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_jump_to(map, &camera));

  char* json = build_feature_collection();
  TEST_ASSERT_NOT_NULL(json);
  const mln_buffer_view document = mln_test_buffer_view(json, strlen(json));

  mln_geojson_source_data current = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(document, &options, &current)
  );

  const mln_buffer_view source_id = MLN_BUFFER_LITERAL("points");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_data(map, source_id, current)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map, MLN_BUFFER_LITERAL(tiling_layer_json), MLN_BUFFER_LITERAL("")
    )
  );

  // No assertion may run while the render thread is live: Unity failures
  // longjmp past the thread cleanup. From here on the test records statuses
  // and reports them after the join.
  probe = (tiling_render_probe){.map = map};
  atomic_init(&probe.stop, false);
  atomic_init(&probe.finished, false);
  mln_test_thread* render_thread =
    mln_test_thread_start(render_until_stopped, &probe);

  mln_status loop_status = MLN_STATUS_OK;
  for (unsigned int batch = 0;
       loop_status == MLN_STATUS_OK && batch < TILING_BATCHES &&
       !atomic_load(&probe.finished);
       batch += 1) {
    mln_geojson_source_data prepared[TILING_BATCH_SIZE] = {0};
    unsigned int count = 0;
    for (; count < TILING_BATCH_SIZE; count += 1) {
      loop_status =
        mln_geojson_source_data_create(document, &options, &prepared[count]);
      if (loop_status != MLN_STATUS_OK) {
        break;
      }
    }

    // A non-blocking pump keeps the replacement period shorter than one
    // slice, so each replacement lands while the slice is still in flight.
    for (unsigned int index = 0; loop_status == MLN_STATUS_OK &&
                                 index < count && !atomic_load(&probe.finished);
         index += 1) {
      loop_status = mln_runtime_pump(runtime, 0, -1);
      mln_test_drain_all(runtime);
      loop_status =
        mln_map_set_geojson_source_data(map, source_id, prepared[index]);
      if (loop_status == MLN_STATUS_OK) {
        mln_geojson_source_data_destroy(current);
        current = prepared[index];
        prepared[index] = MLN_HANDLE_NULL;
      }
    }
    for (unsigned int index = 0; index < TILING_BATCH_SIZE; index += 1) {
      mln_geojson_source_data_destroy(prepared[index]);
    }
  }
  free(json);

  atomic_store(&probe.stop, true);
  // One pump_until budget is enough on a hardware backend, but a
  // software-rendered CI runner can take far longer to wind the render
  // thread down, so wait on a deadline instead.
  const uint64_t deadline = mln_test_monotonic_milliseconds() + 30000;
  bool finished = false;
  while (!(finished = mln_test_pump_until(runtime, &probe.finished)) &&
         mln_test_monotonic_milliseconds() < deadline) {
  }
  // Join only a finished thread; a wedged one fails an assertion below
  // rather than hanging the suite, and the static probe stays valid for it.
  if (finished) {
    mln_test_thread_join(render_thread);
  }

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, loop_status);
  TEST_ASSERT_TRUE(finished);
  TEST_ASSERT_TRUE(probe.attached);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.render_status);

  mln_geojson_source_data_destroy(current);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_geojson_tiling_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(replacing_data_during_async_tiling_survives);
}
