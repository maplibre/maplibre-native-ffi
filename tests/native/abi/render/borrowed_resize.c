// A caller-owned texture is sized by its owner, so its session refuses a
// resize. The browser presets have no borrowed texture fixture.

#include <string.h>

#include "support/harness.h"
#include "support/host_graphics.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)
// The refusal names the handover call that replaces the texture instead.
static void a_borrowed_texture_session_refuses_a_resize(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture)
  );
  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = MLN_TEST_HOST_TARGET_SIZE / 2,
    .height = MLN_TEST_HOST_TARGET_SIZE / 2,
    .scale_factor = 1.0,
  };
  mln_test_completion completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_render_session_resize(
      fixture.session, &extent, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "sized by its owner"), mln_test_last_error()
  );
  mln_test_completion_reject(&completion);
  mln_test_completion_destroy(&completion);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_borrowed_texture_session_refuses_a_resize);
#endif
}
