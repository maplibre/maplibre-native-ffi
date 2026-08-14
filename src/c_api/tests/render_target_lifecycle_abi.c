// Raw C ABI coverage: what a render session keeps when its target changes.
//
// The session renderer is not reachable through the C API, so these tests probe
// for it with mln_render_session_dump_debug_logs(), which reports
// MLN_STATUS_INVALID_STATE while no renderer exists and MLN_STATUS_OK once one
// does. It is the one renderer-requiring entry point that leaves the renderer's
// state intact.

#include <stdbool.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// Renders until the session reports a frame, which is when it builds its
// renderer. Returns whether a frame arrived before the attempts ran out.
static bool render_until_frame(
  mln_runtime runtime, mln_render_session session
) {
  for (unsigned int attempt = 0; attempt < 200; attempt += 1) {
    mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
    bool needs_repaint = false;
    if (
      mln_render_session_render_update(session, &result, &needs_repaint) !=
      MLN_STATUS_OK
    ) {
      return false;
    }
    if (result == MLN_RENDER_RESULT_RENDERED) {
      return true;
    }
    if (mln_runtime_pump(runtime, 0) != MLN_STATUS_OK) {
      return false;
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// Whether the session currently holds a renderer. A backend failure reports
// NATIVE_ERROR, so it cannot read as a retired renderer.
static bool has_renderer(mln_render_session session) {
  const mln_status status = mln_render_session_dump_debug_logs(session);
  if (status == MLN_STATUS_INVALID_STATE) {
    return false;
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, status);
  return true;
}

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

// The pixel ratio is baked into the renderer's shaders when it is built, so a
// scale factor change is the one resize that retires it. The replacement is
// built lazily on the next render.
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

// The descriptors here are defaults no backend would accept, so this test only
// reaches the session check because validation looks at the session first.
static void set_target_rejects_a_session_owned_texture(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  const mln_metal_borrowed_texture_descriptor metal =
    mln_metal_borrowed_texture_descriptor_default();
  const mln_vulkan_borrowed_texture_descriptor vulkan =
    mln_vulkan_borrowed_texture_descriptor_default();
  const mln_opengl_borrowed_texture_descriptor opengl =
    mln_opengl_borrowed_texture_descriptor_default();
  const mln_webgpu_borrowed_texture_descriptor webgpu =
    mln_webgpu_borrowed_texture_descriptor_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_metal_borrowed_texture_set_target(fixture.session, &metal)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_vulkan_borrowed_texture_set_target(fixture.session, &vulkan)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_opengl_borrowed_texture_set_target(fixture.session, &opengl)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_webgpu_borrowed_texture_set_target(fixture.session, &webgpu)
  );

  const mln_metal_surface_descriptor metal_surface =
    mln_metal_surface_descriptor_default();
  const mln_vulkan_surface_descriptor vulkan_surface =
    mln_vulkan_surface_descriptor_default();
  const mln_opengl_surface_descriptor opengl_surface =
    mln_opengl_surface_descriptor_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_metal_surface_set_target(fixture.session, &metal_surface)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_vulkan_surface_set_target(fixture.session, &vulkan_surface)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_opengl_surface_set_target(fixture.session, &opengl_surface)
  );

  // Every rejection left the session usable.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(fixture.session, 32, 32, 1.0)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A stale session is reported as invalid whatever the descriptor says, so a
// host can tell a lost session from a build without that backend.
static void set_target_rejects_a_stale_session(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const mln_render_session stale_session = fixture.session;
  mln_test_render_fixture_destroy(&fixture);

  const mln_metal_surface_descriptor metal_surface =
    mln_metal_surface_descriptor_default();
  const mln_opengl_borrowed_texture_descriptor opengl_texture =
    mln_opengl_borrowed_texture_descriptor_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_surface_set_target(stale_session, &metal_surface)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_set_target(stale_session, &opengl_texture)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_surface_set_target(MLN_HANDLE_NULL, &metal_surface)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_render_target_lifecycle_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(resize_keeps_the_session_renderer);
  RUN_TEST(scale_factor_change_rebuilds_the_session_renderer);
  RUN_TEST(set_target_rejects_a_session_owned_texture);
  RUN_TEST(set_target_rejects_a_stale_session);
}
