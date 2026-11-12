// The frame pacer on platforms without a display for it to follow. The files
// tagged _apple, _android, and _ohos test the pacers of the platforms with one.

#include "support/test_support.h"

#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(__OHOS__)
#define NO_DISPLAY_SOURCE 1
#endif

#if defined(NO_DISPLAY_SOURCE)
static void a_platform_without_a_display_source_has_no_frame_pacer(void) {
  mln_frame_pacer pacer = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_frame_pacer_create(NULL, &pacer, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, pacer);
}

// Without a pacer to change, only the handle check is left to test.
static void the_null_handle_takes_no_frame_pacer_options(void) {
  mln_frame_pacer_options options = {.size = sizeof(mln_frame_pacer_options)};
  options.fields = MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
  options.maximum_frames_per_second = 30;
  MLN_TEST_INVALID(
    mln_frame_pacer_set_options(MLN_HANDLE_NULL, &options, MLN_TEST_DIAGNOSTIC)
  );
}
#endif

MLN_TEST_GROUP {
#if defined(NO_DISPLAY_SOURCE)
  RUN_TEST(a_platform_without_a_display_source_has_no_frame_pacer);
  RUN_TEST(the_null_handle_takes_no_frame_pacer_options);
#endif
}
