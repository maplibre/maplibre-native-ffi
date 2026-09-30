// The browser run loop's scheduling, reached through its C++ internals.

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void queued_async_task_runs_when_clock_advances_during_dispatch(void) {
  TEST_ASSERT_TRUE(mln_test_browser_async_task_runs_after_clock_advance());
}
static void stop_returns_after_its_worker_destroys_the_loop(void) {
  TEST_ASSERT_TRUE(mln_test_browser_stop_allows_immediate_destruction());
}

MLN_TEST_GROUP {
  RUN_TEST(queued_async_task_runs_when_clock_advances_during_dispatch);
  RUN_TEST(stop_returns_after_its_worker_destroys_the_loop);
}
