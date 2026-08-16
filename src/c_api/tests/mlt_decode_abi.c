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

#define MLN_STRING_LITERAL(text) \
  ((mln_buffer_view){.data = (text), .size = sizeof(text) - 1})

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

// Renders and pumps until the source reports features or the attempts run out.
// Tiles arrive through a worker thread, so the count stays zero until the map
// has parsed the tile and folded it into the render tree.
static size_t query_admin_feature_count(
  mln_runtime runtime, mln_render_session session
) {
  const mln_buffer_view source_layers[] = {MLN_STRING_LITERAL("admin")};
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
  options.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  options.source_layer_ids = source_layers;
  options.source_layer_id_count = 1;

  size_t count = 0;
  for (unsigned int attempt = 0; attempt < 600 && count == 0; attempt += 1) {
    mln_render_result render_result = MLN_RENDER_RESULT_NO_UPDATE;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_render_update(session, &render_result)
    );
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));

    mln_queried_feature_list result = MLN_HANDLE_NULL;
    const mln_status status = mln_render_session_query_source_features(
      session, MLN_STRING_LITERAL("mlt-source"), &options, &result
    );
    if (status == MLN_STATUS_OK) {
      TEST_ASSERT_EQUAL_INT(
        MLN_STATUS_OK, mln_queried_feature_list_count(result, &count)
      );
    }
    mln_queried_feature_list_destroy(result);
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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_set_resource_provider(runtime, &provider)
  );

  mln_map_options map_options = mln_map_options_default();
  map_options.width = 64;
  map_options.height = 64;
  map_options.fast_pfor_enabled = fast_pfor_enabled;
  mln_map map = mln_test_create_map_with_options(runtime, &map_options);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(mlt_style_json))
  );

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  const size_t count = query_admin_feature_count(runtime, fixture.session);
  const int served = atomic_load(&provider_state.served);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_clear_resource_provider(runtime)
  );
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
