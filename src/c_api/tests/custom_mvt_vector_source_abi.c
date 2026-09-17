// Raw C ABI coverage for the custom MVT vector source release callback, the
// only report that a host's callback state is no longer referenced.
//
// These tests live below the bindings because the contract under test is that
// the release runs independently of the event mask. A binding observes its own
// release only through that same mechanism.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static const char source_id[] = "custom-mvt-vector";
static const char geometry_source_id[] = "custom-geometry";

// The counters are written from native threads and read from the test thread,
// so they are atomic; the barrier polls below provide the ordering.
typedef struct release_probe {
  atomic_size_t release_count;
  atomic_size_t fetch_count;
} release_probe;

static void probe_fetch_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)tile_id;
  atomic_fetch_add(&((release_probe*)user_data)->fetch_count, 1);
}

static void probe_release(void* user_data) {
  atomic_fetch_add(&((release_probe*)user_data)->release_count, 1);
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

static mln_map create_map_without_style_events(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 256;
  options.initial_extent.height = 256;
  options.event_mask = MLN_RUNTIME_EVENT_MASK_ALL &
                       ~(uint64_t)MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED;
  return mln_test_create_map_with_options(runtime, &options);
}

static void add_mvt_source(mln_map map, release_probe* probe) {
  mln_custom_mvt_vector_source_options options = probe_options(probe);
  mln_test_completion add = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL(source_id), &options, &add.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&add));
}

// Waits out one submitted command and returns its terminal status.
// A host that never subscribes to style-loaded events still learns that a style
// replacement dropped its source.
static void a_style_replacement_releases_a_dropped_source_unsubscribed(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_without_style_events(runtime);
  release_probe probe = {0};

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_mvt_source(map, &probe);
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&probe.release_count));

  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.release_count));
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  // The map no longer references the state, so retiring it releases nothing.
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

static void an_explicit_removal_releases_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe probe = {0};

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_mvt_source(map, &probe);

  mln_test_completion removal = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_remove_style_source(
                     map, MLN_BUFFER_LITERAL(source_id), &removal.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&removal));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.release_count));

  // A style load after the removal has nothing left to reconcile.
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

// Synchronous rejection never references user_data. An asynchronously rejected
// accepted command does reference it and therefore releases it. Retiring the
// map then releases the successful source's state exactly once.
static void accepted_adds_release_their_callback_state(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe probe = {0};

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_custom_mvt_vector_source_options options = probe_options(&probe);
  options.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM;
  options.min_zoom = -1;
  mln_test_completion rejected = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL(source_id), &options, &rejected.descriptor
    )
  );
  rejected.descriptor.release_user_data(rejected.descriptor.user_data);
  mln_test_completion_destroy(&rejected);

  options = probe_options(&probe);
  options.fetch_tile = NULL;
  mln_test_completion missing_fetch = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL(source_id), &options, &missing_fetch.descriptor
    )
  );
  missing_fetch.descriptor.release_user_data(
    missing_fetch.descriptor.user_data
  );
  mln_test_completion_destroy(&missing_fetch);

  options = probe_options(&probe);
  mln_test_completion first = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL(source_id), &options, &first.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&first));
  // The duplicate command is accepted, then fails application because the ID
  // already exists. Its callback state is released independently.
  mln_test_completion duplicate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL(source_id), &options, &duplicate.descriptor
    )
  );
  TEST_ASSERT_NOT_EQUAL(MLN_STATUS_OK, mln_test_completion_settle(&duplicate));
  // The duplicate's release is not ordered before a single barrier.
  mln_test_barrier_until_count(
    runtime, &probe.release_count, 1,
    "the duplicate custom MVT vector source never released its callback state"
  );

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(2, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

static void tile_delivery_and_invalidate_accept_an_empty_tile(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe probe = {0};

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_mvt_source(map, &probe);

  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_source_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(
                     map, MLN_BUFFER_LITERAL(source_id), &info.descriptor
                   )
  );
  mln_style_source_result source_result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish_value(
                     &info, &source_result, sizeof(source_result)
                   )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR, source_result.info.type
  );

  const mln_canonical_tile_id tile_id = {.z = 0, .x = 0, .y = 0};
  const mln_buffer_view empty = {.data = NULL, .size = 0};
  mln_test_completion data = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_custom_mvt_vector_source_tile_data(
      map, MLN_BUFFER_LITERAL(source_id), tile_id, empty, &data.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&data));
  mln_test_completion error = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_custom_mvt_vector_source_tile_error(
                     map, MLN_BUFFER_LITERAL(source_id), tile_id,
                     MLN_BUFFER_LITERAL("missing"), &error.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&error));
  mln_test_completion invalidate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_invalidate_custom_mvt_vector_source_tile(
      map, MLN_BUFFER_LITERAL(source_id), tile_id, &invalidate.descriptor
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&invalidate));

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.release_count));
  mln_test_destroy_runtime(runtime);
}

static void tile_operations_reject_the_other_custom_source_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  release_probe mvt_probe = {0};
  release_probe geometry_probe = {0};

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  add_mvt_source(map, &mvt_probe);
  mln_custom_geometry_source_options geometry_options =
    geometry_probe_options(&geometry_probe);
  mln_test_completion add = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_custom_geometry_source(
                     map, MLN_BUFFER_LITERAL(geometry_source_id),
                     &geometry_options, &add.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&add));

  // A tile operation against the other custom source kind is accepted, then
  // fails application on the worker.
  const mln_canonical_tile_id tile_id = {.z = 0, .x = 0, .y = 0};
  const mln_buffer_view empty = {.data = NULL, .size = 0};
  mln_test_completion wrong_data = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_custom_mvt_vector_source_tile_data(
                     map, MLN_BUFFER_LITERAL(geometry_source_id), tile_id,
                     empty, &wrong_data.descriptor
                   )
  );
  TEST_ASSERT_NOT_EQUAL(MLN_STATUS_OK, mln_test_completion_settle(&wrong_data));
  mln_test_completion wrong_error = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_custom_mvt_vector_source_tile_error(
                     map, MLN_BUFFER_LITERAL(geometry_source_id), tile_id,
                     MLN_BUFFER_LITERAL("missing"), &wrong_error.descriptor
                   )
  );
  TEST_ASSERT_NOT_EQUAL(
    MLN_STATUS_OK, mln_test_completion_settle(&wrong_error)
  );
  mln_test_completion wrong_invalidate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_invalidate_custom_mvt_vector_source_tile(
                     map, MLN_BUFFER_LITERAL(geometry_source_id), tile_id,
                     &wrong_invalidate.descriptor
                   )
  );
  TEST_ASSERT_NOT_EQUAL(
    MLN_STATUS_OK, mln_test_completion_settle(&wrong_invalidate)
  );
  mln_test_completion wrong_geometry = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_custom_geometry_source_tile_data(
      map, MLN_BUFFER_LITERAL(source_id), tile_id,
      MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
      &wrong_geometry.descriptor
    )
  );
  TEST_ASSERT_NOT_EQUAL(
    MLN_STATUS_OK, mln_test_completion_settle(&wrong_geometry)
  );
  mln_test_completion wrong_geometry_invalidate =
    mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_invalidate_custom_geometry_source_tile(
                     map, MLN_BUFFER_LITERAL(source_id), tile_id,
                     &wrong_geometry_invalidate.descriptor
                   )
  );
  TEST_ASSERT_NOT_EQUAL(
    MLN_STATUS_OK, mln_test_completion_settle(&wrong_geometry_invalidate)
  );

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&mvt_probe.release_count));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&geometry_probe.release_count));
  mln_test_destroy_runtime(runtime);
}

void run_custom_mvt_vector_source_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(a_style_replacement_releases_a_dropped_source_unsubscribed);
  RUN_TEST(an_explicit_removal_releases_once);
  RUN_TEST(accepted_adds_release_their_callback_state);
  RUN_TEST(tile_delivery_and_invalidate_accept_an_empty_tile);
  RUN_TEST(tile_operations_reject_the_other_custom_source_kind);
}
