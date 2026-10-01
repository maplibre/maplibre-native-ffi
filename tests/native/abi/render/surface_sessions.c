// Surface sessions from tests/graphics: a resize that the session applies to
// the host's surface, after which a frame renders without presenting, and, on
// Metal, a replacement surface at a new scale factor.
//
// The browser presets have no host surface, since JavaScript owns their
// canvases.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)

#include "support/host_graphics.h"

typedef struct surface_map {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture;
} surface_map;

static void open_surface_map(surface_map* out) {
  out->runtime = mln_test_create_runtime();
  out->map = mln_test_create_map(out->runtime);
  mln_test_render_prepare_map(out->runtime, out->map);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_surface(out->map, &out->fixture),
    mln_test_graphics_last_error()
  );
}

static void close_surface_map(surface_map* map) {
  mln_test_render_fixture_destroy(&map->fixture);
  mln_test_destroy_map(map->map);
  mln_test_destroy_runtime(map->runtime);
}

// Renders one forced frame, which carries no present flag, and returns its
// disposition.
static uint32_t render_without_presenting(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  mln_render_frame_batch_release(batch);
  return result.disposition;
}

static mln_render_session_snapshot read_snapshot(
  const mln_test_render_fixture* fixture
) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture->session, &snapshot, NULL)
  );
  return snapshot;
}

// A resize reaches the host's surface, so the next frame renders at the new
// extent.
static void a_surface_session_resizes_its_surface(void) {
  surface_map map = {0};
  open_surface_map(&map);
  const mln_render_target_extent extent = {
    .size = sizeof(mln_render_target_extent),
    .width = 48,
    .height = 32,
    .scale_factor = 1.0,
  };
  mln_test_completion resize = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_resize(
    map.fixture.session, &extent, &resize.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&map.fixture, &resize));
  mln_test_completion_destroy(&resize);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_without_presenting(&map.fixture, 2)
  );
  const mln_render_session_snapshot snapshot = read_snapshot(&map.fixture);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(32, snapshot.extent.height);
  close_surface_map(&map);
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)

// A replacement surface may carry a new scale factor. The renderer bakes the
// pixel ratio into its shaders, so the session rebuilds it and renders at the
// new ratio.
static void a_surface_replacement_can_change_the_scale_factor(void) {
  surface_map map = {0};
  open_surface_map(&map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_without_presenting(&map.fixture, 1)
  );

  mln_test_graphics* graphics = mln_test_render_fixture_graphics(&map.fixture);
  mln_test_graphics_surface* replacement =
    mln_test_render_fixture_new_surface(&map.fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(graphics, &context));
  mln_test_graphics_surface_info info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_surface_get_info(replacement, &info));
  mln_metal_surface_descriptor descriptor =
    mln_metal_surface_descriptor_default();
  descriptor.extent = (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = MLN_TEST_HOST_TARGET_SIZE,
    .height = MLN_TEST_HOST_TARGET_SIZE,
    .scale_factor = 2.0,
  };
  descriptor.context.device = context.metal_device;
  descriptor.layer = info.metal_layer;
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK_MESSAGE(
    mln_metal_surface_set_target(
      map.fixture.session, &descriptor, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    ),
    mln_test_last_error()
  );
  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&map.fixture, &completion)
  );
  mln_test_completion_destroy(&completion);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_without_presenting(&map.fixture, 2)
  );
  TEST_ASSERT_EQUAL_DOUBLE(
    2.0, read_snapshot(&map.fixture).extent.scale_factor
  );
  close_surface_map(&map);
}

#endif

#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_surface_session_resizes_its_surface);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  RUN_TEST(a_surface_replacement_can_change_the_scale_factor);
#endif
#endif
}
