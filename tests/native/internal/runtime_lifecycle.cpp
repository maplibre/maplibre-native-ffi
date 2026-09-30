// Runtime release against a map's physical cleanup, which runs on a teardown
// lane after the map's own release has completed.

#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

// A released map completes before its worker pool shuts down, and the runtime
// release waits for that shutdown. The parked shutdown is the fence for the
// runtime's pending completion.
void runtime_release_waits_for_retired_map_cleanup() {
  auto sync_points = SyncPointScope{};
  sync_points.hold(SyncPoint::MapPoolShutdown);
  const auto options = mln_runtime_options_default();
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, nullptr)
  );
  auto create_map = mln_test_completion_default(sizeof(mln_map));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_create(runtime, nullptr, &create_map.descriptor, nullptr)
  );
  auto map = mln_map{MLN_HANDLE_NULL};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&create_map, &map, sizeof(map))
  );

  auto map_close = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_release(map, &map_close.descriptor, nullptr)
  );
  const auto cleanup_parked =
    sync_points.wait_for_hits(SyncPoint::MapPoolShutdown, 1);
  const auto map_closed = mln_test_completion_wait(&map_close, -1);
  auto runtime_close = mln_test_completion_default(0);
  const auto runtime_accepted =
    mln_runtime_release(runtime, &runtime_close.descriptor, nullptr);
  const auto runtime_closed_early = mln_test_completion_poll(&runtime_close);
  sync_points.release(SyncPoint::MapPoolShutdown);
  const auto map_status = mln_test_completion_settle(&map_close);
  const auto runtime_status = mln_test_completion_settle(&runtime_close);

  TEST_ASSERT_TRUE(cleanup_parked);
  TEST_ASSERT_TRUE(map_closed);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, runtime_accepted);
  TEST_ASSERT_FALSE_MESSAGE(
    runtime_closed_early,
    "the runtime release completed while a map's cleanup was parked"
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, map_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, runtime_status);
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(runtime_release_waits_for_retired_map_cleanup); }
