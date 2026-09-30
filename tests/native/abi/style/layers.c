// Style layers: the layer result, properties and filters through JSON, layer
// JSON, listing in style order, source bindings, the typed terrain adders, and
// the location indicator setters.

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static void add_source(mln_map map, const char* id) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, mln_test_view_of(id),
                     MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE),
                     &completion.descriptor, NULL
                   )
  );
}

static void add_layer(mln_map map, const char* json, const char* before) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_layer_json(
                     map, mln_test_view_of(json), mln_test_view_of(before),
                     &completion.descriptor, NULL
                   )
  );
}

static const mln_buffer_view fixture_tiles[] = {
  MLN_BUFFER_LITERAL("fixture://dem/{z}/{x}/{y}.png"),
};

static void add_dem_source(mln_map map, const char* id) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_raster_dem_source_tiles(
                     map, mln_test_view_of(id), fixture_tiles, 1, NULL,
                     &completion.descriptor, NULL
                   )
  );
}

// Deep-copies one layer result, whose views die with the callback.
typedef struct layer_probe {
  atomic_bool done;
  mln_status status;
  bool found;
  mln_style_layer_info info;
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
    const mln_style_layer_result* layer = result->value;
    probe->info = layer->info;
    copy_view_into(layer->info.type, probe->type, sizeof(probe->type), NULL);
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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_layer_info(map, mln_test_view_of(id), &completion, NULL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.status);
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

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_min_zoom(
      map, MLN_BUFFER_LITERAL("info-layer"), 3.0, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_max_zoom(
      map, MLN_BUFFER_LITERAL("info-layer"), 12.0, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_visibility(
      map, MLN_BUFFER_LITERAL("info-layer"), MLN_STYLE_LAYER_VISIBILITY_NONE,
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "visibility is invalid",
    mln_map_set_layer_visibility(
      map, MLN_BUFFER_LITERAL("info-layer"), 7, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_source_layer(
                     map, MLN_BUFFER_LITERAL("info-layer"),
                     MLN_BUFFER_LITERAL("roads"), &completion.descriptor, NULL
                   )
  );
  probe = take_layer_result(runtime, map, "info-layer");
  TEST_ASSERT_TRUE(probe.found);
  TEST_ASSERT_EQUAL_DOUBLE(3.0, probe.info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, probe.info.max_zoom);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_LAYER_VISIBILITY_NONE, probe.info.visibility
  );
  TEST_ASSERT_EQUAL_STRING("roads", probe.source_layer);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads one layer property, filter, or JSON copy, or "" when it is undefined.
static mln_status read_layer_property(
  mln_map map, const char* layer, const char* name, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_property(
                     map, mln_test_view_of(layer), mln_test_view_of(name),
                     &completion.descriptor, NULL
                   )
  );
  return mln_test_style_finish_text(&completion, out, capacity, NULL);
}

static mln_status read_layer_filter(
  mln_map map, const char* layer, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_filter(
                     map, mln_test_view_of(layer), &completion.descriptor, NULL
                   )
  );
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
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-radius"),
      MLN_BUFFER_LITERAL("4"), &completion.descriptor, NULL
    )
  );
  EXPECT_LAYER_PROPERTY("4.0", map, "dots", "circle-radius");
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-color"),
      MLN_BUFFER_LITERAL("\"#ff0000\""), &completion.descriptor, NULL
    )
  );
  EXPECT_LAYER_PROPERTY(
    "[\"rgba\",255.0,0.0,0.0,1.0]", map, "dots", "circle-color"
  );

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "layer property",
    mln_map_set_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("fill-color"),
      MLN_BUFFER_LITERAL("\"#00ff00\""), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "layer property",
    mln_map_set_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-radius"),
      MLN_BUFFER_LITERAL("\"wide\""), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "layer property",
    mln_map_set_layer_property(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("circle-radius"),
      MLN_BUFFER_LITERAL("{\"radius\":"), &completion.descriptor, NULL
    )
  );
  EXPECT_LAYER_PROPERTY("4.0", map, "dots", "circle-radius");
  // A name the layer type lacks reads as undefined rather than failing.
  EXPECT_LAYER_PROPERTY("", map, "dots", "fill-color");

  char filter[256];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_filter(map, "dots", filter, sizeof(filter))
  );
  TEST_ASSERT_EQUAL_STRING("", filter);
  const mln_buffer_view kind_filter =
    MLN_BUFFER_LITERAL("[\"==\",[\"get\",\"kind\"],\"park\"]");
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_filter(
                     map, MLN_BUFFER_LITERAL("dots"), &kind_filter,
                     &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_filter(map, "dots", filter, sizeof(filter))
  );
  TEST_ASSERT_EQUAL_STRING("[\"==\",[\"get\",\"kind\"],\"park\"]", filter);
  const mln_buffer_view bad_filter =
    MLN_BUFFER_LITERAL("[\"no-such-operator\",1]");
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "filter",
    mln_map_set_layer_filter(
      map, MLN_BUFFER_LITERAL("dots"), &bad_filter, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_filter(map, "dots", filter, sizeof(filter))
  );
  TEST_ASSERT_EQUAL_STRING("[\"==\",[\"get\",\"kind\"],\"park\"]", filter);
  // A null filter clears it.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_filter(
      map, MLN_BUFFER_LITERAL("dots"), NULL, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_filter(map, "dots", filter, sizeof(filter))
  );
  TEST_ASSERT_EQUAL_STRING("", filter);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status read_layer_json(
  mln_map map, const char* layer, char* out, size_t capacity, bool* found
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_layer_json(
                     map, mln_test_view_of(layer), &completion.descriptor, NULL
                   )
  );
  return mln_test_style_finish_text(&completion, out, capacity, found);
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
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_min_zoom(
      map, MLN_BUFFER_LITERAL("paper"), 2.0, &completion.descriptor, NULL
    )
  );

  char json[512];
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_json(map, "paper", json, sizeof(json), &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_NOT_NULL(strstr(json, "\"id\":\"paper\""));
  TEST_ASSERT_NOT_NULL(strstr(json, "\"type\":\"background\""));
  TEST_ASSERT_NOT_NULL(
    strstr(json, "\"background-color\":[\"rgba\",255.0,0.0,0.0,1.0]")
  );
  TEST_ASSERT_NOT_NULL(strstr(json, "\"minzoom\":2.0"));

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_remove_style_layer(
      map, MLN_BUFFER_LITERAL("paper"), &completion.descriptor, NULL
    )
  );
  add_layer(map, json, "");
  char again[512];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_json(map, "paper", again, sizeof(again), &found)
  );
  TEST_ASSERT_EQUAL_STRING(json, again);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, read_layer_json(map, "missing", json, sizeof(json), &found)
  );
  TEST_ASSERT_FALSE(found);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void move_layer(mln_map map, const char* layer, const char* before) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_move_style_layer(
                     map, mln_test_view_of(layer), mln_test_view_of(before),
                     &completion.descriptor, NULL
                   )
  );
}

// Both listings report the layer stack bottom to top, as moves leave it.
static void layers_list_in_style_order_after_moves(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_source(map, "points");
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_vector_source_tiles(
                     map, MLN_BUFFER_LITERAL("tiles"), fixture_tiles, 1, NULL,
                     &completion.descriptor, NULL
                   )
  );
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

  const mln_test_style_list ids = mln_test_style_list_layer_ids(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, ids.status);
  TEST_ASSERT_EQUAL_size_t(3, ids.count);
  TEST_ASSERT_EQUAL_STRING("roads", ids.entries[0].id);
  TEST_ASSERT_EQUAL_STRING("dots", ids.entries[1].id);
  TEST_ASSERT_EQUAL_STRING("paper", ids.entries[2].id);

  const mln_test_style_list layers = mln_test_style_list_layers(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, layers.status);
  TEST_ASSERT_EQUAL_size_t(3, layers.count);
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

static mln_status copy_layer_text(
  mln_map map, const char* layer, bool source_layer, char* out
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    source_layer ? mln_map_copy_layer_source_layer(
                     map, mln_test_view_of(layer), &completion.descriptor, NULL
                   )
                 : mln_map_copy_layer_source_id(
                     map, mln_test_view_of(layer), &completion.descriptor, NULL
                   )
  );
  return mln_test_style_finish_text(&completion, out, 64, NULL);
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

  char text[64];
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_source_id(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL("not-yet-added"),
      &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_layer_text(map, "dots", false, text)
  );
  TEST_ASSERT_EQUAL_STRING("not-yet-added", text);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_layer_text(map, "dots", true, text)
  );
  TEST_ASSERT_EQUAL_STRING("", text);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_source_layer(
                     map, MLN_BUFFER_LITERAL("dots"),
                     MLN_BUFFER_LITERAL("pois"), &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_layer_text(map, "dots", true, text)
  );
  TEST_ASSERT_EQUAL_STRING("pois", text);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_source_layer(
                     map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL(""),
                     &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_layer_text(map, "dots", true, text)
  );
  TEST_ASSERT_EQUAL_STRING("", text);

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "does not take a source",
    mln_map_set_layer_source_id(
      map, MLN_BUFFER_LITERAL("paper"), MLN_BUFFER_LITERAL("points"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "does not take a source-layer",
    mln_map_set_layer_source_layer(
      map, MLN_BUFFER_LITERAL("paper"), MLN_BUFFER_LITERAL("pois"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "source_id must not be empty",
    mln_map_set_layer_source_id(
      map, MLN_BUFFER_LITERAL("dots"), MLN_BUFFER_LITERAL(""),
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, copy_layer_text(map, "paper", false, text)
  );
  TEST_ASSERT_EQUAL_STRING("", text);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef mln_status (*terrain_adder)(
  mln_map map, mln_buffer_view layer_id, mln_buffer_view source_id,
  mln_buffer_view before_layer_id, const mln_completion* completion,
  mln_diagnostic* diagnostic
);

// Hillshade and color-relief layers draw a raster DEM source, so the typed
// adders check the source kind the style-spec JSON path leaves to rendering.
static void terrain_layers_require_a_raster_dem_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_dem_source(map, "dem");
  add_source(map, "points");
  add_layer(map, "{\"id\":\"paper\",\"type\":\"background\"}", "");

  static const struct {
    const char* layer;
    const char* type;
    terrain_adder add;
  } adders[] = {
    {"hillshade", "hillshade", mln_map_add_hillshade_layer},
    {"relief", "color-relief", mln_map_add_color_relief_layer},
  };
  for (size_t index = 0; index < 2; index += 1) {
    const mln_buffer_view layer = mln_test_view_of(adders[index].layer);
    // Each goes under the paper, before the layer the loop added last.
    MLN_TEST_AWAIT_COMMAND(
      MLN_STATUS_OK, adders[index].add(
                       map, layer, MLN_BUFFER_LITERAL("dem"),
                       MLN_BUFFER_LITERAL("paper"), &completion.descriptor, NULL
                     )
    );
    layer_probe probe = take_layer_result(runtime, map, adders[index].layer);
    TEST_ASSERT_EQUAL_STRING(adders[index].type, probe.type);
    TEST_ASSERT_EQUAL_STRING("dem", probe.source_id);

    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_INVALID_ARGUMENT, "layer already exists",
      adders[index].add(
        map, layer, MLN_BUFFER_LITERAL("dem"), MLN_BUFFER_LITERAL(""),
        &completion.descriptor, NULL
      )
    );
    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_INVALID_ARGUMENT, "not a raster DEM source",
      adders[index].add(
        map, MLN_BUFFER_LITERAL("on-points"), MLN_BUFFER_LITERAL("points"),
        MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
      )
    );
    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_NOT_FOUND, "before_layer_id does not exist",
      adders[index].add(
        map, MLN_BUFFER_LITERAL("before-missing"), MLN_BUFFER_LITERAL("dem"),
        MLN_BUFFER_LITERAL("missing"), &completion.descriptor, NULL
      )
    );
  }
  const mln_test_style_list ids = mln_test_style_list_layer_ids(map);
  TEST_ASSERT_EQUAL_size_t(3, ids.count);
  TEST_ASSERT_EQUAL_STRING("hillshade", ids.entries[0].id);
  TEST_ASSERT_EQUAL_STRING("relief", ids.entries[1].id);
  TEST_ASSERT_EQUAL_STRING("paper", ids.entries[2].id);

  // The color ramp is an ordinary property of the color-relief layer.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_property(
      map, MLN_BUFFER_LITERAL("relief"),
      MLN_BUFFER_LITERAL("color-relief-color"),
      MLN_BUFFER_LITERAL(
        "[\"interpolate\",[\"linear\"],[\"elevation\"],0,\"#000000\",1000,"
        "\"#ffffff\"]"
      ),
      &completion.descriptor, NULL
    )
  );
  char ramp[256];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    read_layer_property(map, "relief", "color-relief-color", ramp, sizeof(ramp))
  );
  TEST_ASSERT_NOT_NULL(strstr(ramp, "\"elevation\""));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status set_indicator_location(
  mln_map map, mln_buffer_view layer, const mln_completion* completion
) {
  return mln_map_set_location_indicator_location(
    map, layer, (mln_lat_lng){.latitude = 37.5, .longitude = -122.25}, 12.0,
    completion, NULL
  );
}
static mln_status set_indicator_bearing(
  mln_map map, mln_buffer_view layer, const mln_completion* completion
) {
  return mln_map_set_location_indicator_bearing(
    map, layer, 90.0, completion, NULL
  );
}
static mln_status set_indicator_radius(
  mln_map map, mln_buffer_view layer, const mln_completion* completion
) {
  return mln_map_set_location_indicator_accuracy_radius(
    map, layer, 25.0, completion, NULL
  );
}
static mln_status set_indicator_image(
  mln_map map, mln_buffer_view layer, const mln_completion* completion
) {
  return mln_map_set_location_indicator_image_name(
    map, layer, MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING,
    MLN_BUFFER_LITERAL("arrow"), completion, NULL
  );
}

static mln_status set_indicator_top_image(
  mln_map map, mln_buffer_view layer, const mln_completion* completion
) {
  return mln_map_set_location_indicator_image_name(
    map, layer, MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP,
    MLN_BUFFER_LITERAL("puck-top"), completion, NULL
  );
}

// The typed setters write location-indicator properties in the renderer's
// order and units, and refuse every other layer type.
static void location_indicator_setters_write_its_properties(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  add_source(map, "points");
  add_layer(
    map, "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"points\"}", ""
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_location_indicator_layer(
                     map, MLN_BUFFER_LITERAL("puck"), MLN_BUFFER_LITERAL(""),
                     &completion.descriptor, NULL
                   )
  );

  static const struct {
    const char* label;
    mln_status (*set)(mln_map, mln_buffer_view, const mln_completion*);
    const char* property;
    // A fragment of the property's JSON.
    const char* expected;
  } setters[] = {
    {"location", set_indicator_location, "location", "[37.5,-122.25,12.0]"},
    {"bearing", set_indicator_bearing, "bearing", "90.0"},
    {"accuracy radius", set_indicator_radius, "accuracy-radius", "25.0"},
    {"bearing image", set_indicator_image, "bearing-image",
     "\"name\":\"arrow\""},
    {"top image", set_indicator_top_image, "top-image",
     "\"name\":\"puck-top\""},
  };
  for (size_t index = 0; index < sizeof(setters) / sizeof(setters[0]);
       index += 1) {
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK,
      setters[index].set(
        map, MLN_BUFFER_LITERAL("puck"), &completion.descriptor
      ),
      setters[index].label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, mln_test_completion_settle(&completion),
      setters[index].label
    );
    char value[128];
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      read_layer_property(
        map, "puck", setters[index].property, value, sizeof(value)
      )
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(value, setters[index].expected), setters[index].label
    );

    completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, setters[index].set(
                       map, MLN_BUFFER_LITERAL("dots"), &completion.descriptor
                     )
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_finish(&completion),
      setters[index].label
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(
        mln_test_completion_diagnostic(&completion),
        "not a location indicator layer"
      ),
      setters[index].label
    );
    mln_test_completion_destroy(&completion);
  }

  // Values the renderer cannot hold are rejected at submission and change
  // nothing.
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "radius must be non-negative",
    mln_map_set_location_indicator_accuracy_radius(
      map, MLN_BUFFER_LITERAL("puck"), -1.0, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "bearing must fit in finite float32",
    mln_map_set_location_indicator_bearing(
      map, MLN_BUFFER_LITERAL("puck"), 1e39, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "altitude must be finite",
    mln_map_set_location_indicator_location(
      map, MLN_BUFFER_LITERAL("puck"),
      (mln_lat_lng){.latitude = 37.5, .longitude = -122.25}, INFINITY,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "image_kind is invalid",
    mln_map_set_location_indicator_image_name(
      map, MLN_BUFFER_LITERAL("puck"), 9, MLN_BUFFER_LITERAL("arrow"),
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_LAYER_PROPERTY("25.0", map, "puck", "accuracy-radius");
  EXPECT_LAYER_PROPERTY("90.0", map, "puck", "bearing");
  EXPECT_LAYER_PROPERTY("[37.5,-122.25,12.0]", map, "puck", "location");

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(layer_result_reports_scalars_and_carries_the_source_ids);
  RUN_TEST(layer_properties_and_filters_round_trip_through_json);
  RUN_TEST(layer_json_serializes_the_current_layer);
  RUN_TEST(layers_list_in_style_order_after_moves);
  RUN_TEST(source_bindings_change_only_on_layers_that_take_a_source);
  RUN_TEST(terrain_layers_require_a_raster_dem_source);
  RUN_TEST(location_indicator_setters_write_its_properties);
}
