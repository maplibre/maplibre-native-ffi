// A session before and after its first frame: a readback has nothing to read
// until a frame renders, and a source query with no options reads every
// feature of the source.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char point_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":{"
  "\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},\"properties\":{"
  "\"name\":\"only\"}}]}}},\"layers\":[{\"id\":\"dots\",\"type\":\"circle\","
  "\"source\":\"points\"}]}";

static mln_status read_back(const mln_test_render_fixture* fixture) {
  mln_test_completion readback = mln_test_completion_readback();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                     fixture->session, &readback.descriptor, NULL
                   )
  );
  const mln_status status =
    mln_test_render_fixture_finish_operation(fixture, &readback);
  mln_test_completion_destroy(&readback);
  return status;
}

static bool the_source_holds_its_feature(void* context) {
  const mln_test_render_fixture* fixture = context;
  const mln_test_feature_list features =
    mln_test_style_query_source_with(fixture, "points", NULL);
  return features.status == MLN_STATUS_OK && features.count == 1 &&
         strstr(features.features[0].feature, "\"name\":\"only\"") != NULL;
}

static void a_readback_needs_a_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_view_of(point_style_json)
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_STATE, read_back(&fixture));
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, the_source_holds_its_feature, &fixture,
    "the source to hold its feature"
  ));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, read_back(&fixture));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(a_readback_needs_a_rendered_frame); }
