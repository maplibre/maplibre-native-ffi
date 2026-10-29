// Style images that a host supplies for a missing-image event. MapLibre drops
// one once no loaded tile uses it, unless the host set it with keep_unused.

#include "support/map.h"
#include "support/test_support.h"

// A GeoJSON point under two icons whose images the style never provides, so
// the first render reports both missing.
static const char style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"kept\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"icon-image\":\"kept-icon\"}},"
  "{\"id\":\"dropped\",\"type\":\"symbol\",\"source\":\"points\","
  "\"layout\":{\"icon-image\":\"dropped-icon\"}}]}";

static void supply(mln_map map, const char* id, bool keep_unused) {
  static const uint8_t pixel[4] = {255, 255, 255, 255};
  mln_premultiplied_rgba8_image image = {
    .size = sizeof(mln_premultiplied_rgba8_image)
  };
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  // An image that the host does not keep leaves keep_unused absent.
  mln_style_image_options options = {.size = sizeof(mln_style_image_options)};
  if (keep_unused) {
    options.fields = MLN_STYLE_IMAGE_OPTION_KEEP_UNUSED;
    options.keep_unused = true;
  }
  MLN_TEST_AWAIT_OK(mln_map_set_style_image(
    map, mln_buffer_view_from_cstr(id), &image, &options,
    &completion.descriptor, NULL
  ));
}

static void set_icon_image(mln_map map, const char* layer, const char* id) {
  char value[64];
  (void)snprintf(value, sizeof(value), "\"%s\"", id);
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, mln_buffer_view_from_cstr(layer), MLN_BUFFER_LITERAL("icon-image"),
    mln_buffer_view_from_cstr(value), &completion.descriptor, NULL
  ));
}

static bool has_image(mln_map map, const char* id) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_get_style_image(
    map, mln_buffer_view_from_cstr(id), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);
  return found;
}

// Cleanup queues image removal to the map's renderer-observer mailbox. A map
// command cannot fence that mailbox. A new still image completes only after
// the map handles its frame, behind the removal in the same mailbox.
static void reduce_memory_use(
  mln_map map, const mln_test_render_fixture* fixture
) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_reduce_memory_use(
      fixture->session, &completion.descriptor, NULL
    )
  );
  MLN_TEST_OK(mln_test_render_still_image(fixture, map));
}

// An unused image stays only while its current options ask to keep it.
static void only_images_set_with_keep_unused_survive_when_unused(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(style_json)));
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  supply(map, "kept-icon", true);
  supply(map, "dropped-icon", false);
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  // While a tile uses them, a reduction drops neither.
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_TRUE(has_image(map, "kept-icon"));
  TEST_ASSERT_TRUE(has_image(map, "dropped-icon"));

  // Pointing both icons at another image lays the tile out again without them.
  supply(map, "other-icon", false);
  set_icon_image(map, "kept", "other-icon");
  set_icon_image(map, "dropped", "other-icon");
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_TRUE(has_image(map, "kept-icon"));
  TEST_ASSERT_FALSE(has_image(map, "dropped-icon"));
  TEST_ASSERT_TRUE(has_image(map, "other-icon"));

  supply(map, "kept-icon", false);
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_FALSE(has_image(map, "kept-icon"));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(only_images_set_with_keep_unused_survive_when_unused);
}
