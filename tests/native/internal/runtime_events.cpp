#include <atomic>
#include <memory>
#include <string>

#include "delivery/delivery_queue.hpp"
#include "internal/support/allocation_faults.hpp"
#include "support/test_support.h"
#include "wake/wake.hpp"

namespace {

void a_failed_event_append_keeps_the_published_events_readable() {
  auto wakes = std::atomic_int{0};
  auto wake = std::make_shared<mln::core::Wake>(
    mln_wake{
      .callback = [](
                    void* context
                  ) { static_cast<std::atomic_int*>(context)->fetch_add(1); },
      .user_data = &wakes,
    },
    nullptr
  );
  wake->accept();
  auto queue = mln::delivery::DeliveryQueueState{wake};
  const auto event =
    mln_runtime_event{.type = MLN_RUNTIME_EVENT_MAP_LOADING_FAILED};
  queue.push_event(event, "first", nullptr);
  const auto message = std::string(65536, 'x');
  auto threw = false;
  {
    const auto faults = mln::native_tests::AllocationFaults{};
    try {
      queue.push_event(event, message, nullptr);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
  }
  auto view = mln_delivery_batch_view{};
  const auto batch = queue.drain(view);
  const auto& events = *view.deliveries[0].payload.runtime_events.events;
  TEST_ASSERT_TRUE(threw);
  TEST_ASSERT_EQUAL_size_t(1, events.event_count);
  TEST_ASSERT_EQUAL_STRING("first", events.events[0].message.data);
  mln::delivery::release_delivery_batch(batch);
  queue.push_event(event, "later", nullptr);
  TEST_ASSERT_EQUAL_INT(2, wakes.load());
}

// Later appends grow the item's storage, which must not move the text that an
// earlier event's payload points at.
void queued_payload_text_outlives_later_appends() {
  auto queue = mln::delivery::DeliveryQueueState{nullptr};
  auto sprite = mln_runtime_event{
    .type = MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_SPRITE,
  };
  queue.push_event(sprite, "failed", nullptr, {"poi", "test://sprites/poi"});
  auto glyphs = mln_runtime_event{
    .type = MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS,
  };
  for (auto index = 0; index < 64; ++index) {
    queue.push_event(glyphs, {}, nullptr, {"Noto Sans Regular", "Arial"});
  }
  auto view = mln_delivery_batch_view{};
  const auto batch = queue.drain(view);
  const auto& events = *view.deliveries[0].payload.runtime_events.events;
  TEST_ASSERT_EQUAL_size_t(65, events.event_count);
  const auto& first = events.events[0];
  TEST_ASSERT_EQUAL_STRING("failed", first.message.data);
  TEST_ASSERT_EQUAL_STRING("poi", first.payload.sprite.id.data);
  TEST_ASSERT_EQUAL_STRING("test://sprites/poi", first.payload.sprite.url.data);
  const auto& last = events.events[64].payload.glyphs;
  TEST_ASSERT_EQUAL_size_t(2, last.font_count);
  TEST_ASSERT_EQUAL_STRING("Noto Sans Regular", last.font_stack[0].data);
  TEST_ASSERT_EQUAL_STRING("Arial", last.font_stack[1].data);
  mln::delivery::release_delivery_batch(batch);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_failed_event_append_keeps_the_published_events_readable);
  RUN_TEST(queued_payload_text_outlives_later_appends);
}
