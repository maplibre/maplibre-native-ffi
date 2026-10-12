// Feature state: the map-owned store behind set, get, and remove, the
// selectors that address it, and its delivery to the renderer.

#include "support/style.h"
#include "support/test_support.h"

static mln_feature_state_selector selector_for(
  const char* source_id, const char* source_layer_id, const char* feature_id
) {
  mln_feature_state_selector selector = {
    .size = sizeof(mln_feature_state_selector),
    .source_id = mln_test_view_of(source_id),
  };
  if (source_layer_id != NULL) {
    selector.fields |= MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
    selector.source_layer_id = mln_test_view_of(source_layer_id);
  }
  if (feature_id != NULL) {
    selector.fields |= MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
    selector.feature_id = mln_test_view_of(feature_id);
  }
  return selector;
}

static void set_state(
  mln_map map, mln_feature_state_selector selector, const char* state
) {
  MLN_TEST_AWAIT_OK(mln_map_set_feature_state(
    map, &selector, mln_test_view_of(state), &completion.descriptor,
    MLN_TEST_DIAGNOSTIC
  ));
}

static void remove_state(mln_map map, mln_feature_state_selector selector) {
  MLN_TEST_AWAIT_OK(mln_map_remove_feature_state(
    map, &selector, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
}

// The state object the map store holds for one feature, as JSON.
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

// Sets merge keys into what a feature holds, and removes take one key, one
// feature, or everything under a source layer, touching nothing else.
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
  hover.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  hover.state_key = MLN_BUFFER_LITERAL("hover");
  remove_state(map, hover);
  EXPECT_STATE("{\"count\":4}", map, feature);
  // Removing the last key leaves nothing behind, so a later set starts fresh.
  mln_feature_state_selector count = hover;
  count.state_key = MLN_BUFFER_LITERAL("count");
  remove_state(map, count);
  EXPECT_STATE("{}", map, feature);
  set_state(map, feature, "{\"label\":\"a\"}");
  EXPECT_STATE("{\"label\":\"a\"}", map, feature);
  remove_state(map, feature);
  EXPECT_STATE("{}", map, feature);

  // A source layer is its own namespace, and a remove without a feature clears
  // one namespace of one source.
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

  remove_state(map, selector_for("tiles", "roads", NULL));
  EXPECT_STATE("{}", map, roads_one);
  EXPECT_STATE("{}", map, roads_two);
  EXPECT_STATE("{\"n\":3}", map, water_one);
  EXPECT_STATE("{\"n\":4}", map, bare_one);

  remove_state(map, selector_for("tiles", NULL, NULL));
  EXPECT_STATE("{}", map, bare_one);
  EXPECT_STATE("{\"n\":3}", map, water_one);
  EXPECT_STATE("{\"n\":5}", map, other_one);

  // Removing what is not there commits and changes nothing.
  remove_state(map, selector_for("missing", NULL, NULL));
  remove_state(map, hover);
  EXPECT_STATE("{\"n\":5}", map, other_one);

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
static void null_source_layer_bytes(void* descriptor) {
  feature_state_call* call = descriptor;
  call->selector.fields |= MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
  call->selector.source_layer_id = (mln_buffer_view){.data = NULL, .size = 4};
}
static void no_feature_id(void* descriptor) {
  ((feature_state_call*)descriptor)->selector.fields &=
    ~(uint32_t)MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
}
static void get_without_feature_id(void* descriptor) {
  no_feature_id(descriptor);
  ((feature_state_call*)descriptor)->kind = FEATURE_STATE_GET;
}
static void remove_key_without_feature_id(void* descriptor) {
  feature_state_call* call = descriptor;
  no_feature_id(descriptor);
  call->kind = FEATURE_STATE_REMOVE;
  call->selector.fields |= MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
  call->selector.state_key = MLN_BUFFER_LITERAL("hover");
}
static void remove_everything_under_a_source(void* descriptor) {
  no_feature_id(descriptor);
  ((feature_state_call*)descriptor)->kind = FEATURE_STATE_REMOVE;
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

// Every selector and state check runs before the call returns, so a rejected
// call never reaches the map worker.
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
    {"null source_layer_id bytes", null_source_layer_bytes,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"set without feature_id", no_feature_id, MLN_STATUS_INVALID_ARGUMENT,
     "requires feature_id"},
    {"get without feature_id", get_without_feature_id,
     MLN_STATUS_INVALID_ARGUMENT, "requires feature_id"},
    {"remove state_key without feature_id", remove_key_without_feature_id,
     MLN_STATUS_INVALID_ARGUMENT, "state_key requires feature_id"},
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

static const char point_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
  "\"id\":\"feature-1\",\"properties\":{},\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]}}]}}},\"layers\":[{\"id\":\"circle\",\"type\":"
  "\"circle\",\"source\":\"points\",\"paint\":{\"circle-radius\":8}}]}";

typedef struct rendered_query {
  const mln_test_render_fixture* fixture;
  mln_test_feature_list list;
} rendered_query;

static bool feature_rendered(void* context) {
  rendered_query* query = context;
  query->list = mln_test_style_query_rendered(query->fixture);
  return query->list.status == MLN_STATUS_OK && query->list.count == 1;
}

// A render session pushes the map store into the renderer on its next render
// update, including a source's first, so a rendered query reports state as of
// the last frame rather than the last command.
static void rendered_features_carry_state_from_the_last_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(point_style_json)
  );
  const mln_feature_state_selector feature =
    selector_for("points", NULL, "feature-1");
  // Set before the source ever renders, so its first frame carries the state.
  set_state(map, feature, "{\"hover\":true}");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
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

  // A removal reaches the renderer the same way, leaving no state at all.
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

MLN_TEST_GROUP {
  RUN_TEST(state_merges_on_set_and_prunes_on_remove);
  RUN_TEST(malformed_selectors_are_rejected_at_submission);
  RUN_TEST(rendered_features_carry_state_from_the_last_frame);
}
