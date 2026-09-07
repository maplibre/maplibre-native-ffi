// MLT tile decoding driven by mln_map_options.fast_pfor_enabled.
//
// The fixtures are the pair upstream uses for the same feature
// (test/tile/vector_tile.test.cpp, VectorTileData.MLTParseResults): one tile
// encoded with FastPFOR and one without, both carrying "water" and "admin"
// source layers.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// The source carries "encoding":"mlt" so the tiles parse as MapLibre Tiles, and
// a layer references it because a source only loads tiles once one does.
static const char mlt_style_json[] =
  "{\"version\":8,\"name\":\"mlt\",\"sources\":{\"mlt-source\":{"
  "\"type\":\"vector\",\"encoding\":\"mlt\","
  "\"tiles\":[\"https://example.com/mlt/{z}/{x}/{y}.mlt\"],"
  "\"minzoom\":0,\"maxzoom\":0}},\"layers\":[{\"id\":\"admin-lines\","
  "\"type\":\"line\",\"source\":\"mlt-source\",\"source-layer\":\"admin\"}]}";

typedef struct mlt_tile_provider {
  const uint8_t* bytes;
  size_t byte_count;
  atomic_int served;
} mlt_tile_provider;

// Serves the recorded tile for every request. The style is set inline, so tiles
// are the only resource that reaches the network file source.
static uint32_t serve_recorded_tile(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  mlt_tile_provider* state = user_data;
  (void)request;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = state->bytes,
    .byte_count = state->byte_count,
  };
  mln_resource_request_complete(handle, &response);
  mln_resource_request_release(handle);
  atomic_fetch_add(&state->served, 1);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}
// A tile whose encoding the map cannot decode yields no feature however long
// the test waits, so the render loop below is bounded by its own attempt count.
// The deadline caps the whole wait on top of that, which keeps the worst case
// off the product of the two nested loops.
enum {
  mlt_render_attempts = 600,
  mlt_render_deadline_milliseconds = 120000,
};

static bool wait_for_frame_result(
  const mln_test_render_fixture* fixture, mln_render_frame_batch* out_batch,
  uint64_t deadline
) {
  while (mln_test_monotonic_milliseconds() <= deadline) {
    if (fixture->driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
      size_t serviced = 0;
      if (
        mln_render_session_service_driver_work(
          fixture->session, SIZE_MAX, &serviced
        ) != MLN_STATUS_OK
      ) {
        return false;
      }
    }
    const mln_status status =
      mln_render_session_drain_frame_results(fixture->session, out_batch);
    if (status == MLN_STATUS_OK) {
      return true;
    }
    if (status != MLN_STATUS_NOT_READY) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// Renders until the source reports features or the attempts run out. Tiles
// arrive through a worker thread, so the count stays zero until the runtime
// worker parses the tile and folds it into the render tree.
static size_t query_admin_feature_count(
  mln_runtime runtime, const mln_test_render_fixture* fixture
) {
  const mln_buffer_view source_layers[] = {MLN_BUFFER_LITERAL("admin")};
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
  options.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  options.source_layer_ids = source_layers;
  options.source_layer_id_count = 1;

  size_t count = 0;
  const uint64_t deadline =
    mln_test_monotonic_milliseconds() + mlt_render_deadline_milliseconds;
  for (unsigned int attempt = 0; count == 0 && attempt < mlt_render_attempts &&
                                 mln_test_monotonic_milliseconds() <= deadline;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
    mln_frame_demand demand = mln_frame_demand_default();
    demand.flags = 0;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_request_frame(fixture->session, &demand)
    );

    // Frame-result readiness is the documented completion signal for a demand.
    // Waiting for it directly avoids placing query or barrier work behind a
    // frame whose renderer has not been created yet.
    mln_render_frame_batch frame_batch = MLN_HANDLE_NULL;
    TEST_ASSERT_TRUE(wait_for_frame_result(fixture, &frame_batch, deadline));
    size_t frame_result_count = 0;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_render_frame_batch_count(frame_batch, &frame_result_count)
    );
    TEST_ASSERT_EQUAL_size_t(1, frame_result_count);
    mln_render_frame_result frame_result = {
      .size = sizeof(mln_render_frame_result)
    };
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_frame_batch_get(frame_batch, 0, &frame_result)
    );
    mln_render_frame_batch_release(frame_batch);

    if (frame_result.disposition == MLN_RENDER_RESULT_RENDERED) {
      mln_acquired_frame frame = MLN_HANDLE_NULL;
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK,
        mln_render_session_acquire_frame(fixture->session, &frame)
      );
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK, mln_acquired_frame_release(&frame, NULL)
      );
    }

    mln_test_completion query = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_query_source_features(
                       fixture->session, MLN_BUFFER_LITERAL("mlt-source"),
                       &options, &query.descriptor
                     )
    );
    const mln_status query_status =
      mln_test_render_fixture_finish_operation(fixture, &query);
    if (query_status == MLN_STATUS_INVALID_STATE) {
      // A terminal NO_UPDATE result can precede creation of the renderer.
      // Retry after the next map update rather than ordering query work behind
      // an in-flight demand.
      mln_test_completion_destroy(&query);
    } else {
      TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, query_status);
      count = mln_test_completion_value_count(&query);
      mln_test_completion_destroy(&query);
    }
    if (count == 0) {
      mln_test_sleep_millisecond();
    }
  }
  return count;
}

// Loads one recorded MLT tile through a map created with the given FastPFOR
// setting and returns the "admin" feature count the source yields.
static size_t decode_recorded_tile(
  const char* fixture_relative_path, bool fast_pfor_enabled
) {
  size_t tile_size = 0;
  uint8_t* tile_bytes =
    mln_test_read_fixture(fixture_relative_path, &tile_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(
    tile_bytes,
    "MLT fixture missing; check third_party/maplibre-native/test/fixtures"
  );
  TEST_ASSERT_GREATER_THAN_UINT32(0, (uint32_t)tile_size);

  mlt_tile_provider provider_state = {
    .bytes = tile_bytes, .byte_count = tile_size, .served = 0
  };

  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = serve_recorded_tile,
    .user_data = &provider_state,
  };
  mln_test_completion provider_completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_set_resource_provider(
                     runtime, &provider, &provider_completion.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&provider_completion)
  );
  mln_test_completion_destroy(&provider_completion);

  mln_map_options map_options = mln_map_options_default();
  map_options.initial_extent.width = 64;
  map_options.initial_extent.height = 64;
  map_options.fast_pfor_enabled = fast_pfor_enabled;
  mln_map map = mln_test_create_map_with_options(runtime, &map_options);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(mlt_style_json))
  );

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  const size_t count = query_admin_feature_count(runtime, &fixture);
  const int served = atomic_load(&provider_state.served);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_completion clear_completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_clear_resource_provider(runtime, &clear_completion.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&clear_completion)
  );
  mln_test_completion_destroy(&clear_completion);
  mln_test_destroy_runtime(runtime);
  free(tile_bytes);

  // A zero count means nothing about decoding if the tile never arrived.
  TEST_ASSERT_GREATER_THAN_INT(0, served);
  return count;
}

static void fast_pfor_option_gates_mlt_tile_decoding(void) {
  TEST_ASSERT_GREATER_THAN_UINT32(
    0, (uint32_t)decode_recorded_tile("map/issue12432/0-0-0-fastpfor.mlt", true)
  );
  TEST_ASSERT_EQUAL_UINT32(
    0,
    (uint32_t)decode_recorded_tile("map/issue12432/0-0-0-fastpfor.mlt", false)
  );
  TEST_ASSERT_GREATER_THAN_UINT32(
    0, (uint32_t)decode_recorded_tile("map/issue12432/0-0-0.mlt", false)
  );
}

void run_mlt_decode_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(fast_pfor_option_gates_mlt_tile_decoding);
}
