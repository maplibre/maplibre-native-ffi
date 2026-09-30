// An ambient cache operation that a runtime accepted before its release still
// runs to completion after the handle stops resolving.

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void an_accepted_cache_operation_completes_after_the_runtime_releases(
  void
) {
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = ":memory:";
  mln_runtime runtime = mln_test_create_runtime_with_options(options);
  // Nothing has opened the database yet, so the operation opens it on the
  // worker, after the release below retired the handle.
  mln_test_completion cleared = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_run_ambient_cache_operation(
      runtime, MLN_AMBIENT_CACHE_OPERATION_CLEAR, &cleared.descriptor, NULL
    )
  );
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&cleared));
  mln_test_completion_destroy(&cleared);
}

MLN_TEST_GROUP {
  RUN_TEST(an_accepted_cache_operation_completes_after_the_runtime_releases);
}
