#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "delivery/delivery_queue.hpp"
#include "internal/support/allocation_faults.hpp"
#include "support/test_support.h"
#include "wake/wake.hpp"

namespace {

// What one dispatched event call saw, copied during the call.
struct SeenEvents {
  std::size_t calls = 0;
  std::size_t count = 0;
  std::string first_message;
  std::string sprite_id;
  std::string sprite_url;
  std::vector<std::string> last_fonts;
};

auto text(mln_buffer_view view) -> std::string {
  return {static_cast<const char*>(view.data), view.size};
}

void see_events(void* user_data, const mln_event_batch_view* events) {
  auto& seen = *static_cast<SeenEvents*>(user_data);
  seen.calls += 1;
  seen.count = events->event_count;
  if (events->event_count == 0) return;
  const auto& first = events->events[0];
  seen.first_message = text(first.message);
  if (first.payload_type == MLN_RUNTIME_EVENT_PAYLOAD_SPRITE) {
    seen.sprite_id = text(first.payload.sprite.id);
    seen.sprite_url = text(first.payload.sprite.url);
  }
  const auto& last = events->events[events->event_count - 1];
  seen.last_fonts.clear();
  if (last.payload_type == MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS) {
    for (std::size_t index = 0; index < last.payload.glyphs.font_count;
         ++index) {
      seen.last_fonts.push_back(text(last.payload.glyphs.font_stack[index]));
    }
  }
}

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
  auto seen = SeenEvents{};
  queue.push_event(event, "first", &seen, see_events);
  const auto message = std::string(65536, 'x');
  auto threw = false;
  {
    const auto faults = mln::native_tests::AllocationFaults{};
    try {
      queue.push_event(event, message, &seen, see_events);
    } catch (const std::bad_alloc&) {
      threw = true;
    }
  }
  TEST_ASSERT_TRUE(queue.dispatch() == mln::delivery::DispatchResult::Ran);
  TEST_ASSERT_TRUE(threw);
  TEST_ASSERT_EQUAL_size_t(1, seen.calls);
  TEST_ASSERT_EQUAL_size_t(1, seen.count);
  TEST_ASSERT_EQUAL_STRING("first", seen.first_message.c_str());
  queue.push_event(event, "later", &seen, see_events);
  TEST_ASSERT_EQUAL_INT(2, wakes.load());
}

// Later appends grow the item's storage, which must not move the text that an
// earlier event's payload points at.
void queued_payload_text_outlives_later_appends() {
  auto queue = mln::delivery::DeliveryQueueState{nullptr};
  auto seen = SeenEvents{};
  auto sprite = mln_runtime_event{
    .type = MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_SPRITE,
  };
  queue.push_event(
    sprite, "failed", &seen, see_events, {"poi", "test://sprites/poi"}
  );
  auto glyphs = mln_runtime_event{
    .type = MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS,
  };
  for (auto index = 0; index < 64; ++index) {
    queue.push_event(
      glyphs, {}, &seen, see_events, {"Noto Sans Regular", "Arial"}
    );
  }
  TEST_ASSERT_TRUE(queue.dispatch() == mln::delivery::DispatchResult::Ran);
  TEST_ASSERT_EQUAL_size_t(1, seen.calls);
  TEST_ASSERT_EQUAL_size_t(65, seen.count);
  TEST_ASSERT_EQUAL_STRING("failed", seen.first_message.c_str());
  TEST_ASSERT_EQUAL_STRING("poi", seen.sprite_id.c_str());
  TEST_ASSERT_EQUAL_STRING("test://sprites/poi", seen.sprite_url.c_str());
  TEST_ASSERT_EQUAL_size_t(2, seen.last_fonts.size());
  TEST_ASSERT_EQUAL_STRING("Noto Sans Regular", seen.last_fonts[0].c_str());
  TEST_ASSERT_EQUAL_STRING("Arial", seen.last_fonts[1].c_str());
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_failed_event_append_keeps_the_published_events_readable);
  RUN_TEST(queued_payload_text_outlives_later_appends);
}
