// Feature state: the map-owned store behind set, get, and remove, the
// selectors that address it, and its delivery to the renderer.

#include <math.h>

#include "support/style.h"
#include "support/test_support.h"

static mln_feature_state_selector selector_with_id(
  const char* source_id, mln_feature_id feature_id
) {
  return (mln_feature_state_selector){
    .size = sizeof(mln_feature_state_selector),
    .fields = MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
    .source_id = mln_buffer_view_from_cstr(source_id),
    .feature_id = feature_id,
  };
}

static mln_feature_id string_id(const char* text) {
  return (mln_feature_id){
    .type = MLN_FEATURE_ID_TYPE_STRING,
    .data.string_value = mln_buffer_view_from_cstr(text),
  };
}

static mln_feature_id uint_id(uint64_t value) {
  return (mln_feature_id){
    .type = MLN_FEATURE_ID_TYPE_UINT, .data.uint_value = value
  };
}

static mln_feature_id int_id(int64_t value) {
  return (mln_feature_id){
    .type = MLN_FEATURE_ID_TYPE_INT, .data.int_value = value
  };
}

static mln_feature_id double_id(double value) {
  return (mln_feature_id){
    .type = MLN_FEATURE_ID_TYPE_DOUBLE, .data.double_value = value
  };
}

static mln_feature_state_selector selector_for(
  const char* source_id, const char* source_layer_id, const char* feature_id
) {
  mln_feature_state_selector selector = {
    .size = sizeof(mln_feature_state_selector),
    .source_id = mln_buffer_view_from_cstr(source_id),
  };
  if (source_layer_id != NULL) {
    selector.source_layer_id = mln_buffer_view_from_cstr(source_layer_id);
  }
  if (feature_id != NULL) {
    selector.fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    selector.feature_id = string_id(feature_id);
  }
  return selector;
}

static void set_state(
  mln_map map, mln_feature_state_selector selector, const char* state
) {
  MLN_TEST_AWAIT_OK(mln_map_set_feature_state(
    map, &selector, mln_buffer_view_from_cstr(state), &completion.descriptor,
    MLN_TEST_DIAGNOSTIC
  ));
}

static void remove_state(mln_map map, mln_feature_state_selector selector) {
  MLN_TEST_AWAIT_OK(mln_map_remove_feature_state(
    map, &selector, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
}

static void read_state(
  mln_map map, mln_feature_state_selector selector, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_map_get_feature_state(
    map, &selector, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  bool found = false;
  MLN_TEST_OK(mln_test_style_finish_text(&completion, out, capacity, &found));
  TEST_ASSERT_TRUE(found);
}

#define EXPECT_STATE(expected, map, selector)         \
  do {                                                \
    char state_json[256];                             \
    read_state((map), (selector), state_json, 256);   \
    TEST_ASSERT_EQUAL_STRING((expected), state_json); \
  } while (false)

static void state_merges_on_set_and_prunes_on_remove(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_feature_state_selector feature =
    selector_for("points", NULL, "feature-1");

  // State belongs to the map store, so no style or source needs to exist.
  EXPECT_STATE("{}", map, feature);
  set_state(map, feature, "{\"hover\":true}");
  set_state(map, feature, "{\"count\":3}");
  set_state(map, feature, "{\"count\":4}");
  char merged[256];
  read_state(map, feature, merged, sizeof(merged));
  TEST_ASSERT_NOT_NULL(strstr(merged, "\"hover\":true"));
  TEST_ASSERT_NOT_NULL(strstr(merged, "\"count\":4"));
  TEST_ASSERT_NULL(strstr(merged, "\"count\":3"));

  mln_feature_state_selector hover = feature;
  hover.state_key = MLN_BUFFER_LITERAL("hover");
  remove_state(map, hover);
  EXPECT_STATE("{\"count\":4}", map, feature);
  mln_feature_state_selector count = hover;
  count.state_key = MLN_BUFFER_LITERAL("count");
  remove_state(map, count);
  EXPECT_STATE("{}", map, feature);
  set_state(map, feature, "{\"label\":\"a\"}");
  EXPECT_STATE("{\"label\":\"a\"}", map, feature);
  remove_state(map, feature);
  EXPECT_STATE("{}", map, feature);
  // An empty state key selects every key, as no key does.
  set_state(map, feature, "{\"label\":\"b\",\"n\":1}");
  mln_feature_state_selector every_key = feature;
  every_key.state_key = (mln_buffer_view){.data = "label", .size = 0};
  remove_state(map, every_key);
  EXPECT_STATE("{}", map, feature);

  // The empty feature ID is an ID of its own, which its field bit tells from
  // no ID.
  const mln_feature_state_selector empty_id = selector_for("points", NULL, "");
  set_state(map, empty_id, "{\"empty\":true}");
  set_state(map, feature, "{\"named\":true}");
  EXPECT_STATE("{\"empty\":true}", map, empty_id);
  EXPECT_STATE("{\"named\":true}", map, feature);
  remove_state(map, selector_for("points", NULL, NULL));
  EXPECT_STATE("{}", map, empty_id);
  EXPECT_STATE("{}", map, feature);

  const mln_feature_state_selector roads_one =
    selector_for("tiles", "roads", "1");
  const mln_feature_state_selector roads_two =
    selector_for("tiles", "roads", "2");
  const mln_feature_state_selector water_one =
    selector_for("tiles", "water", "1");
  const mln_feature_state_selector bare_one = selector_for("tiles", NULL, "1");
  const mln_feature_state_selector other_one = selector_for("other", NULL, "1");
  set_state(map, roads_one, "{\"n\":1}");
  set_state(map, roads_two, "{\"n\":2}");
  set_state(map, water_one, "{\"n\":3}");
  set_state(map, bare_one, "{\"n\":4}");
  set_state(map, other_one, "{\"n\":5}");
  // An empty source layer ID is no source layer.
  EXPECT_STATE("{\"n\":4}", map, selector_for("tiles", "", "1"));

  remove_state(map, selector_for("tiles", "roads", NULL));
  EXPECT_STATE("{}", map, roads_one);
  EXPECT_STATE("{}", map, roads_two);
  EXPECT_STATE("{\"n\":3}", map, water_one);
  EXPECT_STATE("{\"n\":4}", map, bare_one);

  remove_state(map, selector_for("tiles", NULL, NULL));
  EXPECT_STATE("{}", map, bare_one);
  EXPECT_STATE("{\"n\":3}", map, water_one);
  EXPECT_STATE("{\"n\":5}", map, other_one);

  remove_state(map, selector_for("missing", NULL, NULL));
  remove_state(map, hover);
  EXPECT_STATE("{\"n\":5}", map, other_one);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_string_and_a_number_with_the_same_text_name_one_feature(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  set_state(map, selector_with_id("points", uint_id(7)), "{\"n\":1}");
  EXPECT_STATE("{\"n\":1}", map, selector_with_id("points", string_id("7")));
  EXPECT_STATE("{\"n\":1}", map, selector_with_id("points", int_id(7)));
  // A whole double's text has no trailing ".0".
  EXPECT_STATE("{\"n\":1}", map, selector_with_id("points", double_id(7.0)));
  EXPECT_STATE("{}", map, selector_with_id("points", string_id("7.0")));

  set_state(map, selector_with_id("points", int_id(-3)), "{\"n\":2}");
  EXPECT_STATE("{\"n\":2}", map, selector_with_id("points", string_id("-3")));
  set_state(map, selector_with_id("points", uint_id(UINT64_MAX)), "{\"n\":3}");
  EXPECT_STATE(
    "{\"n\":3}", map,
    selector_with_id("points", string_id("18446744073709551615"))
  );

  // A double's text is its shortest form, which printf's formats don't give.
  set_state(map, selector_with_id("points", double_id(0.1)), "{\"n\":4}");
  EXPECT_STATE("{\"n\":4}", map, selector_with_id("points", string_id("0.1")));
  set_state(map, selector_with_id("points", double_id(1e21)), "{\"n\":5}");
  EXPECT_STATE("{\"n\":5}", map, selector_with_id("points", string_id("1e21")));

  // Removal goes by the same text.
  remove_state(map, selector_with_id("points", string_id("7")));
  EXPECT_STATE("{}", map, selector_with_id("points", uint_id(7)));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

enum feature_state_call_kind {
  FEATURE_STATE_SET,
  FEATURE_STATE_GET,
  FEATURE_STATE_REMOVE,
};

// One call a validation row submits: which entry point, its selector, and the
// state that a set carries.
typedef struct feature_state_call {
  enum feature_state_call_kind kind;
  bool null_selector;
  mln_feature_state_selector selector;
  mln_buffer_view state;
} feature_state_call;

static mln_status submit_feature_state_call(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_map map = *(const mln_map*)context;
  const feature_state_call* call = descriptor;
  const mln_feature_state_selector* selector =
    call->null_selector ? NULL : &call->selector;
  const mln_completion completion = mln_test_discard_completion();
  switch (call->kind) {
    case FEATURE_STATE_SET:
      return mln_map_set_feature_state(
        map, selector, call->state, &completion, diagnostic
      );
    case FEATURE_STATE_GET:
      return mln_map_get_feature_state(map, selector, &completion, diagnostic);
    case FEATURE_STATE_REMOVE:
      return mln_map_remove_feature_state(
        map, selector, &completion, diagnostic
      );
  }
  return MLN_STATUS_NATIVE_ERROR;
}

static void null_selector(void* descriptor) {
  ((feature_state_call*)descriptor)->null_selector = true;
}
static void undersized_selector(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.size -= 1;
}
static void unknown_selector_field(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.fields |= UINT32_C(1) << 31;
}
static void empty_source_id(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.source_id = (mln_buffer_view){0};
}
static void null_source_id_bytes(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.source_id =
    (mln_buffer_view){.data = NULL, .size = 4};
}
static void no_feature_id(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.fields &=
    ~(uint32_t)MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
}
static void remove_key_without_feature_id(void* descriptor) {
  feature_state_call* call = descriptor;
  no_feature_id(descriptor);
  call->kind = FEATURE_STATE_REMOVE;
  call->selector.state_key = MLN_BUFFER_LITERAL("hover");
}
static void remove_empty_key_without_feature_id(void* descriptor) {
  feature_state_call* call = descriptor;
  no_feature_id(descriptor);
  call->kind = FEATURE_STATE_REMOVE;
  call->selector.state_key = (mln_buffer_view){.data = "hover", .size = 0};
}
static void null_source_layer_id_bytes(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.source_layer_id =
    (mln_buffer_view){.data = NULL, .size = 4};
}
static void null_state_key_bytes(void* descriptor) {
  feature_state_call* call = descriptor;
  call->kind = FEATURE_STATE_REMOVE;
  call->selector.state_key = (mln_buffer_view){.data = NULL, .size = 4};
}
static void remove_everything_under_a_source(void* descriptor) {
  no_feature_id(descriptor);
  ((feature_state_call*)descriptor)->kind = FEATURE_STATE_REMOVE;
}
static void unset_feature_id_type(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.feature_id.type = 0;
}
static void unknown_feature_id_type(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.feature_id.type = 99;
}
static void remove_with_unknown_feature_id_type(void* descriptor) {
  unknown_feature_id_type(descriptor);
  ((feature_state_call*)descriptor)->kind = FEATURE_STATE_REMOVE;
}
static void get_with_unknown_feature_id_type(void* descriptor) {
  unknown_feature_id_type(descriptor);
  ((feature_state_call*)descriptor)->kind = FEATURE_STATE_GET;
}
static void null_feature_id_bytes(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.feature_id.data.string_value =
    (mln_buffer_view){.data = NULL, .size = 4};
}
static void nan_feature_id(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.feature_id = double_id(NAN);
}
static void infinite_feature_id(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.feature_id = double_id(-INFINITY);
}
static void unknown_feature_id_type_without_its_field(void* descriptor) {
  unknown_feature_id_type(descriptor);
  remove_everything_under_a_source(descriptor);
}
static void empty_state(void* descriptor) {
  ((feature_state_call*)descriptor)->state = (mln_buffer_view){0};
}
static void unparsable_state(void* descriptor) {
  ((feature_state_call*)descriptor)->state = MLN_BUFFER_LITERAL("{\"hover\":");
}
static void array_state(void* descriptor) {
  ((feature_state_call*)descriptor)->state = MLN_BUFFER_LITERAL("[true]");
}

static void malformed_selectors_are_rejected_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  static const mln_test_validation_case cases[] = {
    {"set with defaults", NULL, MLN_STATUS_OK, NULL},
    {"remove everything under a source", remove_everything_under_a_source,
     MLN_STATUS_OK, NULL},
    {"null selector", null_selector, MLN_STATUS_INVALID_ARGUMENT,
     "must not be null"},
    {"undersized selector", undersized_selector, MLN_STATUS_INVALID_ARGUMENT,
     "size is too small"},
    {"unknown selector field", unknown_selector_field,
     MLN_STATUS_INVALID_ARGUMENT, "unknown fields"},
    {"empty source_id", empty_source_id, MLN_STATUS_INVALID_ARGUMENT,
     "source_id must not be empty"},
    {"null source_id bytes", null_source_id_bytes, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"set without feature_id", no_feature_id, MLN_STATUS_INVALID_ARGUMENT,
     "requires feature_id"},
    {"remove state_key without feature_id", remove_key_without_feature_id,
     MLN_STATUS_INVALID_ARGUMENT, "state_key requires feature_id"},
    {"remove empty state_key without feature_id",
     remove_empty_key_without_feature_id, MLN_STATUS_OK, NULL},
    {"null source_layer_id bytes", null_source_layer_id_bytes,
     MLN_STATUS_INVALID_ARGUMENT, "must not be null"},
    {"null state_key bytes", null_state_key_bytes, MLN_STATUS_INVALID_ARGUMENT,
     "must not be null"},
    {"unset feature_id type", unset_feature_id_type,
     MLN_STATUS_INVALID_ARGUMENT, "unknown type"},
    {"unknown feature_id type", unknown_feature_id_type,
     MLN_STATUS_INVALID_ARGUMENT, "unknown type"},
    {"get with unknown feature_id type", get_with_unknown_feature_id_type,
     MLN_STATUS_INVALID_ARGUMENT, "unknown type"},
    {"remove with unknown feature_id type", remove_with_unknown_feature_id_type,
     MLN_STATUS_INVALID_ARGUMENT, "unknown type"},
    {"unknown feature_id type without its field",
     unknown_feature_id_type_without_its_field, MLN_STATUS_OK, NULL},
    {"null feature_id bytes", null_feature_id_bytes,
     MLN_STATUS_INVALID_ARGUMENT, "must not be null"},
    {"NaN feature_id", nan_feature_id, MLN_STATUS_INVALID_ARGUMENT,
     "must be finite"},
    {"infinite feature_id", infinite_feature_id, MLN_STATUS_INVALID_ARGUMENT,
     "must be finite"},
    {"empty state", empty_state, MLN_STATUS_INVALID_ARGUMENT,
     "must not be empty"},
    {"unparsable state", unparsable_state, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"array state", array_state, MLN_STATUS_INVALID_ARGUMENT,
     "must be a JSON object"},
  };
  const feature_state_call defaults = {
    .kind = FEATURE_STATE_SET,
    .selector = selector_for("points", NULL, "feature-1"),
    .state = MLN_BUFFER_LITERAL("{\"hover\":true}"),
  };
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_feature_state_call, &map
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#define POINT_SOURCE_JSON                                           \
  "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\"," \
  "\"features\":[{\"type\":\"Feature\",\"id\":\"feature-1\","       \
  "\"properties\":{},\"geometry\":{\"type\":\"Point\","             \
  "\"coordinates\":[0,0]}}]}}"
#define CIRCLE_LAYER_JSON                                         \
  "{\"id\":\"circle\",\"type\":\"circle\",\"source\":\"points\"," \
  "\"paint\":{\"circle-radius\":8}}"

static const char point_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":" POINT_SOURCE_JSON
  "},\"layers\":[" CIRCLE_LAYER_JSON "]}";

typedef struct rendered_query {
  const mln_test_render_fixture* fixture;
  mln_test_feature_list list;
} rendered_query;

static bool feature_rendered(void* context) {
  rendered_query* query = context;
  query->list = mln_test_style_query_rendered(query->fixture);
  return query->list.status == MLN_STATUS_OK && query->list.count == 1;
}

static void rendered_features_carry_state_from_the_last_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(point_style_json));
  const mln_feature_state_selector feature =
    selector_for("points", NULL, "feature-1");
  // Set before the source ever renders, so its first frame carries the state.
  set_state(map, feature, "{\"hover\":true}");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  rendered_query query = {.fixture = &fixture};
  TEST_ASSERT_TRUE(
    mln_test_style_render_until(&fixture, feature_rendered, &query, "a feature")
  );
  TEST_ASSERT_EQUAL_STRING("points", query.list.features[0].source_id);
  TEST_ASSERT_NOT_NULL(
    strstr(query.list.features[0].feature, "\"id\":\"feature-1\"")
  );
  TEST_ASSERT_TRUE(query.list.features[0].has_state);
  TEST_ASSERT_EQUAL_STRING("{\"hover\":true}", query.list.features[0].state);

  // The committed command reaches the store at once and the renderer only
  // with the next frame, and no frame has been requested since.
  set_state(map, feature, "{\"hover\":false}");
  EXPECT_STATE("{\"hover\":false}", map, feature);
  TEST_ASSERT_TRUE(feature_rendered(&query));
  TEST_ASSERT_EQUAL_STRING("{\"hover\":true}", query.list.features[0].state);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(&fixture)
  );
  TEST_ASSERT_TRUE(feature_rendered(&query));
  TEST_ASSERT_EQUAL_STRING("{\"hover\":false}", query.list.features[0].state);

  remove_state(map, feature);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(&fixture)
  );
  TEST_ASSERT_TRUE(feature_rendered(&query));
  TEST_ASSERT_FALSE(query.list.features[0].has_state);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Renders until the one point shows, carrying its state when `has_state`.
static void render_point(
  const mln_test_render_fixture* fixture, rendered_query* query, bool has_state
) {
  TEST_ASSERT_TRUE(
    mln_test_style_render_until(fixture, feature_rendered, query, "a feature")
  );
  TEST_ASSERT_EQUAL(has_state, query->list.features[0].has_state);
}

static void a_removed_source_takes_its_state_with_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(point_style_json));
  const mln_feature_state_selector feature =
    selector_for("points", NULL, "feature-1");
  const mln_feature_state_selector other = selector_for("other", NULL, "1");
  set_state(map, feature, "{\"hover\":true}");
  set_state(map, other, "{\"n\":1}");
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  rendered_query query = {.fixture = &fixture};
  render_point(&fixture, &query, true);

  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("circle"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("points"), &completion.descriptor, NULL
  ));
  EXPECT_STATE("{}", map, feature);
  EXPECT_STATE("{\"n\":1}", map, other);

  // No frame renders between the removal and the new source, so the renderer
  // keeps the source it drew under this ID.
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("points"), MLN_BUFFER_LITERAL(POINT_SOURCE_JSON),
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, MLN_BUFFER_LITERAL(CIRCLE_LAYER_JSON), MLN_BUFFER_LITERAL(""),
    &completion.descriptor, NULL
  ));
  EXPECT_STATE("{}", map, feature);
  render_point(&fixture, &query, false);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_style_load_clears_every_source_state(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(point_style_json));
  const mln_feature_state_selector feature =
    selector_for("points", NULL, "feature-1");
  const mln_feature_state_selector other = selector_for("other", NULL, "1");
  set_state(map, feature, "{\"hover\":true}");
  set_state(map, other, "{\"n\":1}");
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  rendered_query query = {.fixture = &fixture};
  render_point(&fixture, &query, true);

  // The new style has the same source, which the renderer keeps.
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(point_style_json));
  EXPECT_STATE("{}", map, feature);
  EXPECT_STATE("{}", map, other);
  render_point(&fixture, &query, false);

  // State set once the style has loaded reaches its source.
  set_state(map, feature, "{\"hover\":true}");
  render_point(&fixture, &query, true);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Points whose IDs are a string, an unsigned and a negative integer, and two
// doubles whose shortest forms printf's formats don't give.
static const char typed_id_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"id\":\"8\",\"properties\":{\"name\":\"string\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]}},"
  "{\"type\":\"Feature\",\"id\":7,\"properties\":{\"name\":\"uint\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[5,0]}},"
  "{\"type\":\"Feature\",\"id\":-3,\"properties\":{\"name\":\"int\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[-5,0]}},"
  "{\"type\":\"Feature\",\"id\":0.1,\"properties\":{\"name\":\"tenth\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,5]}},"
  "{\"type\":\"Feature\",\"id\":1e21,\"properties\":{\"name\":\"large\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,-5]}}"
  "]}}},\"layers\":[{\"id\":\"circle\",\"type\":\"circle\",\"source\":"
  "\"points\",\"paint\":{\"circle-radius\":4}}]}";

static bool every_typed_id_feature_rendered(void* context) {
  rendered_query* query = context;
  query->list = mln_test_style_query_rendered(query->fixture);
  return query->list.status == MLN_STATUS_OK && query->list.count == 5;
}

static void typed_ids_reach_the_features_that_carry_them(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(typed_id_style_json));
  // Each ID has a type other than its feature's, except the doubles, so the
  // state reaches a feature only when the engine keys both by the same text.
  set_state(
    map, selector_with_id("points", uint_id(8)), "{\"name\":\"string\"}"
  );
  set_state(
    map, selector_with_id("points", string_id("7")), "{\"name\":\"uint\"}"
  );
  set_state(map, selector_with_id("points", int_id(-3)), "{\"name\":\"int\"}");
  set_state(
    map, selector_with_id("points", double_id(0.1)), "{\"name\":\"tenth\"}"
  );
  set_state(
    map, selector_with_id("points", double_id(1e21)), "{\"name\":\"large\"}"
  );

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  rendered_query query = {.fixture = &fixture};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, every_typed_id_feature_rendered, &query, "every feature"
  ));
  for (size_t i = 0; i < query.list.count; ++i) {
    const mln_test_feature* feature = &query.list.features[i];
    TEST_ASSERT_TRUE_MESSAGE(feature->has_state, feature->feature);
    // The state names the feature it was set for.
    char properties[128];
    snprintf(
      properties, sizeof(properties), "\"properties\":%s", feature->state
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(feature->feature, properties), feature->feature
    );
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(state_merges_on_set_and_prunes_on_remove);
  RUN_TEST(a_string_and_a_number_with_the_same_text_name_one_feature);
  RUN_TEST(malformed_selectors_are_rejected_at_submission);
  RUN_TEST(rendered_features_carry_state_from_the_last_frame);
  RUN_TEST(a_removed_source_takes_its_state_with_it);
  RUN_TEST(a_style_load_clears_every_source_state);
  RUN_TEST(typed_ids_reach_the_features_that_carry_them);
}
