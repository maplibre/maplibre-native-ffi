// A render session at the edges of its life: the map it holds refuses to
// release, a resize needs an attached session whose target the session sizes,
// a readback needs a rendered frame, and a source query takes no options.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)
#include "support/host_graphics.h"
#endif

static void detach_fixture(mln_test_render_fixture* fixture) {
  mln_test_completion detach = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_detach(fixture->session, &detach.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(fixture, &detach)
  );
  mln_test_completion_destroy(&detach);
}

// A map with an attached session refuses its release and stays live, and
// releases once the session detaches.
static void a_map_release_waits_for_its_session_to_detach(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_map_release(map, &discard, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_STRING(
    "map still has an attached render session", mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_request_repaint(map));

  detach_fixture(&fixture);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status submit_resize(mln_render_session session) {
  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 48,
    .height = 48,
    .scale_factor = 1.0,
  };
  mln_completion discard = mln_test_discard_completion();
  return mln_render_session_resize(
    session, &extent, &discard, MLN_TEST_DIAGNOSTIC
  );
}

// A detached session has no target to resize, and a caller-owned texture is
// sized by the host, which hands over a replacement instead.
static void resize_needs_an_attached_session_that_sizes_its_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  detach_fixture(&fixture);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, submit_resize(fixture.session)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "not attached"));
  mln_test_render_fixture_destroy(&fixture);

#if !defined(__EMSCRIPTEN__)
  mln_test_render_fixture borrowed = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_texture(map, &borrowed),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED, submit_resize(borrowed.session)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "sized by its owner"));
  mln_test_render_fixture_destroy(&borrowed);
#endif
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A readback is accepted before any frame renders, and its completion reports
// that no frame is available.
static void a_readback_before_any_frame_fails_at_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_completion readback = mln_test_completion_readback();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                     fixture.session, &readback.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_test_render_fixture_finish_operation(&fixture, &readback)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_completion_diagnostic(&readback), "no rendered frame"),
    mln_test_completion_diagnostic(&readback)
  );
  mln_test_completion_destroy(&readback);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static const char point_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
  "\"properties\":{\"name\":\"origin\"},\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]}}]}}},\"layers\":[{\"id\":\"circles\",\"type\":"
  "\"circle\",\"source\":\"points\",\"paint\":{\"circle-radius\":4}}]}";

typedef struct source_wait {
  const mln_test_render_fixture* fixture;
  mln_test_feature_list list;
} source_wait;

static bool source_holds_the_point(void* context) {
  source_wait* wait = context;
  wait->list = mln_test_style_query_source_with(wait->fixture, "points", NULL);
  return wait->list.status == MLN_STATUS_OK && wait->list.count == 1;
}

// A source query with no options reads every feature the source holds.
static void a_source_query_without_options_reads_every_feature(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_view_of(point_style_json)
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  source_wait wait = {.fixture = &fixture};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, source_holds_the_point, &wait, "the source's point to load"
  ));
  TEST_ASSERT_NOT_NULL(
    strstr(wait.list.features[0].feature, "\"name\":\"origin\"")
  );
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_map_release_waits_for_its_session_to_detach);
  RUN_TEST(resize_needs_an_attached_session_that_sizes_its_target);
  RUN_TEST(a_readback_before_any_frame_fails_at_completion);
  RUN_TEST(a_source_query_without_options_reads_every_feature);
}
