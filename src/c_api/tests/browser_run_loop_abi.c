#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#if defined(__EMSCRIPTEN__)
static void queued_async_task_runs_when_clock_advances_during_dispatch(void) {
  TEST_ASSERT_TRUE(mln_test_browser_async_task_runs_after_clock_advance());
}
static void stop_returns_after_its_worker_destroys_the_loop(void) {
  TEST_ASSERT_TRUE(mln_test_browser_stop_allows_immediate_destruction());
}
#endif

void run_browser_run_loop_abi_tests(void) {
#if defined(__EMSCRIPTEN__)
  UnitySetTestFile(__FILE__);
  RUN_TEST(queued_async_task_runs_when_clock_advances_during_dispatch);
  RUN_TEST(stop_returns_after_its_worker_destroys_the_loop);
#endif
}
