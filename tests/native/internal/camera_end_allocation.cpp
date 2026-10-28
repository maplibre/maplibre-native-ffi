// A queued call whose producer cannot allocate still reaches its queue.

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <mln/map/map.hpp>

#include "delivery/delivery_queue.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"
#include "support/allocation_faults.hpp"
#include "support/delivery.h"
#include "support/test_support.h"

namespace {

void record_end(void* user_data, const mln_camera_transition_end* end) {
  mln_test_record_call('e', user_data, MLN_STATUS_OK, end->outcome);
}

// End an accepted transition on the worker with allocations disabled. The
// end call must precede the release even when its producer cannot allocate.
void a_camera_end_reaches_its_queue_without_allocating() {
  auto queue = mln_test_delivery_queue{};
  mln_test_delivery_queue_create(&queue);
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  auto camera = mln_camera_update{};
  camera.size = sizeof(mln_camera_update);
  camera.camera.size = sizeof(mln_camera_options);
  camera.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  camera.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.camera.zoom = 6;
  camera.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  camera.animation.duration_ms = 60000;
  camera.animation.end_handler.callback = record_end;
  camera.animation.end_handler.release_user_data = mln_test_record_release;
  camera.animation.end_handler.queue = queue.handle;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &camera, &completion.descriptor, nullptr)
  );
  auto live = mln::core::handle_table<mln::core::MapObject>().lease(map);
  auto ended = std::atomic_bool{false};
  live->runtime_state->executor.invoke([live, &ended] {
    {
      const auto faults = mln::native_tests::AllocationFaults{};
      try {
        live->map->cancelTransitions();
      } catch (const std::bad_alloc&) {
        // MapLibre may allocate for the update after the transition ends.
      }
    }
    ended.store(true);
    mln_test_pulse();
  });
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&ended));

  mln_test_calls_reset();
  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, nullptr));
  TEST_ASSERT_EQUAL_size_t(2, mln_test_calls.count);
  TEST_ASSERT_EQUAL_CHAR('e', mln_test_call_at(0)->kind);
  TEST_ASSERT_EQUAL_UINT64(
    MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, mln_test_call_at(0)->generation
  );
  TEST_ASSERT_EQUAL_CHAR('r', mln_test_call_at(1)->kind);
  live.reset();
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(a_camera_end_reaches_its_queue_without_allocating); }
