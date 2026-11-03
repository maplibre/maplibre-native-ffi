// An offscreen session that a browser WebGPU build creates its own device for.

#include "internal/support/driver_blocker.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"
#include "support/frames.h"
#include "support/map.h"
#include "support/status.h"
#include "support/test_support.h"
#include "support/wait.h"
#include "unity.h"

namespace {

using mln::native_tests::DriverBlocker;

void attach_without_a_device(mln_map map, mln_test_render_fixture& fixture) {
  fixture.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  auto descriptor = mln_offscreen_descriptor{};
  descriptor.size = sizeof(descriptor);
  descriptor.logical_size = {.width = 64, .height = 64};
  auto options = mln_render_session_attach_options{};
  options.size = sizeof(options);
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  auto attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach_offscreen(
    map, &descriptor, &options, MLN_HANDLE_NULL, &fixture.session,
    &attach.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
}

// Disposes a session while a readback's buffer map is pending. The map
// settles only on the worker's event loop, and disposal reaches the worker
// first, which then destroys the device and ends.
void dispose_with_a_readback_pending(mln_runtime runtime) {
  auto map_options = mln_test_map_options();
  map_options.initial_extent = {.width = 64, .height = 64, .scale_factor = 1.0};
  map_options.map_mode = MLN_MAP_MODE_STATIC;
  const auto map = mln_test_create_map_with_options(runtime, &map_options);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  auto fixture = mln_test_render_fixture{};
  attach_without_a_device(map, fixture);
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  // Starts the readback, then parks the worker before its event loop runs.
  auto blocker = DriverBlocker{};
  mln_test_gate_init(blocker.gate.get());
  blocker.completion = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln::core::enqueue_driver_operation(
      fixture.session,
      [gate = blocker.gate](mln_render_session_object& session) {
        const auto readback = session.texture.backend->start_readback({});
        mln_test_gate_park(gate.get());
        return readback != nullptr ? MLN_STATUS_OK : MLN_STATUS_NATIVE_ERROR;
      },
      &blocker.completion.descriptor,
      mln::core::valueless_completion<&mln_render_session_reduce_memory_use>()
    )
  );
  blocker.submitted = true;
  TEST_ASSERT_TRUE(mln_test_gate_wait_entered(blocker.gate.get()));
  auto disposed = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_release(
    fixture.session, &disposed.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  mln_test_gate_release(blocker.gate.get());
  MLN_TEST_OK(mln_test_completion_settle(&blocker.completion));
  mln_test_completion_destroy(&blocker.completion);
  blocker.submitted = false;
  MLN_TEST_OK(mln_test_completion_settle(&disposed));
  mln_test_completion_destroy(&disposed);
  mln_test_destroy_map(map);
}

// A worker that ended with the map still pending left the next runtime on
// the page unable to release, so the case releases two.
void disposal_with_a_readback_pending_ends_the_worker() {
  for (auto round = 0; round < 2; round += 1) {
    const auto runtime = mln_test_create_runtime();
    dispose_with_a_readback_pending(runtime);
    mln_test_destroy_runtime(runtime);
  }
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(disposal_with_a_readback_pending_ends_the_worker); }
