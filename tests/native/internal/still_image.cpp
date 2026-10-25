// A still image and the frames that may complete it. The map hears of a
// frame through a mailbox that hands the runtime worker one message per turn,
// so no public command can be ordered after a given frame's delivery; the
// StillImageFrameHeldBack point marks it. The map can also complete an image
// before the session publishes the result of the frame that completed it,
// which the RenderFrameRendered point holds open.

#include <atomic>
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
#include "testing/sync_point.hpp"

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
  MLN_TEST_OK(mln_render_session_read_texture(
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
  const auto batch = mln_test_render_wait_for_results(&fixture);
  const auto result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  auto frame = mln_acquired_frame{MLN_HANDLE_NULL};
  MLN_TEST_OK(
    mln_render_session_acquire_frame(fixture.session, &frame, nullptr)
  );
  auto cpu_complete = mln_gpu_sync{};
  cpu_complete.size = sizeof(mln_gpu_sync);
  MLN_TEST_OK(mln_acquired_frame_release(frame, &cpu_complete, nullptr));
}

// A still image completes only with a frame that rendered the map as its
// request found it. A forced demand renders the previous still image's update
// again while the request waits behind a parked runtime worker, so the map
// hears of that fully loaded frame just after the request. MapLibre would
// complete the image from it; the map reports it as partial instead, and the
// image waits for a frame of its own update, which shows the new style.
void a_frame_of_an_older_update_does_not_complete_a_still_image() {
  const auto runtime = mln_test_create_runtime();
  auto options = mln_test_map_options();
  options.initial_extent =
    mln_logical_extent{.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  const auto map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  // A static map publishes no update for the new style until a still image
  // requests one, so the session's latest update still shows red.
  mln_test_load_style_and_wait(
    map, mln_buffer_view{
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
  // The request's own demand can render the image as soon as the request
  // runs, so the held point keeps that frame's messages behind this check.
  points.hold(SyncPoint::StillImageFrameHeldBack);
  mln_test_gate_release(parked.get());

  // The request runs first, since it was queued before the frame's messages.
  TEST_ASSERT_TRUE(points.wait_for_hits(SyncPoint::StillImageFrameHeldBack, 1));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&still));
  points.release(SyncPoint::StillImageFrameHeldBack);

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

struct NextRequest {
  mln_map map;
  mln_test_completion* first;
  mln_test_completion* second;
  std::atomic_bool requested{false};
  bool first_completed = false;
  mln_status second_submitted = MLN_STATUS_NATIVE_ERROR;
  mln_status fence = MLN_STATUS_NATIVE_ERROR;
};

// Runs on the driver once the first image's frame has rendered and before its
// result is published. It waits for the map to complete that image, then
// requests the next one and waits until the request has run, so the next
// request arrives while the first one's demand is still outstanding. A case
// cannot assert off its own thread, so this records what it saw.
void request_the_next_image(SyncPoint point, void* context) noexcept {
  if (point != SyncPoint::RenderFrameRendered) return;
  auto& next = *static_cast<NextRequest*>(context);
  if (next.requested.exchange(true)) return;
  next.first_completed = mln_test_completion_wait(next.first, -1);
  next.second_submitted =
    mln_map_request_still_image(next.map, &next.second->descriptor, nullptr);
  // A read behind the request completes once the request has run.
  auto camera = mln_test_completion_default(sizeof(mln_camera_query_result));
  if (
    mln_map_get_camera(next.map, &camera.descriptor, nullptr) != MLN_STATUS_OK
  ) {
    camera.descriptor.release_user_data(camera.descriptor.user_data);
    mln_test_completion_destroy(&camera);
    return;
  }
  auto value = mln_camera_query_result{};
  next.fence = mln_test_completion_finish_value(&camera, &value, sizeof(value));
}

// A request that arrives while the previous request's demand is still
// outstanding, because the map completed that image before the session
// published the frame's result, gets a demand of its own and renders.
void a_request_behind_an_unfinished_demand_still_renders() {
  const auto runtime = mln_test_create_runtime();
  auto options = mln_test_map_options();
  options.initial_extent =
    mln_logical_extent{.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  const auto map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  auto first = mln_test_completion_default(0);
  auto second = mln_test_completion_default(0);
  auto next = NextRequest{.map = map, .first = &first, .second = &second};
  mln::testing::set_sync_point_handler(request_the_next_image, &next);
  MLN_TEST_OK(mln_map_request_still_image(map, &first.descriptor, nullptr));
  const auto status = mln_test_render_pending_still_image(&fixture, &first);
  const auto requested = next.requested.load();
  mln::testing::set_sync_point_handler(nullptr, nullptr);
  MLN_TEST_OK(status);
  TEST_ASSERT_TRUE(requested);
  TEST_ASSERT_TRUE(next.first_completed);
  MLN_TEST_OK(next.second_submitted);
  MLN_TEST_OK(next.fence);

  // A caller driver renders the next image within the same service, so the
  // wait above can drain its frame's result; this waits for its completion.
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &second));
  mln_test_completion_destroy(&first);
  mln_test_completion_destroy(&second);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_frame_of_an_older_update_does_not_complete_a_still_image);
  RUN_TEST(a_request_behind_an_unfinished_demand_still_renders);
}
