// Raw C ABI coverage for the custom MVT vector source release callback, the
// only report that a host's callback state is no longer referenced.
//
// These tests live below the bindings because the contract under test is that
// the release runs independently of the event mask. A binding observes its own
// release only through that same mechanism.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\",\"type\":"
  "\"background\",\"paint\":{\"background-color\":\"#102030\"}}]}";
static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";
static const char source_id[] = "custom-mvt-vector";
static const char geometry_source_id[] = "custom-geometry";
static const size_t style_pump_attempts = 200;

typedef struct release_probe {
  size_t release_count;
  size_t fetch_count;
} release_probe;

static void probe_fetch_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)tile_id;
  ((release_probe*)user_data)->fetch_count += 1;
}

static void probe_release(void* user_data) {
  ((release_probe*)user_data)->release_count += 1;
}

static mln_custom_mvt_vector_source_options probe_options(
  release_probe* probe
) {
  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fetch_tile = probe_fetch_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  return options;
}

static mln_custom_geometry_source_options geometry_probe_options(
  release_probe* probe
) {
  mln_custom_geometry_source_options options =
    mln_custom_geometry_source_options_default();
  options.fetch_tile = probe_fetch_tile;
  options.user_data = probe;
  options.release_user_data = probe_release;
  return options;
}

// Loads an inline style and pumps, so the map reaches a loaded style without
// reaching the network.
static void load_style_and_pump(
  mln_runtime runtime, mln_map map, mln_buffer_view style_json
) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_set_style_json(map, style_json));
  for (size_t attempt = 0; attempt < style_pump_attempts; attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 2, -1));
  }
}

static mln_map create_map_without_style_events(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.width = 256;
  options.height = 256;
  options.event_mask = MLN_RUNTIME_EVENT_MASK_ALL &
                       ~(uint64_t)MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED;
  return mln_test_create_map_with_options(runtime, &options);
}

static void add_mvt_source(mln_map map, release_probe* probe) {
  mln_custom_mvt_vector_source_options options = probe_options(probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_custom_mvt_vector_source(
                     map, MLN_BUFFER_LITERAL(source_id), &options
                   )
  );
}

// A host that never subscribes to style-loaded events still learns that a style
// replacement dropped its source.
static void a_style_replacement_releases_a_dropped_source_unsubscribed(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_without_style_events(runtime);
  release_probe probe = {0};

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(background_style_json));
  add_mvt_source(map, &probe);
  TEST_ASSERT_EQUAL_size_t(0, probe.release_count);

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(empty_style_json));
  TEST_ASSERT_EQUAL_size_t(1, probe.release_count);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, probe.release_count);
  mln_test_destroy_runtime(runtime);
}

static void an_explicit_removal_releases_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe probe = {0};

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(background_style_json));
  add_mvt_source(map, &probe);

  bool removed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_source(map, MLN_BUFFER_LITERAL(source_id), &removed)
  );
  TEST_ASSERT_TRUE(removed);
  TEST_ASSERT_EQUAL_size_t(1, probe.release_count);

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(empty_style_json));
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, probe.release_count);
  mln_test_destroy_runtime(runtime);
}

// A rejected add never referenced user_data, so releasing it would free state
// the host still owns. Retiring the map then releases the accepted source's
// state exactly once.
static void a_rejected_add_releases_nothing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe probe = {0};

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(background_style_json));
  mln_custom_mvt_vector_source_options options = probe_options(&probe);
  options.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
  options.min_zoom = -1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_custom_mvt_vector_source(
                                   map, MLN_BUFFER_LITERAL(source_id), &options
                                 )
  );

  options = probe_options(&probe);
  options.fetch_tile = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_custom_mvt_vector_source(
                                   map, MLN_BUFFER_LITERAL(source_id), &options
                                 )
  );

  options = probe_options(&probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_custom_mvt_vector_source(
                     map, MLN_BUFFER_LITERAL(source_id), &options
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_custom_mvt_vector_source(
                                   map, MLN_BUFFER_LITERAL(source_id), &options
                                 )
  );
  TEST_ASSERT_EQUAL_size_t(0, probe.release_count);

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, probe.release_count);
  mln_test_destroy_runtime(runtime);
}

static void tile_delivery_and_invalidate_accept_an_empty_tile(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe probe = {0};

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(background_style_json));
  add_mvt_source(map, &probe);

  uint32_t source_type = 0;
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_type(
                     map, MLN_BUFFER_LITERAL(source_id), &source_type, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR, source_type
  );

  const mln_canonical_tile_id tile_id = {.z = 0, .x = 0, .y = 0};
  const mln_buffer_view empty = {.data = NULL, .size = 0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_custom_mvt_vector_source_tile_data(
                     map, MLN_BUFFER_LITERAL(source_id), tile_id, empty
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_custom_mvt_vector_source_tile_error(
      map, MLN_BUFFER_LITERAL(source_id), tile_id, MLN_BUFFER_LITERAL("missing")
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_invalidate_custom_mvt_vector_source_tile(
                     map, MLN_BUFFER_LITERAL(source_id), tile_id
                   )
  );

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, probe.release_count);
  mln_test_destroy_runtime(runtime);
}

static void tile_operations_reject_the_other_custom_source_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe mvt_probe = {0};
  release_probe geometry_probe = {0};

  load_style_and_pump(runtime, map, MLN_BUFFER_LITERAL(background_style_json));
  add_mvt_source(map, &mvt_probe);
  mln_custom_geometry_source_options geometry_options =
    geometry_probe_options(&geometry_probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_custom_geometry_source(
      map, MLN_BUFFER_LITERAL(geometry_source_id), &geometry_options
    )
  );

  const mln_canonical_tile_id tile_id = {.z = 0, .x = 0, .y = 0};
  const mln_buffer_view empty = {.data = NULL, .size = 0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_mvt_vector_source_tile_data(
      map, MLN_BUFFER_LITERAL(geometry_source_id), tile_id, empty
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_mvt_vector_source_tile_error(
      map, MLN_BUFFER_LITERAL(geometry_source_id), tile_id,
      MLN_BUFFER_LITERAL("missing")
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_invalidate_custom_mvt_vector_source_tile(
      map, MLN_BUFFER_LITERAL(geometry_source_id), tile_id
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_custom_geometry_source_tile_data(
      map, MLN_BUFFER_LITERAL(source_id), tile_id,
      MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}")
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_invalidate_custom_geometry_source_tile(
                                   map, MLN_BUFFER_LITERAL(source_id), tile_id
                                 )
  );

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, mvt_probe.release_count);
  TEST_ASSERT_EQUAL_size_t(1, geometry_probe.release_count);
  mln_test_destroy_runtime(runtime);
}

void run_custom_mvt_vector_source_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(a_style_replacement_releases_a_dropped_source_unsubscribed);
  RUN_TEST(an_explicit_removal_releases_once);
  RUN_TEST(a_rejected_add_releases_nothing);
  RUN_TEST(tile_delivery_and_invalidate_accept_an_empty_tile);
  RUN_TEST(tile_operations_reject_the_other_custom_source_kind);
}
