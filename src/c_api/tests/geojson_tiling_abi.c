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
    // Integer hundredths of a degree: %f would honor a locale someone in the
    // process set (a GL/Vulkan driver init is enough) and print a decimal
    // comma, which breaks the JSON.
    const int longitude_hundredths = -12300 + (int)(index % 100);
    const int latitude_hundredths = 3700 + (int)(index / 100);
    const int written = snprintf(
      json + used, capacity - used,
      "%s{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
      "\"coordinates\":[%d.%02d,%d.%02d]},\"properties\":{}}",
      index == 0 ? "" : ",", longitude_hundredths / 100,
      abs(longitude_hundredths) % 100, latitude_hundredths / 100,
      latitude_hundredths % 100
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
  // flight. Each demand is serviced and drained before the next one so the
  // queue never grows.
  probe.render_status = MLN_STATUS_OK;
  while (!atomic_load(&probe.stop)) {
    mln_frame_demand demand = mln_frame_demand_default();
    demand.flags = 0;
    mln_status status =
      mln_render_session_request_frame(fixture.session, &demand);
    if (status != MLN_STATUS_OK) {
      probe.render_status = status;
      break;
    }
    bool settled = false;
    while (!settled && status == MLN_STATUS_OK && !atomic_load(&probe.stop)) {
      if (fixture.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
        size_t serviced = 0;
        status = mln_render_session_service_driver_work(
          fixture.session, SIZE_MAX, &serviced
        );
        if (status != MLN_STATUS_OK) {
          break;
        }
      }
      mln_render_session_snapshot snapshot = {
        .size = sizeof(mln_render_session_snapshot)
      };
      status = mln_render_session_get_snapshot(fixture.session, &snapshot);
      if (status != MLN_STATUS_OK) {
        break;
      }
      if (snapshot.pending_demand_count != 0) {
        mln_test_sleep_millisecond();
        continue;
      }
      mln_render_frame_batch batch = MLN_HANDLE_NULL;
      const mln_status drain_status =
        mln_render_session_drain_frame_results(fixture.session, &batch);
      if (drain_status == MLN_STATUS_OK) {
        mln_render_frame_batch_release(batch);
        settled = true;
      } else if (drain_status != MLN_STATUS_NOT_READY) {
        status = drain_status;
      }
    }
    if (status != MLN_STATUS_OK) {
      probe.render_status = status;
      break;
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
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(tiling_style_json))
  );

  mln_camera_update camera_update = mln_camera_update_default();
  camera_update.camera.fields =
    MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera_update.camera.latitude = 37.4;
  camera_update.camera.longitude = -122.5;
  camera_update.camera.zoom = 7.0;
  const mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_update_camera(map, &camera_update, &discard)
  );

  char* json = build_feature_collection();
  TEST_ASSERT_NOT_NULL(json);
  const mln_buffer_view document = mln_test_buffer_view(json, strlen(json));

  mln_geojson_source_data current = MLN_HANDLE_NULL;
  const mln_status create_status =
    mln_geojson_source_data_create(document, &options, &current);
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    MLN_STATUS_OK, create_status, mln_thread_last_error_message()
  );

  const mln_buffer_view source_id = MLN_BUFFER_LITERAL("points");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, source_id, current, &discard)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_style_layer_json(
                     map, MLN_BUFFER_LITERAL(tiling_layer_json),
                     MLN_BUFFER_LITERAL(""), &discard
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));

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

    // Back-to-back replacements keep the replacement period shorter than one
    // slice, so each replacement lands while the slice is still in flight.
    for (unsigned int index = 0; loop_status == MLN_STATUS_OK &&
                                 index < count && !atomic_load(&probe.finished);
         index += 1) {
      mln_test_drain_all(runtime);
      loop_status = mln_map_set_geojson_source_data(
        map, source_id, prepared[index], &discard
      );
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
  // One wait budget is enough on a hardware backend, but a software-rendered
  // CI runner can take far longer to wind the render thread down, so wait on
  // a deadline instead.
  const uint64_t deadline = mln_test_monotonic_milliseconds() + 30000;
  bool finished = false;
  while (!(finished = mln_test_wait_until(runtime, &probe.finished)) &&
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
