// Style layers: the layer result, properties and filters through JSON, layer
// JSON, listing in style order, source bindings, and the source a layer type
// requires.

#include <math.h>

#include "support/style.h"
#include "support/test_support.h"

static void add_source(mln_map map, const char* id) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr(id),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
}

static void add_layer(mln_map map, const char* json, const char* before) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(json), mln_buffer_view_from_cstr(before),
    &completion.descriptor, NULL
  ));
}

static void add_dem_source(mln_map map, const char* id) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr(id),
    MLN_BUFFER_LITERAL(
      "{\"type\":\"raster-dem\",\"tiles\":[\"fixture://dem/{z}/{x}/{y}.png\"]}"
    ),
    &completion.descriptor, NULL
  ));
}

// Deep-copies one layer result, whose views die with the callback.
typedef struct layer_probe {
  atomic_bool done;
  mln_status status;
  bool found;
  mln_style_layer_info info;
  char id[64];
  char type[32];
  char source_id[64];
  size_t source_id_size;
  char source_layer[64];
  size_t source_layer_size;
} layer_probe;

static void copy_view_into(
  mln_buffer_view view, char* out, size_t capacity, size_t* out_size
) {
  TEST_ASSERT_LESS_THAN_size_t(capacity, view.size);
  if (out_size != NULL) *out_size = view.size;
  if (view.size != 0) memcpy(out, view.data, view.size);
  out[view.size] = '\0';
}

static void copy_layer_result(
  void* user_data, const mln_completion_result* result
) {
  layer_probe* probe = user_data;
  probe->status = result->status;
  probe->found = result->value_count == 1;
  if (probe->found) {
    const mln_style_layer_info* layer = result->value;
    probe->info = *layer;
    copy_view_into(layer->id, probe->id, sizeof(probe->id), NULL);
    copy_view_into(layer->type, probe->type, sizeof(probe->type), NULL);
    copy_view_into(
      layer->source_id, probe->source_id, sizeof(probe->source_id),
      &probe->source_id_size
    );
    copy_view_into(
      layer->source_layer, probe->source_layer, sizeof(probe->source_layer),
      &probe->source_layer_size
    );
  }
  mln_test_flag_set(&probe->done);
}

static layer_probe take_layer_result(
  mln_runtime runtime, mln_map map, const char* id
) {
  layer_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_layer_result,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_map_get_style_layer(
    map, mln_buffer_view_from_cstr(id), &completion, NULL
  ));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  MLN_TEST_OK(probe.status);
  return probe;
}

static void layer_result_reports_scalars_and_carries_the_source_ids(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_FALSE(take_layer_result(runtime, map, "missing-layer").found);

  add_source(map, "info-source");
  add_layer(
    map,
    "{\"id\":\"info-layer\",\"type\":\"circle\",\"source\":\"info-source\"}", ""
  );
  layer_probe probe = take_layer_result(runtime, map, "info-layer");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_STRING("info-layer", probe.id);
  TEST_ASSERT_EQUAL_STRING("circle", probe.type);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_LAYER_VISIBILITY_VISIBLE, probe.info.visibility
  );
  TEST_ASSERT_EQUAL_STRING("info-source", probe.source_id);
  // A layer that sets no source-layer carries an empty view, not a missing one.
  TEST_ASSERT_EQUAL_size_t(0, probe.source_layer_size);
  // Unbounded zooms report the documented infinities before bounds are set.
  TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, probe.info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(INFINITY, probe.info.max_zoom);

  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("info-layer"), MLN_BUFFER_LITERAL("minzoom"),
    MLN_BUFFER_LITERAL("3.0"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("info-layer"), MLN_BUFFER_LITERAL("maxzoom"),
    MLN_BUFFER_LITERAL("12.0"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("info-layer"), MLN_BUFFER_LITERAL("visibility"),
    MLN_BUFFER_LITERAL("\"none\""), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("info-layer"), MLN_BUFFER_LITERAL("source-layer"),
    MLN_BUFFER_LITERAL("\"roads\""), &completion.descriptor, NULL
  ));
  probe = take_layer_result(runtime, map, "info-layer");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_DOUBLE(3.0, probe.info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, probe.info.max_zoom);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_LAYER_VISIBILITY_NONE, probe.info.visibility
  );
  TEST_ASSERT_EQUAL_STRING("roads", probe.source_layer);

  // The list delivers the same record as the get.
  const mln_test_style_list layers = mln_test_style_list_layers(map);
  MLN_TEST_OK(layers.status);
  TEST_ASSERT_EQUAL_size_t(1, layers.count);
  const mln_test_style_entry* entry = &layers.entries[0];
  TEST_ASSERT_EQUAL_STRING(probe.id, entry->id);
  TEST_ASSERT_EQUAL_STRING(probe.type, entry->type);
  TEST_ASSERT_EQUAL_STRING(probe.source_id, entry->source_id);
  TEST_ASSERT_EQUAL_STRING(probe.source_layer, entry->source_layer);
  TEST_ASSERT_EQUAL_DOUBLE(probe.info.min_zoom, entry->min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(probe.info.max_zoom, entry->max_zoom);
  TEST_ASSERT_EQUAL_UINT32(probe.info.visibility, entry->visibility);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads one layer property, filter, or JSON copy, or "" when it is undefined.
static mln_status read_layer_property(
  mln_map map, const char* layer, const char* name, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_style_layer_property(
    map, mln_buffer_view_from_cstr(layer), mln_buffer_view_from_cstr(name),
    &completion.descriptor, NULL
  ));
  return mln_test_style_finish_text(&completion, out, capacity, NULL);
}

static mln_status read_layer_filter(
  mln_map map, const char* layer, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_style_layer_property(
    map, mln_buffer_view_from_cstr(layer), MLN_BUFFER_LITERAL("filter"),
    &completion.descriptor, NULL
  ));
  return mln_test_style_finish_text(&completion, out, capacity, NULL);
}

#define EXPECT_LAYER_PROPERTY(expected, map, layer, name)             \
  do {                                                                \
    char property_json[256];                                          \
    TEST_ASSERT_EQUAL_INT(                                            \
      MLN_STATUS_OK,                                                  \
      read_layer_property((map), (layer), (name), property_json, 256) \
    );                                                                \
    TEST_ASSERT_EQUAL_STRING((expected), property_json);              \
  } while (false)

// Properties and filters read back as the JSON MapLibre Native stores. A bad
// name or value on a live layer is an INVALID_ARGUMENT that leaves the layer
// as it was; the layer ID itself is what reports NOT_FOUND.
static void layer_properties_and_filters_round_trip_through_json(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_source(map, "points");
  add_layer(
    map, "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"points\"}", ""
  );

  EXPECT_LAYER_PROPERTY("", map, "dots", "circle-radius");
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-radius"),
    MLN_BUFFER_LITERAL("4"), &completion.descriptor, NULL
  ));
  EXPECT_LAYER_PROPERTY("4.0", map, "dots", "circle-radius");
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-color"),
    MLN_BUFFER_LITERAL("\"#ff0000\""), &completion.descriptor, NULL
  ));
  EXPECT_LAYER_PROPERTY(
    "[\"rgba\",255.0,0.0,0.0,1.0]", map, "dots", "circle-color"
  );

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "layer property",
    mln_map_set_style_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("fill-color"),
      MLN_BUFFER_LITERAL("\"#00ff00\""), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "layer property",
    mln_map_set_style_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-radius"),
      MLN_BUFFER_LITERAL("{\"radius\":"), &completion.descriptor, NULL
    )
  );
  EXPECT_LAYER_PROPERTY("4.0", map, "dots", "circle-radius");
  // A name the layer type lacks reads as undefined rather than failing.
  EXPECT_LAYER_PROPERTY("", map, "dots", "fill-color");

  char filter[256];
  MLN_TEST_OK(read_layer_filter(map, "dots", filter, sizeof(filter)));
  TEST_ASSERT_EQUAL_STRING("", filter);
  const mln_buffer_view kind_filter =
    MLN_BUFFER_LITERAL("[\"==\",[\"get\",\"kind\"],\"park\"]");
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("filter"), kind_filter,
    &completion.descriptor, NULL
  ));
  MLN_TEST_OK(read_layer_filter(map, "dots", filter, sizeof(filter)));
  TEST_ASSERT_EQUAL_STRING("[\"==\",[\"get\",\"kind\"],\"park\"]", filter);
  // A null filter clears it.
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("filter"),
    MLN_BUFFER_LITERAL("null"), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(read_layer_filter(map, "dots", filter, sizeof(filter)));
  TEST_ASSERT_EQUAL_STRING("", filter);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status read_layer_json(
  mln_map map, const char* layer, char* out, size_t capacity, bool* found
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_style_layer_json(
    map, mln_buffer_view_from_cstr(layer), &completion.descriptor, NULL
  ));
  return mln_test_style_finish_text(&completion, out, capacity, found);
}

static void root_properties_match_serialization_and_null_resets_zoom(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_source(map, "points");
  add_layer(
    map, "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"points\"}", ""
  );
  EXPECT_LAYER_PROPERTY("\"points\"", map, "dots", "source");
  EXPECT_LAYER_PROPERTY("", map, "dots", "source-layer");
  EXPECT_LAYER_PROPERTY("", map, "dots", "minzoom");
  EXPECT_LAYER_PROPERTY("", map, "dots", "maxzoom");

  static const char* zoom_names[] = {"minzoom", "maxzoom"};
  for (size_t index = 0; index < 2; index += 1) {
    MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
      map, MLN_BUFFER_LITERAL("dots"),
      mln_buffer_view_from_cstr(zoom_names[index]), MLN_BUFFER_LITERAL("4.5"),
      &completion.descriptor, NULL
    ));
    EXPECT_LAYER_PROPERTY("4.5", map, "dots", zoom_names[index]);
    MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
      map, MLN_BUFFER_LITERAL("dots"),
      mln_buffer_view_from_cstr(zoom_names[index]), MLN_BUFFER_LITERAL("null"),
      &completion.descriptor, NULL
    ));
    EXPECT_LAYER_PROPERTY("", map, "dots", zoom_names[index]);
  }
  const layer_probe probe = take_layer_result(runtime, map, "dots");
  TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, probe.info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(INFINITY, probe.info.max_zoom);
  char json[512];
  bool found = false;
  MLN_TEST_OK(read_layer_json(map, "dots", json, sizeof(json), &found));
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_NULL(strstr(json, "minzoom"));
  TEST_ASSERT_NULL(strstr(json, "maxzoom"));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Layer JSON serializes the whole layer as it stands after later edits, and
// adds back as the same layer.
static void layer_json_serializes_the_current_layer(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_layer(
    map,
    "{\"id\":\"paper\",\"type\":\"background\",\"paint\":{"
    "\"background-color\":\"#ff0000\"}}",
    ""
  );
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("paper"), MLN_BUFFER_LITERAL("minzoom"),
    MLN_BUFFER_LITERAL("2.0"), &completion.descriptor, NULL
  ));

  char json[512];
  bool found = false;
  MLN_TEST_OK(read_layer_json(map, "paper", json, sizeof(json), &found));
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_NOT_NULL(strstr(json, "\"id\":\"paper\""));
  TEST_ASSERT_NOT_NULL(strstr(json, "\"type\":\"background\""));
  TEST_ASSERT_NOT_NULL(
    strstr(json, "\"background-color\":[\"rgba\",255.0,0.0,0.0,1.0]")
  );
  TEST_ASSERT_NOT_NULL(strstr(json, "\"minzoom\":2.0"));

  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("paper"), &completion.descriptor, NULL
  ));
  add_layer(map, json, "");
  char again[512];
  MLN_TEST_OK(read_layer_json(map, "paper", again, sizeof(again), &found));
  TEST_ASSERT_EQUAL_STRING(json, again);

  MLN_TEST_OK(read_layer_json(map, "missing", json, sizeof(json), &found));
  TEST_ASSERT_FALSE(found);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void move_layer(mln_map map, const char* layer, const char* before) {
  MLN_TEST_AWAIT_OK(mln_map_move_style_layer(
    map, mln_buffer_view_from_cstr(layer), mln_buffer_view_from_cstr(before),
    &completion.descriptor, NULL
  ));
}

// Both listings report the layer stack bottom to top, as moves leave it.
static void layers_list_in_style_order_after_moves(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_source(map, "points");
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("tiles"),
    MLN_BUFFER_LITERAL(
      "{\"type\":\"vector\",\"tiles\":[\"fixture://dem/{z}/{x}/{y}.png\"]}"
    ),
    &completion.descriptor, NULL
  ));
  add_layer(map, "{\"id\":\"paper\",\"type\":\"background\"}", "");
  add_layer(
    map, "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"points\"}", ""
  );
  add_layer(
    map,
    "{\"id\":\"roads\",\"type\":\"line\",\"source\":\"tiles\","
    "\"source-layer\":\"transportation\"}",
    ""
  );
  // Insert the roads under everything, then lift the paper to the top.
  move_layer(map, "roads", "paper");
  move_layer(map, "paper", "");
  // Moving a layer before itself leaves the order alone.
  move_layer(map, "dots", "dots");

  const mln_test_style_list layers = mln_test_style_list_layers(map);
  MLN_TEST_OK(layers.status);
  TEST_ASSERT_EQUAL_size_t(3, layers.count);
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_style_layer_info), layers.value_size);
  TEST_ASSERT_EQUAL_STRING("roads", layers.entries[0].id);
  TEST_ASSERT_EQUAL_STRING("line", layers.entries[0].type);
  TEST_ASSERT_EQUAL_STRING("tiles", layers.entries[0].source_id);
  TEST_ASSERT_EQUAL_STRING("transportation", layers.entries[0].source_layer);
  TEST_ASSERT_EQUAL_STRING("dots", layers.entries[1].id);
  TEST_ASSERT_EQUAL_STRING("circle", layers.entries[1].type);
  TEST_ASSERT_EQUAL_STRING("points", layers.entries[1].source_id);
  TEST_ASSERT_EQUAL_STRING("", layers.entries[1].source_layer);
  // A layer type that takes no source reports empty source fields.
  TEST_ASSERT_EQUAL_STRING("paper", layers.entries[2].id);
  TEST_ASSERT_EQUAL_STRING("background", layers.entries[2].type);
  TEST_ASSERT_EQUAL_STRING("", layers.entries[2].source_id);
  TEST_ASSERT_EQUAL_STRING("", layers.entries[2].source_layer);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A layer's source and source layer change only on layer types that take a
// source. The source need not exist yet, and an empty source layer clears it.
static void source_bindings_change_only_on_layers_that_take_a_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_source(map, "points");
  add_layer(
    map, "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"points\"}", ""
  );
  add_layer(map, "{\"id\":\"paper\",\"type\":\"background\"}", "");

  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("source"),
    MLN_BUFFER_LITERAL("\"not-yet-added\""), &completion.descriptor, NULL
  ));
  layer_probe probe = take_layer_result(runtime, map, "dots");
  TEST_ASSERT_EQUAL_STRING("not-yet-added", probe.source_id);
  EXPECT_LAYER_PROPERTY("\"not-yet-added\"", map, "dots", "source");
  TEST_ASSERT_EQUAL_STRING("", probe.source_layer);
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("source-layer"),
    MLN_BUFFER_LITERAL("\"pois\""), &completion.descriptor, NULL
  ));
  probe = take_layer_result(runtime, map, "dots");
  TEST_ASSERT_EQUAL_STRING("pois", probe.source_layer);
  EXPECT_LAYER_PROPERTY("\"pois\"", map, "dots", "source-layer");
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("source-layer"),
    MLN_BUFFER_LITERAL("\"\""), &completion.descriptor, NULL
  ));
  probe = take_layer_result(runtime, map, "dots");
  TEST_ASSERT_EQUAL_STRING("", probe.source_layer);
  EXPECT_LAYER_PROPERTY("", map, "dots", "source-layer");

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "does not take a source",
    mln_map_set_style_layer_property(
      map, MLN_BUFFER_LITERAL("paper"), MLN_BUFFER_LITERAL("source"),
      MLN_BUFFER_LITERAL("\"points\""), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "does not take a source-layer",
    mln_map_set_style_layer_property(
      map, MLN_BUFFER_LITERAL("paper"), MLN_BUFFER_LITERAL("source-layer"),
      MLN_BUFFER_LITERAL("\"pois\""), &completion.descriptor, NULL
    )
  );
  probe = take_layer_result(runtime, map, "paper");
  TEST_ASSERT_EQUAL_STRING("", probe.source_id);
  EXPECT_LAYER_PROPERTY("", map, "paper", "source");
  EXPECT_LAYER_PROPERTY("", map, "paper", "source-layer");

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A layer whose type requires a source takes only a source that supports the
// type: a hillshade layer requires a raster DEM source, and a circle layer
// cannot draw one. A layer ID is taken once.
static void a_layer_requires_a_source_that_supports_its_type(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_dem_source(map, "dem");
  add_source(map, "points");

  const mln_buffer_view hillshade = MLN_BUFFER_LITERAL(
    "{\"id\":\"hillshade\",\"type\":\"hillshade\",\"source\":\"dem\"}"
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, hillshade, MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
  ));
  const layer_probe probe = take_layer_result(runtime, map, "hillshade");
  TEST_ASSERT_EQUAL_STRING("hillshade", probe.type);
  TEST_ASSERT_EQUAL_STRING("dem", probe.source_id);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "layer already exists",
    mln_map_add_style_layer_json(
      map, hillshade, MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
    )
  );

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source does not support layer type",
    mln_map_add_style_layer_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"id\":\"on-points\",\"type\":\"hillshade\",\"source\":"
        "\"points\"}"
      ),
      MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source does not support layer type",
    mln_map_add_style_layer_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"dem\"}"
      ),
      MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(layer_result_reports_scalars_and_carries_the_source_ids);
  RUN_TEST(layer_properties_and_filters_round_trip_through_json);
  RUN_TEST(root_properties_match_serialization_and_null_resets_zoom);
  RUN_TEST(layer_json_serializes_the_current_layer);
  RUN_TEST(layers_list_in_style_order_after_moves);
  RUN_TEST(source_bindings_change_only_on_layers_that_take_a_source);
  RUN_TEST(a_layer_requires_a_source_that_supports_its_type);
}
