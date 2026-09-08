// Raw C ABI coverage for acquired-frame ownership and synchronization input.

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void acquired_frame_release_validates_owned_handle_input(void) {
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  const mln_gpu_sync sync = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_acquired_frame_release(&frame, &sync)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_acquired_frame_get_result(
      MLN_HANDLE_NULL,
      &(mln_render_frame_result){.size = sizeof(mln_render_frame_result)}
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_acquired_frame_get_producer_sync(
      MLN_HANDLE_NULL, &(mln_gpu_sync){.size = sizeof(mln_gpu_sync)}
    )
  );
}

void run_owned_texture_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(acquired_frame_release_validates_owned_handle_input);
}
