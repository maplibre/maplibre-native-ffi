// MLT tile decoding follows mln_map_options.fast_pfor_enabled, the only map
// option that reaches MapLibre's tile parser.
//
// The fixtures are the pair upstream uses for the same feature
// (test/tile/vector_tile.test.cpp, VectorTileData.MLTParseResults): one tile
// encoded with FastPFOR and one without, both carrying "water" and "admin"
// source layers. Every case waits for a positive signal. A static map's still
// image completes once the tile has loaded and rendered, after which a decoded
// tile yields its features. A tile the map cannot decode yields none, so that
// case waits instead for the tile parse warning that map.h documents.

#include "support/map.h"
#include "support/test_support.h"

// The source carries "encoding":"mlt" so the tiles parse as MapLibre Tiles, and
// a layer references it because a source only loads tiles once one does. The
// provider below serves every tile request, so the URL never reaches a network.
static const char mlt_style_json[] =
  "{\"version\":8,\"name\":\"mlt\",\"sources\":{\"mlt-source\":{"
  "\"type\":\"vector\",\"encoding\":\"mlt\","
  "\"tiles\":[\"custom://mlt/{z}/{x}/{y}.mlt\"],"
  "\"minzoom\":0,\"maxzoom\":0}},\"layers\":[{\"id\":\"admin-lines\","
  "\"type\":\"line\",\"source\":\"mlt-source\",\"source-layer\":\"admin\"}]}";

typedef struct mlt_tile {
  uint8_t* bytes;
  size_t byte_count;
} mlt_tile;

static uint32_t serve_recorded_tile(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  const mlt_tile* tile = user_data;
  (void)request;
  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = tile->bytes,
    .byte_count = tile->byte_count,
  };
  mln_resource_request_complete(handle, &response, NULL);
  mln_resource_request_release(handle);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

// Set from MapLibre's logging threads, so it lives in static storage that
// outlives any registration a failing case leaves behind.
static atomic_bool mlt_parse_warning_logged;

static uint32_t record_mlt_parse_warning(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  (void)user_data;
  (void)code;
  if (
    severity == MLN_LOG_SEVERITY_WARNING && event == MLN_LOG_EVENT_PARSE_TILE &&
    message != NULL && strstr(message, "MLT parse failed") != NULL
  ) {
    mln_test_flag_set(&mlt_parse_warning_logged);
  }
  return 0;
}

// One static map that loads one recorded tile through a provider.
typedef struct mlt_map {
  mlt_tile tile;
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture;
} mlt_map;

// Loads the tile at fixture_relative_path into a static map created with the
// given FastPFOR setting, and renders a still image, which completes once the
// tile has loaded and rendered.
static void render_recorded_tile(
  mlt_map* out, const char* fixture_relative_path, bool fast_pfor_enabled
) {
  out->tile.bytes =
    mln_test_read_fixture(fixture_relative_path, &out->tile.byte_count);
  TEST_ASSERT_NOT_NULL_MESSAGE(
    out->tile.bytes, "MLT fixture missing; check tests/native/fixtures"
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, out->tile.byte_count);

  out->runtime = mln_test_create_runtime();
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = serve_recorded_tile,
    .user_data = &out->tile,
  };
  MLN_TEST_AWAIT_OK(mln_runtime_set_resource_provider(
    out->runtime, &provider, &completion.descriptor, NULL
  ));

  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  options.fast_pfor_enabled = fast_pfor_enabled;
  out->map = mln_test_create_map_with_options(out->runtime, &options);
  mln_test_load_style_and_wait(
    out->runtime, out->map, MLN_BUFFER_LITERAL(mlt_style_json)
  );
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(out->map, &out->fixture));
  MLN_TEST_OK(mln_test_render_still_image(&out->fixture, out->map));
}

static void destroy_mlt_map(mlt_map* map) {
  mln_test_render_fixture_destroy(&map->fixture);
  mln_test_destroy_map(map->map);
  mln_test_destroy_runtime(map->runtime);
  free(map->tile.bytes);
}

static size_t count_admin_features(const mlt_map* map) {
  const mln_buffer_view source_layers[] = {MLN_BUFFER_LITERAL("admin")};
  mln_source_feature_query_options options =
    mln_source_feature_query_options_default();
  options.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  options.source_layer_ids = source_layers;
  options.source_layer_id_count = 1;
  mln_test_completion query = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_query_source_features(
    map->fixture.session, MLN_BUFFER_LITERAL("mlt-source"), &options,
    &query.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&map->fixture, &query));
  const size_t count = mln_test_completion_value_count(&query);
  mln_test_completion_destroy(&query);
  return count;
}

static void a_fast_pfor_tile_decodes_when_the_option_is_on(void) {
  mlt_map map = {0};
  render_recorded_tile(&map, "map/issue12432/0-0-0-fastpfor.mlt", true);
  TEST_ASSERT_GREATER_THAN_size_t(0, count_admin_features(&map));
  destroy_mlt_map(&map);
}

static void a_plain_tile_decodes_when_the_option_is_off(void) {
  mlt_map map = {0};
  render_recorded_tile(&map, "map/issue12432/0-0-0.mlt", false);
  TEST_ASSERT_GREATER_THAN_size_t(0, count_admin_features(&map));
  destroy_mlt_map(&map);
}

static void a_fast_pfor_tile_logs_a_parse_warning_when_the_option_is_off(void) {
  atomic_store(&mlt_parse_warning_logged, false);
  MLN_TEST_OK(mln_log_set_callback(record_mlt_parse_warning, NULL, NULL, NULL));
  mlt_map map = {0};
  render_recorded_tile(&map, "map/issue12432/0-0-0-fastpfor.mlt", false);
  // MapLibre may deliver a warning from its logging thread after the tile
  // has rendered, so the case waits for the record rather than checking it.
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&mlt_parse_warning_logged),
    "the map logged no MLT parse warning"
  );
  destroy_mlt_map(&map);
  MLN_TEST_OK(mln_log_clear_callback(NULL));
}

MLN_TEST_GROUP {
  RUN_TEST(a_fast_pfor_tile_decodes_when_the_option_is_on);
  RUN_TEST(a_plain_tile_decodes_when_the_option_is_off);
  RUN_TEST(a_fast_pfor_tile_logs_a_parse_warning_when_the_option_is_off);
}
