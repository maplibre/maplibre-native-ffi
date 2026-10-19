#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

typedef struct image_layout_events {
  size_t referenced_parses;
  size_t unrelated_parses;
} image_layout_events;

static bool count_parse(
  const mln_runtime_event* event, const char* messages, void* context
) {
  image_layout_events* counts = context;
  if (
    event->type == MLN_RUNTIME_EVENT_MAP_TILE_ACTION &&
    event->payload.tile_action.operation == MLN_TILE_OPERATION_START_PARSE
  ) {
    const char* source = messages + event->message_offset;
    if (event->message_size == 4 && memcmp(source, "late", 4) == 0) {
      ++counts->referenced_parses;
    } else if (event->message_size == 5 && memcmp(source, "other", 5) == 0) {
      ++counts->unrelated_parses;
    }
  }
  return false;
}

static image_layout_events render_still(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
) {
  MLN_TEST_OK(mln_test_render_still_image(fixture, map));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  image_layout_events counts = {0};
  (void)mln_test_drain_counting_matching(runtime, count_parse, &counts);
  return counts;
}

static void add_red_image(mln_map map, mln_buffer_view id) {
  uint8_t pixels[32 * 32 * 4];
  for (size_t index = 0; index < sizeof(pixels); index += 4) {
    pixels[index] = 255;
    pixels[index + 1] = 0;
    pixels[index + 2] = 0;
    pixels[index + 3] = 255;
  }
  const mln_premultiplied_rgba8_image image = {
    .size = sizeof(image),
    .width = 32,
    .height = 32,
    .stride = 32 * 4,
    .pixels = pixels,
    .byte_length = sizeof(pixels),
  };
  MLN_TEST_AWAIT_OK(
    mln_map_set_style_image(map, id, &image, NULL, &completion.descriptor, NULL)
  );
}

static void expect_center(
  const mln_test_render_fixture* fixture, uint8_t green
) {
  uint8_t pixels[64 * 64 * 4];
  mln_texture_image_info info = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_render_read_back(fixture, &info, pixels, sizeof(pixels))
  );
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  const uint8_t* center = pixels + 32 * info.stride + 32 * 4;
  TEST_ASSERT_EQUAL_UINT8(255, center[0]);
  TEST_ASSERT_EQUAL_UINT8(green, center[1]);
  TEST_ASSERT_EQUAL_UINT8(green, center[2]);
  TEST_ASSERT_EQUAL_UINT8(255, center[3]);
}

static void late_image_relayouts_its_source(bool synchronous, bool pattern) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"version\":8,\"sources\":{\"other\":{\"type\":\"geojson\","
        "\"data\":{\"type\":\"Point\",\"coordinates\":[100,0]}}},\"layers\":["
        "{\"id\":\"bg\",\"type\":\"background\",\"paint\":{\"background-"
        "color\":\"white\"}},"
        "{\"id\":\"circle\",\"type\":\"circle\",\"source\":\"other\"}]}"
      )
    )
  );
  mln_geojson_source_options source_options =
    mln_geojson_source_options_default();
  source_options.fields = MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
  source_options.synchronous_tiling = synchronous;
  const mln_buffer_view point = MLN_BUFFER_LITERAL(
    "{\"type\":\"Feature\",\"properties\":{\"icon\":\"marker\"},"
    "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]}}"
  );
  const mln_buffer_view polygon = MLN_BUFFER_LITERAL(
    "{\"type\":\"Polygon\",\"coordinates\":[[[-20,-20],[20,-20],"
    "[20,20],[-20,20],[-20,-20]]]}"
  );
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     pattern ? polygon : point, &source_options, &data, NULL
                   )
  );
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("late"), data, &completion.descriptor, NULL
  ));
  mln_geojson_source_data_release(data);
  const mln_buffer_view symbol = MLN_BUFFER_LITERAL(
    "{\"id\":\"late\",\"type\":\"symbol\",\"source\":\"late\",\"layout\":"
    "{\"icon-image\":[\"get\",\"icon\"],\"icon-allow-overlap\":true,\"icon-"
    "ignore-placement\":true}}"
  );
  const mln_buffer_view fill = MLN_BUFFER_LITERAL(
    "{\"id\":\"late\",\"type\":\"fill\",\"source\":\"late\","
    "\"paint\":{\"fill-pattern\":\"marker\"}}"
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, pattern ? fill : symbol, MLN_BUFFER_LITERAL(""),
    &completion.descriptor, NULL
  ));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  image_layout_events counts = render_still(runtime, map, &fixture);
  TEST_ASSERT_GREATER_THAN_size_t(0, counts.referenced_parses);
  TEST_ASSERT_GREATER_THAN_size_t(0, counts.unrelated_parses);
  expect_center(&fixture, 255);

  add_red_image(map, MLN_BUFFER_LITERAL("unrelated"));
  counts = render_still(runtime, map, &fixture);
  TEST_ASSERT_EQUAL_size_t(0, counts.referenced_parses);
  TEST_ASSERT_EQUAL_size_t(0, counts.unrelated_parses);
  expect_center(&fixture, 255);

  add_red_image(map, MLN_BUFFER_LITERAL("marker"));
  counts = render_still(runtime, map, &fixture);
  TEST_ASSERT_EQUAL_size_t(1, counts.referenced_parses);
  TEST_ASSERT_EQUAL_size_t(0, counts.unrelated_parses);
  expect_center(&fixture, 0);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void asynchronous_late_icon(void) {
  late_image_relayouts_its_source(false, false);
}

static void synchronous_late_icon(void) {
  late_image_relayouts_its_source(true, false);
}

static void asynchronous_late_pattern(void) {
  late_image_relayouts_its_source(false, true);
}

static void synchronous_late_pattern(void) {
  late_image_relayouts_its_source(true, true);
}

MLN_TEST_GROUP {
  RUN_TEST(asynchronous_late_icon);
  RUN_TEST(synchronous_late_icon);
  RUN_TEST(asynchronous_late_pattern);
  RUN_TEST(synchronous_late_pattern);
}
