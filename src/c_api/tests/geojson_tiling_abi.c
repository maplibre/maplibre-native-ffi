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

// A few thousand points match the field repro's dataset shape; the exact
// size matters less than replacing faster than the slicing worker drains.
#define TILING_POINT_COUNT 8000
#define TILING_REPLACEMENTS 400

// One source, as in the field repro. Every GeoJSON source pins a slot of the
// sequenced-scheduler round-robin for its lifetime, and a dataset on a pinned
// slot never leaves its scheduler to its worker.
#define TILING_SOURCE_COUNT 1

// Prepared datasets the producer threads have ready for the map owner
// thread.
#define PIPELINE_CAPACITY 8

// The style renders without the network; the layers make the sources' tiles
// visible so every replacement re-requests them.
static const char tiling_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":"
  "[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#000000\"}}]}";

static const char tiling_layer_json[] =
  "{\"id\":\"fill-%u\",\"type\":\"fill\",\"source\":\"points-%u\","
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

static void render_until_stopped(void* argument) {
  tiling_render_probe* probe = (tiling_render_probe*)argument;
  mln_test_render_fixture fixture = {0};
  probe->attached = mln_test_render_fixture_create(probe->map, &fixture);
  if (!probe->attached) {
    atomic_store(&probe->finished, true);
    return;
  }

  // The target stays small so each render applies the replacement quickly,
  // dropping the previous dataset's references while its slices are in
  // flight.
  probe->render_status = MLN_STATUS_OK;
  while (!atomic_load(&probe->stop)) {
    mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
    bool needs_repaint = false;
    const mln_status status = mln_render_session_render_update(
      fixture.session, &result, &needs_repaint
    );
    if (status != MLN_STATUS_OK) {
      probe->render_status = status;
      break;
    }
    if (result != MLN_RENDER_RESULT_RENDERED) {
      mln_test_sleep_millisecond();
    }
  }

  mln_test_render_fixture_destroy(&fixture);
  atomic_store(&probe->finished, true);
}

// Producer threads prepare datasets from one shared document, so the map
// owner thread replaces data faster than the slicing worker slices it — the
// shape of the field repro.
#define PRODUCER_COUNT 4

typedef struct tiling_data_supply {
  mln_buffer_view json;
  const mln_geojson_source_options* options;
  atomic_bool stop;
  atomic_bool failed;
  atomic_uint ticket;
  atomic_uint claimed;
  atomic_uint tail;
  mln_geojson_source_data slots[PIPELINE_CAPACITY];
  atomic_uint ready[PIPELINE_CAPACITY];
} tiling_data_supply;

static void produce_prepared_data(void* argument) {
  tiling_data_supply* supply = (tiling_data_supply*)argument;
  while (!atomic_load(&supply->stop) && !atomic_load(&supply->failed)) {
    const unsigned int ticket = atomic_fetch_add(&supply->ticket, 1);
    if (ticket >= TILING_REPLACEMENTS) {
      return;
    }

    mln_geojson_source_data next = MLN_HANDLE_NULL;
    if (
      mln_geojson_source_data_create(supply->json, supply->options, &next) !=
      MLN_STATUS_OK
    ) {
      atomic_store(&supply->failed, true);
      return;
    }

    const unsigned int position = atomic_fetch_add(&supply->claimed, 1);
    while (atomic_load(&supply->tail) + PIPELINE_CAPACITY <= position &&
           !atomic_load(&supply->stop)) {
      mln_test_sleep_millisecond();
    }
    if (atomic_load(&supply->stop)) {
      mln_geojson_source_data_destroy(next);
      return;
    }
    supply->slots[position % PIPELINE_CAPACITY] = next;
    atomic_store_explicit(
      &supply->ready[position % PIPELINE_CAPACITY], position + 1,
      memory_order_release
    );
  }
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
  mln_geojson_source_data current = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      mln_test_buffer_view(json, strlen(json)), &options, &current
    )
  );

  char source_ids[TILING_SOURCE_COUNT][32];
  char layer_json[160];
  mln_buffer_view source_views[TILING_SOURCE_COUNT];
  for (unsigned int source = 0; source < TILING_SOURCE_COUNT; source += 1) {
    const int id_length = snprintf(
      source_ids[source], sizeof(source_ids[source]), "points-%u", source
    );
    TEST_ASSERT_TRUE(
      id_length > 0 && (size_t)id_length < sizeof(source_ids[source])
    );
    source_views[source] =
      mln_test_buffer_view(source_ids[source], (size_t)id_length);
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_map_add_geojson_source_data(map, source_views[source], current)
    );
    const int layer_length = snprintf(
      layer_json, sizeof(layer_json), tiling_layer_json, source, source
    );
    TEST_ASSERT_TRUE(
      layer_length > 0 && (size_t)layer_length < sizeof(layer_json)
    );
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_map_add_style_layer_json(
        map, mln_test_buffer_view(layer_json, (size_t)layer_length),
        MLN_BUFFER_LITERAL("")
      )
    );
  }

  tiling_render_probe probe = {.map = map};
  atomic_init(&probe.stop, false);
  atomic_init(&probe.finished, false);
  mln_test_thread* render_thread =
    mln_test_thread_start(render_until_stopped, &probe);

  tiling_data_supply supply = {
    .json = mln_test_buffer_view(json, strlen(json)), .options = &options
  };
  atomic_init(&supply.stop, false);
  atomic_init(&supply.failed, false);
  atomic_init(&supply.ticket, 0);
  atomic_init(&supply.claimed, 0);
  atomic_init(&supply.tail, 0);
  for (unsigned int slot = 0; slot < PIPELINE_CAPACITY; slot += 1) {
    atomic_init(&supply.ready[slot], 0);
  }
  mln_test_thread* producer_threads[PRODUCER_COUNT];
  for (unsigned int index = 0; index < PRODUCER_COUNT; index += 1) {
    producer_threads[index] =
      mln_test_thread_start(produce_prepared_data, &supply);
  }

  // Each pump queues the current dataset's slice tasklets; replacing and
  // releasing that dataset right after the pump lands while they run.
  //
  // A failed call longjmps past the thread cleanup below if it goes through
  // Unity here, so the loop records the first failure and reports it after
  // the joins.
  mln_status loop_status = MLN_STATUS_OK;
  while (loop_status == MLN_STATUS_OK &&
         atomic_load(&supply.tail) < TILING_REPLACEMENTS &&
         !atomic_load(&probe.finished) && !atomic_load(&supply.failed)) {
    const unsigned int position = atomic_load(&supply.tail);
    if (
      atomic_load_explicit(
        &supply.ready[position % PIPELINE_CAPACITY], memory_order_acquire
      ) != position + 1
    ) {
      loop_status = mln_runtime_pump(runtime, 2, -1);
      mln_test_drain_all(runtime);
      continue;
    }
    mln_geojson_source_data next = supply.slots[position % PIPELINE_CAPACITY];
    atomic_store(&supply.tail, position + 1);

    // A non-blocking pump keeps the replacement period shorter than one
    // slice, so the replacement lands while the slice is still in flight.
    loop_status = mln_runtime_pump(runtime, 0, -1);
    mln_test_drain_all(runtime);
    for (unsigned int source = 0;
         loop_status == MLN_STATUS_OK && source < TILING_SOURCE_COUNT;
         source += 1) {
      loop_status =
        mln_map_set_geojson_source_data(map, source_views[source], next);
    }
    if (loop_status == MLN_STATUS_OK) {
      mln_geojson_source_data_destroy(current);
      current = next;
    } else {
      mln_geojson_source_data_destroy(next);
    }
  }

  atomic_store(&supply.stop, true);
  for (unsigned int index = 0; index < PRODUCER_COUNT; index += 1) {
    mln_test_thread_join(producer_threads[index]);
  }
  free(json);

  atomic_store(&probe.stop, true);
  const bool render_finished = mln_test_pump_until(runtime, &probe.finished);
  mln_test_thread_join(render_thread);

  // The threads are joined, so assertions are safe from here on.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, loop_status);
  TEST_ASSERT_FALSE(atomic_load(&supply.failed));
  TEST_ASSERT_TRUE(render_finished);
  TEST_ASSERT_TRUE(probe.attached);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.render_status);

  // A handle the consumer never took is still the test's to release.
  for (unsigned int position = atomic_load(&supply.tail);
       position < atomic_load(&supply.claimed); position += 1) {
    if (
      atomic_load(&supply.ready[position % PIPELINE_CAPACITY]) == position + 1
    ) {
      mln_geojson_source_data_destroy(
        supply.slots[position % PIPELINE_CAPACITY]
      );
    }
  }
  mln_geojson_source_data_destroy(current);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_geojson_tiling_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(replacing_data_during_async_tiling_survives);
}
