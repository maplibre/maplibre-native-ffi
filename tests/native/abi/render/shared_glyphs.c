// The maps of one runtime share the glyphs their renderers parse.

#include "support/frames.h"
#include "support/resources.h"
#include "support/test_support.h"

enum {
  glyph_side = 4,
  // Glyph bitmaps carry a 3-pixel border on each side.
  glyph_bitmap_bytes = (glyph_side + 6) * (glyph_side + 6),
  glyph_bytes = 2 + 2 + glyph_bitmap_bytes + 5 * 2,
  font_stack_bytes = 2 + 8 + 2 + 5 + 2 + glyph_bytes,
  glyph_pbf_bytes = 3 + font_stack_bytes,
  pixel_bytes = 64 * 64 * 4,
};

static const char text_style_json[] =
  "{\"version\":8,\"glyphs\":\"test://fonts/{fontstack}/{range}.pbf\","
  "\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ffffff\"}},"
  "{\"id\":\"label\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"text-field\":\"A\",\"text-font\":[\"TestFont\"],"
  "\"text-size\":24}}]}";

// One glyph range holding a solid square for "A", encoded as the glyph PBF
// schema lays it out: a font stack named TestFont for range 0-255.
static uint8_t glyph_pbf[glyph_pbf_bytes];

static void encode_glyph_pbf(void) {
  static const uint8_t font_stack_head[] = {
    0x0a, 0x85, 0x01, 0x0a, 0x08, 'T', 'e', 's',  't',  'F',  'o',  'n',  't',
    0x12, 0x05, '0',  '-',  '2',  '5', '5', 0x1a, 0x72, 0x08, 0x41, 0x12, 0x64,
  };
  // Width, height, left, top -4 (zigzag 7), and advance.
  static const uint8_t metrics[] = {
    0x18, glyph_side, 0x20, glyph_side, 0x28, 0x00, 0x30, 0x07, 0x38, 0x06,
  };
  TEST_ASSERT_EQUAL_size_t(
    glyph_pbf_bytes,
    sizeof(font_stack_head) + glyph_bitmap_bytes + sizeof(metrics)
  );
  uint8_t* cursor = glyph_pbf;
  memcpy(cursor, font_stack_head, sizeof(font_stack_head));
  cursor += sizeof(font_stack_head);
  memset(cursor, 0xff, glyph_bitmap_bytes);
  cursor += glyph_bitmap_bytes;
  memcpy(cursor, metrics, sizeof(metrics));
}

static mln_test_provider* install_glyph_provider(mln_runtime runtime) {
  static mln_test_provided_resource resources[1];
  resources[0] = (mln_test_provided_resource){
    .url = "test://fonts/TestFont/0-255.pbf",
    .response = {
      .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
      .bytes = glyph_pbf,
      .byte_count = sizeof(glyph_pbf),
    },
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_test_provider_install(runtime, provider);
  return provider;
}

static void render_text(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture,
  uint8_t* pixels
) {
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(text_style_json))
  );
  mln_test_render_until_idle(runtime, map, fixture);
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_size_t(pixel_bytes, info.byte_length);
}

static bool all_white(const uint8_t* pixels) {
  for (size_t index = 0; index < pixel_bytes; index += 1) {
    if (pixels[index] != 0xff) return false;
  }
  return true;
}

// A map renders the label alone in its runtime, then as one of two maps whose
// renderers share its glyphs, and still after the other map is gone.
static void maps_of_one_runtime_render_the_glyphs_they_share(void) {
  static uint8_t expected[pixel_bytes];
  static uint8_t actual[pixel_bytes];
  encode_glyph_pbf();

  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider* provider = install_glyph_provider(runtime);
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  render_text(runtime, map, &fixture, expected);
  TEST_ASSERT_FALSE_MESSAGE(all_white(expected), "the label did not render");
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);

  runtime = mln_test_create_runtime();
  provider = install_glyph_provider(runtime);
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_test_render_fixture first = {0};
  mln_test_render_fixture second = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(first_map, &first));
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(second_map, &second));
  render_text(runtime, first_map, &first, actual);
  TEST_ASSERT_EQUAL_MEMORY(expected, actual, pixel_bytes);
  render_text(runtime, second_map, &second, actual);
  TEST_ASSERT_EQUAL_MEMORY(expected, actual, pixel_bytes);

  mln_test_render_fixture_destroy(&first);
  mln_test_destroy_map(first_map);
  mln_test_render_request_forced(&second, 1);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&second));
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(&second, &info, actual, pixel_bytes));
  TEST_ASSERT_EQUAL_MEMORY(expected, actual, pixel_bytes);

  mln_test_render_fixture_destroy(&second);
  mln_test_destroy_map(second_map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

MLN_TEST_GROUP { RUN_TEST(maps_of_one_runtime_render_the_glyphs_they_share); }
