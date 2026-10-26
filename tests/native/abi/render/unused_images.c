// Style images that a host supplies for a missing-image event. MapLibre drops
// one once no loaded tile uses it, unless the host set it with keep_unused.

#include "support/frames.h"
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

typedef struct image_probe {
  atomic_bool done;
  mln_status status;
  bool found;
  bool keep_unused;
} image_probe;

static void record_image(void* user_data, const mln_completion_result* result) {
  image_probe* probe = user_data;
  probe->status = result->status;
  probe->found = result->value_count == 1;
  if (probe->found) {
    probe->keep_unused =
      ((const mln_style_image_info*)result->value)->keep_unused;
  }
  mln_test_flag_set(&probe->done);
}

static image_probe read_image(mln_map map, const char* id) {
  image_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = record_image,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_map_get_style_image(
    map, mln_buffer_view_from_cstr(id), &completion, NULL
  ));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  MLN_TEST_OK(probe.status);
  return probe;
}

// The renderer hands the unused images to the map's worker before the
// reduction completes, so the fence after it orders the removals before any
// later query.
static void reduce_memory_use(
  mln_map map, const mln_test_render_fixture* fixture
) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_reduce_memory_use(
      fixture->session, &completion.descriptor, NULL
    )
  );
  MLN_TEST_OK(mln_test_map_fence(map));
}

// Each reduction asks again about every unused image, so a kept image stays
// through repeated reductions and through a style load that fails, and goes
// once a replacement leaves keep_unused unset.
static void only_images_set_with_keep_unused_survive_when_unused(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL(style_json)));
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);

  supply(map, "kept-icon", true);
  supply(map, "dropped-icon", false);
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);
  TEST_ASSERT_TRUE(read_image(map, "kept-icon").keep_unused);
  TEST_ASSERT_FALSE(read_image(map, "dropped-icon").keep_unused);

  // While a tile uses them, a reduction drops neither.
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_TRUE(read_image(map, "kept-icon").found);
  TEST_ASSERT_TRUE(read_image(map, "dropped-icon").found);

  // Pointing both icons at another image lays the tile out again without them.
  supply(map, "other-icon", false);
  set_icon_image(map, "kept", "other-icon");
  set_icon_image(map, "dropped", "other-icon");
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_TRUE(read_image(map, "kept-icon").found);
  TEST_ASSERT_FALSE(read_image(map, "dropped-icon").found);
  TEST_ASSERT_TRUE(read_image(map, "other-icon").found);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NATIVE_ERROR,
    mln_map_set_style_json(
      map, MLN_BUFFER_LITERAL("{\"version\":8"), &completion.descriptor, NULL
    )
  );
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_TRUE(read_image(map, "kept-icon").found);

  supply(map, "kept-icon", false);
  (void)mln_test_render_until_idle(runtime, map, &fixture, NULL);
  reduce_memory_use(map, &fixture);
  TEST_ASSERT_FALSE(read_image(map, "kept-icon").found);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(only_images_set_with_keep_unused_survive_when_unused);
}
