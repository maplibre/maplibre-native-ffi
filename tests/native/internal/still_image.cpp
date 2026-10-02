// A still image and the frames that may complete it. The map hears of a
// frame through a mailbox that hands the runtime worker one message per turn,
// so no public command can be ordered after a given frame's delivery; the
// StillImageFrameHeldBack point marks it.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"
#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

constexpr char green_background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"bg\","
  "\"type\":\"background\",\"paint\":{\"background-color\":\"#00ff00\"}}]}";

// Reads back the center pixel of the frame the session rendered last.
void read_center_pixel(
  const mln_test_render_fixture& fixture, std::uint8_t (&out_rgba)[4]
) {
  auto readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_texture_read_premultiplied_rgba8(
    fixture.session, &readback.descriptor, nullptr
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &readback));
  auto image = mln_texture_readback_result{};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &image, sizeof(image))
  );
  TEST_ASSERT_EQUAL_UINT32(64, image.info.width);
  TEST_ASSERT_EQUAL_UINT32(64, image.info.height);
  const auto* pixels = static_cast<const std::uint8_t*>(image.data.data);
  std::memcpy(out_rgba, pixels + (((32 * 64) + 32) * 4), 4);
  mln_test_completion_destroy(&readback);
}

// Renders one forced frame, which renders the session's latest update again,
// and releases it so the texture ring never fills.
void render_forced_frame(const mln_test_render_fixture& fixture) {
  mln_test_render_request_forced(&fixture, 1);
  const auto batch = mln_test_render_wait_for_results(&fixture, 1);
  const auto result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  auto frame = mln_acquired_frame{MLN_HANDLE_NULL};
  MLN_TEST_OK(
    mln_render_session_acquire_frame(fixture.session, &frame, nullptr)
  );
  const auto cpu_complete = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(&frame, &cpu_complete, nullptr));
}

// A still image completes only with a frame that rendered the map as its
// request found it. A forced demand renders the previous still image's update
// again while the request waits behind a parked runtime worker, so the map
// hears of that fully loaded frame just after the request. MapLibre would
// complete the image from it; the map reports it as partial instead, and the
// image waits for a frame of its own update, which shows the new style.
void a_frame_of_an_older_update_does_not_complete_a_still_image() {
  const auto runtime = mln_test_create_runtime();
  auto options = mln_map_options_default();
  options.initial_extent =
    mln_logical_extent{.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  const auto map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  // A static map publishes no update for the new style until a still image
  // requests one, so the session's latest update still shows red.
  mln_test_load_style_and_wait(
    runtime, map,
    mln_buffer_view{
      green_background_style_json, sizeof(green_background_style_json) - 1
    }
  );

  auto points = SyncPointScope{};
  // A task of the case's own, because a public completion can run on the
  // submitting thread instead when the command finishes first.
  auto parked = std::make_shared<mln_test_gate>();
  mln_test_gate_init(parked.get());
  mln::core::lease_runtime(runtime)->executor.invoke([parked] {
    mln_test_gate_park(parked.get());
  });
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(parked.get()), "the runtime worker never parked"
  );
  auto still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, nullptr));
  render_forced_frame(fixture);
  mln_test_gate_release(parked.get());

  // The request runs first, since it was queued before the frame's messages.
  TEST_ASSERT_TRUE(points.wait_for_hits(SyncPoint::StillImageFrameHeldBack, 1));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&still));

  MLN_TEST_OK(mln_test_render_pending_still_image(&fixture, &still));
  mln_test_completion_destroy(&still);
  std::uint8_t rgba[4] = {};
  read_center_pixel(fixture, rgba);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[1]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[2]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[3]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_frame_of_an_older_update_does_not_complete_a_still_image);
}
