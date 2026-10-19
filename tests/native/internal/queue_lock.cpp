#include <stdexcept>

#include "render/queue_lock.hpp"

#include "support/test_support.h"

namespace {

void throw_from_lock(void*) { throw std::runtime_error{"lock failed"}; }

void count_unlock(void* user_data) { ++*static_cast<int*>(user_data); }

void a_throwing_host_lock_refuses_queue_access() {
  auto unlocks = 0;
  const auto descriptor = mln_queue_lock{
    .lock = throw_from_lock,
    .unlock = count_unlock,
    .user_data = &unlocks,
  };
  const auto lock = mln::core::QueueLock{descriptor};
  {
    const auto guard = mln::core::QueueLockGuard{&lock};
    TEST_ASSERT_TRUE(guard.refused());
  }
  TEST_ASSERT_EQUAL_INT(0, unlocks);
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(a_throwing_host_lock_refuses_queue_access); }
