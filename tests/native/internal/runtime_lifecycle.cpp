// Runtime release against a map's physical cleanup, which runs on a teardown
// lane after the map's own release has completed.

#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

auto create_runtime() -> mln_runtime {
  const auto options = mln_runtime_options_default();
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
  return runtime;
}

// A released map completes before its worker pool shuts down, and the runtime
// release waits for that shutdown. The case parks the shutdown and waits for
// the first runtime's release to report that it deferred retirement, which it
// reports only when it has to defer. Runtime retirement shares one FIFO lane,
// so a second runtime's release completes while the first one waits, and the
// first one's completion would arrive before it if it had not waited.
void a_deferred_runtime_release_does_not_hold_other_retirements() {
  auto sync_points = SyncPointScope{};
  sync_points.hold(SyncPoint::MapPoolShutdown);
  const auto waiting = create_runtime();
  const auto other = create_runtime();

  auto create_map = mln_test_completion_default(sizeof(mln_map));
  MLN_TEST_OK(
    mln_runtime_create_map(waiting, nullptr, &create_map.descriptor, nullptr)
  );
  auto map = mln_map{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_test_completion_finish_value(&create_map, &map, sizeof(map)));
  auto map_close = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_release(map, &map_close.descriptor, nullptr));
  const auto cleanup_parked =
    sync_points.wait_for_hits(SyncPoint::MapPoolShutdown, 1);
  MLN_TEST_OK(mln_test_completion_settle(&map_close));

  auto waiting_close = mln_test_completion_default(0);
  const auto waiting_accepted =
    mln_runtime_release(waiting, &waiting_close.descriptor, nullptr);
  const auto waiting_deferred =
    sync_points.wait_for_hits(SyncPoint::RuntimeRetirementDeferred, 1);
  auto other_close = mln_test_completion_default(0);
  const auto other_accepted =
    mln_runtime_release(other, &other_close.descriptor, nullptr);
  const auto other_closed = mln_test_completion_wait(&other_close, -1);
  const auto waiting_closed_early = mln_test_completion_poll(&waiting_close);
  sync_points.release(SyncPoint::MapPoolShutdown);
  const auto other_status = mln_test_completion_settle(&other_close);
  const auto waiting_status = mln_test_completion_settle(&waiting_close);

  TEST_ASSERT_TRUE(cleanup_parked);
  MLN_TEST_OK(waiting_accepted);
  TEST_ASSERT_TRUE_MESSAGE(
    waiting_deferred,
    "the runtime release never deferred for a map's parked cleanup"
  );
  MLN_TEST_OK(other_accepted);
  TEST_ASSERT_TRUE_MESSAGE(
    other_closed,
    "a runtime release waited behind another runtime's deferred retirement"
  );
  TEST_ASSERT_FALSE_MESSAGE(
    waiting_closed_early,
    "the runtime release completed while a map's cleanup was parked"
  );
  MLN_TEST_OK(other_status);
  MLN_TEST_OK(waiting_status);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_deferred_runtime_release_does_not_hold_other_retirements);
}
