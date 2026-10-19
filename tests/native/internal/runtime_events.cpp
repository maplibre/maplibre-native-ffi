#include <atomic>
#include <memory>
#include <vector>

#include "internal/support/allocation_faults.hpp"
#include "map/map_internal.hpp"
#include "runtime/runtime.hpp"
#include "support/test_support.h"
#include "wake/wake.hpp"

namespace {
using mln::native_tests::AllocationFaults;

void partial_event_publication_still_wakes() {
  auto wakes = std::atomic_int{0};
  auto descriptor = mln_wake{
    .callback = [](
                  void* context
                ) { static_cast<std::atomic_int*>(context)->fetch_add(1); },
    .user_data = &wakes,
  };
  auto queue = std::make_shared<mln::core::RuntimeEventQueueState>();
  queue->wake = std::make_shared<mln::core::Wake>(descriptor);
  queue->wake->accept();
  constexpr auto map = mln_map{42};
  queue->event_maps.emplace(map, std::make_shared<mln::core::MapEventState>());
  queue->pending.events.reserve(1);
  auto events = std::vector<mln::core::HeldMapEvent>(2);
  events[0].type = MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE;
  events[1].type = MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE;
  auto threw = false;
  {
    const auto faults = AllocationFaults{};
    try {
      mln::core::push_runtime_map_events(queue, map, 1, events);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
  }
  const auto partially_published = queue->pending.events.size();
  mln::core::push_runtime_map_event_payload(
    queue, map, 2, MLN_RUNTIME_EVENT_MAP_IDLE, MLN_RUNTIME_EVENT_PAYLOAD_NONE,
    mln::core::zeroed_event_payload()
  );
  TEST_ASSERT_TRUE(threw);
  TEST_ASSERT_EQUAL_UINT(1, partially_published);
  TEST_ASSERT_GREATER_THAN_INT_MESSAGE(
    0, wakes.load(), "queued events lost their empty-to-nonempty wake"
  );
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(partial_event_publication_still_wakes); }
