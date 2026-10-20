// Recovery after a delivery drain that cannot allocate its batch.

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c.h"
#include "support/allocation_faults.hpp"
#include "support/test_support.h"

namespace {

// Counts a queue's wakes, which arrive on native threads.
struct CountingWake {
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  std::atomic_int wakes = 0;
  std::atomic_bool released = false;
};

void count_wake(void* user_data) noexcept {
  static_cast<CountingWake*>(user_data)->wakes.fetch_add(1);
  mln_test_pulse();
}

void release_counting_wake(void* user_data) noexcept {
  static_cast<CountingWake*>(user_data)->released.store(true);
  mln_test_pulse();
}

auto create_counting_queue(CountingWake& wake) -> void {
  const auto options = mln_delivery_queue_options{
    .size = sizeof(mln_delivery_queue_options),
    .wake = {
      .callback = count_wake,
      .user_data = &wake,
      .release_user_data = release_counting_wake,
    },
  };
  MLN_TEST_OK(mln_delivery_queue_create(&options, &wake.queue, nullptr));
}

auto push_tagged(mln_delivery_queue handle, std::uintptr_t tag) -> bool {
  auto item = mln::delivery::make_light_item(
    MLN_DELIVERY_TYPE_RETIRED, reinterpret_cast<void*>(tag)
  );
  return mln::delivery::resolve_delivery_queue(handle)->push(item);
}

auto drain_with_allocation_budget(
  mln_delivery_queue handle, std::size_t budget, mln_delivery_batch& batch,
  mln_delivery_batch_view& view
) -> mln_status {
  const auto faults = mln::native_tests::AllocationFaults{budget};
  return mln_delivery_queue_drain(handle, &batch, &view, nullptr);
}

auto tag_at(const mln_delivery_batch_view& view, std::size_t index)
  -> std::uintptr_t {
  TEST_ASSERT_TRUE(index < view.delivery_count);
  const auto* bytes = static_cast<const unsigned char*>(
    static_cast<const void*>(view.deliveries)
  );
  return reinterpret_cast<std::uintptr_t>(
    reinterpret_cast<const mln_delivery*>(bytes + (index * view.delivery_size))
      ->user_data
  );
}

// A drain that fails at any allocation keeps every delivery queued in push
// order, ahead of what arrives before the next drain.
void a_failed_drain_keeps_every_delivery_in_order() {
  for (auto budget = std::size_t{}; budget < 5; ++budget) {
    const auto options = mln_delivery_queue_options{
      .size = sizeof(mln_delivery_queue_options),
    };
    auto handle = mln_delivery_queue{MLN_HANDLE_NULL};
    MLN_TEST_OK(mln_delivery_queue_create(&options, &handle, nullptr));
    const auto pushed = push_tagged(handle, 1) && push_tagged(handle, 2);
    auto batch = MLN_HANDLE_NULL;
    auto view =
      mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
    const auto failed =
      drain_with_allocation_budget(handle, budget, batch, view) !=
      MLN_STATUS_OK;
    const auto pushed_later = push_tagged(handle, 3);
    if (failed) {
      MLN_TEST_OK(mln_delivery_queue_drain(handle, &batch, &view, nullptr));
    }
    const auto count = view.delivery_count;
    const auto first = count > 0 ? tag_at(view, 0) : 0;
    mln_delivery_batch_release(batch);
    mln_delivery_queue_release(handle);
    TEST_ASSERT_TRUE(pushed);
    TEST_ASSERT_TRUE(pushed_later);
    TEST_ASSERT_EQUAL_size_t(failed ? 3 : 2, count);
    TEST_ASSERT_EQUAL_UINT64(1, first);
  }
}

// The queue wakes only on its empty-to-nonempty transition, so a failed drain
// makes the next push wake the receiver although the queue is not empty.
void the_next_push_wakes_after_a_failed_drain() {
  auto wake = CountingWake{};
  create_counting_queue(wake);
  TEST_ASSERT_TRUE(push_tagged(wake.queue, 1));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&wake.wakes, 1));

  auto batch = MLN_HANDLE_NULL;
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    drain_with_allocation_budget(wake.queue, 0, batch, view)
  );
  TEST_ASSERT_TRUE(push_tagged(wake.queue, 2));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&wake.wakes, 2));
  // Only the push right after the failure wakes a nonempty queue.
  TEST_ASSERT_TRUE(push_tagged(wake.queue, 3));
  const auto wakes = wake.wakes.load();

  MLN_TEST_OK(mln_delivery_queue_drain(wake.queue, &batch, &view, nullptr));
  const auto count = view.delivery_count;
  mln_delivery_batch_release(batch);
  mln_delivery_queue_release(wake.queue);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&wake.released));
  TEST_ASSERT_EQUAL_INT(2, wakes);
  TEST_ASSERT_EQUAL_size_t(3, count);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_failed_drain_keeps_every_delivery_in_order);
  RUN_TEST(the_next_push_wakes_after_a_failed_drain);
}
