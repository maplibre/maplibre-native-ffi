// Raw C ABI coverage: what a render session keeps when its target changes.
//
// The session renderer is not reachable through the C API, so these tests probe
// it through mln_render_session_clear_data(), which reports
// MLN_STATUS_INVALID_STATE while no renderer exists and MLN_STATUS_OK once one
// does. That makes "the renderer survived" and "the renderer was rebuilt"
// directly observable, which is the whole contract under test: a surviving
// renderer is what carries the tile pyramid, atlases, symbol placement, and
// feature state across a resize instead of rebuilding them cold.

#include <stdbool.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// Renders until the session reports a frame, which is when it builds its
// renderer. Returns whether one was rendered before the attempts ran out.
static bool render_until_frame(
  mln_runtime runtime, mln_render_session session
) {
  for (unsigned int attempt = 0; attempt < 200; attempt += 1) {
    bool rendered = false;
    if (mln_render_session_render_update(session, &rendered) != MLN_STATUS_OK) {
      return false;
    }
    if (rendered) {
      return true;
    }
    if (mln_runtime_pump(runtime, 0) != MLN_STATUS_OK) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// Whether the session currently holds a renderer.
static bool has_renderer(mln_render_session session) {
  return mln_render_session_clear_data(session) == MLN_STATUS_OK;
}

// A resize changes the size of the target and nothing the renderer caches
// against, so the renderer carries over.
static void resize_keeps_the_session_renderer(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  TEST_ASSERT_TRUE(render_until_frame(runtime, fixture.session));
  TEST_ASSERT_TRUE_MESSAGE(
    has_renderer(fixture.session), "a rendered frame should leave a renderer"
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(fixture.session, 96, 48, 1.0)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    has_renderer(fixture.session),
    "resizing at the same scale factor should keep the renderer"
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The pixel ratio is fixed when a renderer is built and baked into its shaders,
// so a scale factor change is the one resize that starts over. The replacement
// is built lazily on the next render.
static void scale_factor_change_rebuilds_the_session_renderer(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  TEST_ASSERT_TRUE(render_until_frame(runtime, fixture.session));
  TEST_ASSERT_TRUE(has_renderer(fixture.session));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(fixture.session, 64, 64, 2.0)
  );
  TEST_ASSERT_FALSE_MESSAGE(
    has_renderer(fixture.session),
    "changing the scale factor should retire the renderer"
  );

  TEST_ASSERT_TRUE(render_until_frame(runtime, fixture.session));
  TEST_ASSERT_TRUE_MESSAGE(
    has_renderer(fixture.session),
    "the next render should build a renderer for the new scale factor"
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A session-owned texture is allocated and sized by its session, so following a
// host resize means resizing rather than handing over a new target. The
// caller-owned entry points say so instead of quietly doing nothing.
static void set_target_rejects_a_session_owned_texture(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_metal_borrowed_texture_descriptor metal =
    mln_metal_borrowed_texture_descriptor_default();
  metal.extent.width = 64;
  metal.extent.height = 64;
  metal.physical_width = 64;
  metal.physical_height = 64;
  TEST_ASSERT_NOT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_metal_borrowed_texture_set_target(fixture.session, &metal)
  );

  mln_vulkan_borrowed_texture_descriptor vulkan =
    mln_vulkan_borrowed_texture_descriptor_default();
  vulkan.extent.width = 64;
  vulkan.extent.height = 64;
  vulkan.physical_width = 64;
  vulkan.physical_height = 64;
  TEST_ASSERT_NOT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_vulkan_borrowed_texture_set_target(fixture.session, &vulkan)
  );

  mln_opengl_borrowed_texture_descriptor opengl =
    mln_opengl_borrowed_texture_descriptor_default();
  opengl.extent.width = 64;
  opengl.extent.height = 64;
  opengl.physical_width = 64;
  opengl.physical_height = 64;
  TEST_ASSERT_NOT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_opengl_borrowed_texture_set_target(fixture.session, &opengl)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Null handles and descriptors are rejected before any of this reaches a
// backend, so a host that loses track of a session gets a status rather than a
// crash.
static void set_target_rejects_null_raw_arguments(void) {
  mln_metal_surface_descriptor metal_surface =
    mln_metal_surface_descriptor_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_surface_set_target(MLN_HANDLE_NULL, NULL)
  );
  TEST_ASSERT_NOT_EQUAL_INT(
    MLN_STATUS_OK, mln_metal_surface_set_target(MLN_HANDLE_NULL, &metal_surface)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_surface_set_target(MLN_HANDLE_NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_surface_set_target(MLN_HANDLE_NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_borrowed_texture_set_target(MLN_HANDLE_NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_set_target(MLN_HANDLE_NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_set_target(MLN_HANDLE_NULL, NULL)
  );
}

void run_render_target_lifecycle_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(resize_keeps_the_session_renderer);
  RUN_TEST(scale_factor_change_rebuilds_the_session_renderer);
  RUN_TEST(set_target_rejects_a_session_owned_texture);
  RUN_TEST(set_target_rejects_null_raw_arguments);
}
