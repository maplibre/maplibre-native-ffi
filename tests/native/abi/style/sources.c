// Style sources: URL, attribution, inline tile URLs, source listing,
// volatility, removal, and image sources and their corners.
//
// URL sources start loading as soon as they are added, so every case that adds
// one serves the suite's resources through a provider that fails the rest.

#include "support/style.h"
#include "support/test_support.h"

// A deep copy of one source's metadata, whose views die with the callback.
// Strings longer than their buffers are truncated, which the cases never reach.
typedef struct source_copy {
  bool found;
  mln_style_source_info info;
  char id[32];
  char attribution[64];
  char url[64];
  char tile_urls[2][64];
} source_copy;

#define SOURCE_PROBE_CAPACITY 4

// Copies of the sources one get or list query delivered.
typedef struct source_probe {
  atomic_bool done;
  mln_status status;
  size_t count;
  source_copy sources[SOURCE_PROBE_CAPACITY];
} source_probe;

static void copy_text(mln_buffer_view view, char* out, size_t capacity) {
  snprintf(out, capacity, "%.*s", (int)view.size, (const char*)view.data);
}

static void copy_sources(void* user_data, const mln_completion_result* result) {
  source_probe* probe = user_data;
  probe->status = result->status;
  probe->count = result->value_count;
  const mln_style_source_info* infos = result->value;
  for (size_t index = 0;
       index < result->value_count && index < SOURCE_PROBE_CAPACITY;
       index += 1) {
    const mln_style_source_info* info = &infos[index];
    source_copy* copy = &probe->sources[index];
    copy->found = true;
    copy->info = *info;
    copy_text(info->id, copy->id, sizeof(copy->id));
    copy_text(info->attribution, copy->attribution, sizeof(copy->attribution));
    copy_text(info->url, copy->url, sizeof(copy->url));
    for (size_t url = 0; url < info->tilejson.tile_url_count && url < 2;
         url += 1) {
      copy_text(
        info->tilejson.tile_urls[url], copy->tile_urls[url],
        sizeof(copy->tile_urls[url])
      );
    }
  }
  mln_test_flag_set(&probe->done);
}

static mln_completion source_completion(source_probe* probe) {
  atomic_init(&probe->done, false);
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = copy_sources,
    .user_data = probe,
  };
}

static void finish_source_probe(const source_probe* probe) {
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe->done), "the source query never completed"
  );
  MLN_TEST_OK(probe->status);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(SOURCE_PROBE_CAPACITY, probe->count);
}

// Reads a source's metadata. `found` reports whether the source exists.
static source_copy read_source(mln_map map, const char* id) {
  source_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  const mln_completion completion = source_completion(&probe);
  MLN_TEST_OK(mln_map_get_style_source(
    map, mln_buffer_view_from_cstr(id), &completion, NULL
  ));
  finish_source_probe(&probe);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(1, probe.count);
  return probe.sources[0];
}

// Lists every source's metadata into `probe`, which starts zeroed.
static void list_sources(mln_map map, source_probe* probe) {
  const mln_completion completion = source_completion(probe);
  MLN_TEST_OK(mln_map_list_style_sources(map, &completion, NULL));
  finish_source_probe(probe);
}

static bool source_exists(mln_map map, const char* id) {
  return read_source(map, id).found;
}

static void add_source_json(mln_map map, const char* id, const char* json) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr(id), mln_buffer_view_from_cstr(json),
    &completion.descriptor, NULL
  ));
}

// A URL source reports the URL it was added with, or last set to. An inline
// tile source reports its attribution and tile URLs. A source reports only the
// members it has. A missing source completes with no value.
static void a_source_reports_its_url_attribution_and_tilejson(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);

  add_source_json(
    map, "remote", "{\"type\":\"raster\",\"url\":\"fixture://tiles.json\"}"
  );
  add_source_json(
    map, "inline",
    "{\"type\":\"vector\",\"attribution\":\"Fixture "
    "tiles\",\"tiles\":[\"fixture://a/{z}/{x}/{y}.mvt\",\"fixture://b/{z}/{x}/"
    "{y}.mvt\"]}"
  );
  add_source_json(
    map, "unattributed",
    "{\"type\":\"vector\",\"tiles\":[\"fixture://a/{z}/{x}/{y}.mvt\"]}"
  );
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL("fixture://first.geojson"), NULL, &completion.descriptor,
    NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_geojson_source_url(
    map, MLN_BUFFER_LITERAL("geojson"),
    MLN_BUFFER_LITERAL("fixture://second.geojson"), &completion.descriptor, NULL
  ));

  source_copy probe = read_source(map, "remote");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_SOURCE_INFO_URL,
    probe.info.fields &
      (MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_ATTRIBUTION |
       MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("fixture://tiles.json", probe.url);

  probe = read_source(map, "geojson");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_STRING("fixture://second.geojson", probe.url);

  probe = read_source(map, "inline");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON,
    probe.info.fields &
      (MLN_STYLE_SOURCE_INFO_URL | MLN_STYLE_SOURCE_INFO_ATTRIBUTION |
       MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("Fixture tiles", probe.attribution);
  TEST_ASSERT_EQUAL_size_t(2, probe.info.tilejson.tile_url_count);
  TEST_ASSERT_EQUAL_STRING("fixture://a/{z}/{x}/{y}.mvt", probe.tile_urls[0]);
  TEST_ASSERT_EQUAL_STRING("fixture://b/{z}/{x}/{y}.mvt", probe.tile_urls[1]);

  probe = read_source(map, "unattributed");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_TRUE(probe.info.fields & MLN_STYLE_SOURCE_INFO_ATTRIBUTION);
  TEST_ASSERT_EQUAL_STRING("", probe.attribution);

  TEST_ASSERT_FALSE(read_source(map, "missing").found);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void assert_same_source(
  const source_copy* expected, const source_copy* actual
) {
  const mln_style_source_info* want = &expected->info;
  const mln_style_source_info* got = &actual->info;
  TEST_ASSERT_EQUAL_STRING(expected->id, actual->id);
  TEST_ASSERT_EQUAL_UINT32(want->type, got->type);
  TEST_ASSERT_EQUAL_HEX32(want->fields, got->fields);
  TEST_ASSERT_EQUAL_INT(want->is_volatile, got->is_volatile);
  TEST_ASSERT_EQUAL_STRING(expected->attribution, actual->attribution);
  TEST_ASSERT_EQUAL_STRING(expected->url, actual->url);
  TEST_ASSERT_EQUAL_size_t(
    want->tilejson.tile_url_count, got->tilejson.tile_url_count
  );
  TEST_ASSERT_EQUAL_STRING(expected->tile_urls[0], actual->tile_urls[0]);
  TEST_ASSERT_EQUAL_DOUBLE(want->tilejson.min_zoom, got->tilejson.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(want->tilejson.max_zoom, got->tilejson.max_zoom);
  TEST_ASSERT_EQUAL_UINT32(want->tilejson.scheme, got->tilejson.scheme);
  TEST_ASSERT_EQUAL_UINT32(want->tile_size, got->tile_size);
  TEST_ASSERT_EQUAL_UINT32(want->vector_encoding, got->vector_encoding);
}

// Sources list in style order with the same metadata a get reports: the
// style's own sources as the document declares them, then added ones, less
// removed ones.
static void sources_list_in_style_order_with_their_info(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);
  mln_test_load_style_and_wait(
    map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{\"first\":" MLN_TEST_EMPTY_GEOJSON_SOURCE
      ",\"second\":" MLN_TEST_EMPTY_GEOJSON_SOURCE "},\"layers\":[]}"
    )
  );
  add_source_json(
    map, "third",
    "{\"type\":\"vector\",\"attribution\":\"Third "
    "tiles\",\"tiles\":[\"fixture://tiles/{z}/{x}/{y}\"]}"
  );
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("first"), &completion.descriptor, NULL
  ));

  source_probe list = {.status = MLN_STATUS_INVALID_STATE};
  list_sources(map, &list);
  TEST_ASSERT_EQUAL_size_t(2, list.count);
  const source_copy* second = &list.sources[0];
  const source_copy* third = &list.sources[1];
  TEST_ASSERT_EQUAL_STRING("second", second->id);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_GEOJSON, second->info.type);
  TEST_ASSERT_EQUAL_HEX32(
    0, second->info.fields &
         (MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("third", third->id);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_VECTOR, third->info.type);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON,
    third->info.fields &
      (MLN_STYLE_SOURCE_INFO_ATTRIBUTION | MLN_STYLE_SOURCE_INFO_TILEJSON)
  );
  TEST_ASSERT_EQUAL_STRING("Third tiles", third->attribution);
  TEST_ASSERT_EQUAL_size_t(1, third->info.tilejson.tile_url_count);
  TEST_ASSERT_EQUAL_STRING("fixture://tiles/{z}/{x}/{y}", third->tile_urls[0]);

  const source_copy got = read_source(map, "third");
  TEST_ASSERT_TRUE(got.found);
  assert_same_source(&got, third);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static bool source_is_volatile(mln_map map) {
  const source_copy probe = read_source(map, "volatile-vector");
  TEST_ASSERT_TRUE(probe.found);
  return probe.info.is_volatile;
}

static void style_source_volatility_round_trips(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view source_id = MLN_BUFFER_LITERAL("volatile-vector");
  add_source_json(
    map, "volatile-vector",
    "{\"type\":\"vector\",\"tiles\":[\"fixture://tiles/{z}/{x}/{y}\"]}"
  );
  TEST_ASSERT_FALSE(source_is_volatile(map));

  MLN_TEST_AWAIT_OK(mln_map_set_style_source_volatile(
    map, source_id, true, &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(source_is_volatile(map));

  MLN_TEST_AWAIT_OK(mln_map_set_style_source_volatile(
    map, source_id, false, &completion.descriptor, NULL
  ));
  TEST_ASSERT_FALSE(source_is_volatile(map));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void an_in_use_source_removal_fails_and_leaves_the_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view in_use = MLN_BUFFER_LITERAL("in-use");
  add_source_json(map, "in-use", MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"user\",\"type\":\"circle\",\"source\":\"in-use\"}"
    ),
    MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
  ));

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "used by a layer",
    mln_map_remove_style_source(map, in_use, &completion.descriptor, NULL)
  );
  TEST_ASSERT_TRUE(source_exists(map, "in-use"));

  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("user"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(
    mln_map_remove_style_source(map, in_use, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(source_exists(map, "in-use"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads an image source's four corners into `out`.
static void read_corners(mln_map map, const char* id, mln_lat_lng out[4]) {
  mln_test_completion completion =
    mln_test_completion_default(4 * sizeof(mln_lat_lng));
  MLN_TEST_OK(mln_map_get_image_source_coordinates(
    map, mln_buffer_view_from_cstr(id), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(4, mln_test_completion_value_count(&completion));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, out, 4 * sizeof(*out))
  );
  mln_test_completion_destroy(&completion);
}

static const mln_lat_lng corners[4] = {{1, 2}, {1, 3}, {0, 3}, {0, 2}};

// Image sources hold four corner coordinates and an image, from a URL or from
// inline pixels, and the typed updates reject every other source kind.
static void image_sources_hold_corners_and_pixels(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);
  const uint8_t pixels[16] = {
    255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
  };
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);
  const mln_buffer_view inline_image = MLN_BUFFER_LITERAL("inline-image");
  const mln_buffer_view remote_image = MLN_BUFFER_LITERAL("remote-image");
  const mln_buffer_view geojson = MLN_BUFFER_LITERAL("geojson");
  const mln_buffer_view png = MLN_BUFFER_LITERAL("fixture://image.png");

  MLN_TEST_AWAIT_OK(mln_map_add_image_source_image(
    map, inline_image, corners, 4, &image, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_url(
    map, remote_image, corners, 4, png, &completion.descriptor, NULL
  ));
  const source_copy probe = read_source(map, "inline-image");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_IMAGE, probe.info.type);

  mln_lat_lng read[4];
  read_corners(map, "remote-image", read);
  TEST_ASSERT_EQUAL_MEMORY(corners, read, sizeof(corners));

  const mln_lat_lng moved[4] = {{5, 6}, {5, 7}, {4, 7}, {4, 6}};
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_coordinates(
    map, inline_image, moved, 4, &completion.descriptor, NULL
  ));
  read_corners(map, "inline-image", read);
  TEST_ASSERT_EQUAL_MEMORY(moved, read, sizeof(moved));
  // A URL source takes inline pixels, and an inline one takes a URL.
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_image(
    map, remote_image, &image, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_image_source_url(
    map, inline_image, png, &completion.descriptor, NULL
  ));

  // Coordinates come in fours, and pixels must cover the image, before the
  // call returns.
  mln_completion discard = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_set_image_source_coordinates(
    map, inline_image, moved, 3, &discard, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "must be 4", mln_map_add_image_source_url(
                   map, MLN_BUFFER_LITERAL("one-corner"), moved, 1, png,
                   &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  mln_premultiplied_rgba8_image short_image = image;
  short_image.byte_length = 15;
  MLN_TEST_INVALID(mln_map_set_image_source_image(
    map, inline_image, &short_image, &discard, NULL
  ));

  add_source_json(map, "geojson", MLN_TEST_EMPTY_GEOJSON_SOURCE);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_coordinates(
      map, geojson, moved, 4, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_image(
      map, geojson, &image, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "source does not exist",
    mln_map_set_image_source_image(
      map, MLN_BUFFER_LITERAL("missing"), &image, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_image_source_coordinates(
                                   map, geojson, &completion.descriptor, NULL
                                 )
  );
  // Corners are a member of the source, so reading a missing one fails.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_NOT_FOUND,
    mln_map_get_image_source_coordinates(
      map, MLN_BUFFER_LITERAL("missing"), &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_source_reports_its_url_attribution_and_tilejson);
  RUN_TEST(sources_list_in_style_order_with_their_info);
  RUN_TEST(style_source_volatility_round_trips);
  RUN_TEST(an_in_use_source_removal_fails_and_leaves_the_source);
  RUN_TEST(image_sources_hold_corners_and_pixels);
}
