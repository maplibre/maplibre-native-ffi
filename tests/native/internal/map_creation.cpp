// A map whose creation failed. Nothing public makes MapLibre's map constructor
// throw, so the case fails the first allocation the runtime worker makes after
// the MapConstruction point.

#include <optional>

#include "internal/support/allocation_faults.hpp"
#include "maplibre_native_c.h"
#include "support/test_support.h"
#include "testing/sync_point.hpp"

namespace {

using mln::native_tests::AllocationFaults;
using mln::testing::SyncPoint;

void fail_construction(SyncPoint point, void* /*context*/) noexcept {
  if (point != SyncPoint::MapConstruction) return;
  // A thread-local scope outlives the handler on the worker. It fails one
  // allocation and then permits the rest.
  thread_local auto faults = std::optional<AllocationFaults>{};
  faults.emplace(0);
}

void work_queued_on_a_failed_map_reports_invalid_state() {
  const auto runtime = mln_test_create_runtime();
  auto options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  auto created = mln_test_completion_default(0);
  auto map = mln_map{MLN_HANDLE_NULL};
  mln::testing::set_sync_point_handler(fail_construction, nullptr);
  MLN_TEST_OK(mln_runtime_create_map(
    runtime, &options, &map, &created.descriptor, nullptr
  ));
  auto command = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_set_debug_options(map, 0, &command.descriptor, nullptr));
  auto still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, nullptr));
  const auto status = mln_test_completion_settle(&created);
  mln::testing::set_sync_point_handler(nullptr, nullptr);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, status);

  TEST_ASSERT_TRUE(mln_test_completion_wait(&command, -1));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_FAILED, mln_test_completion_disposition(&command)
  );
  MLN_TEST_INVALID_STATE(mln_test_completion_settle(&command));
  MLN_TEST_INVALID_STATE(mln_test_completion_settle(&still));

  auto url = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_get_style_url(map, &url.descriptor, nullptr));
  MLN_TEST_INVALID_STATE(mln_test_completion_settle(&url));
  auto layers = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_list_style_layers(map, &layers.descriptor, nullptr));
  MLN_TEST_INVALID_STATE(mln_test_completion_settle(&layers));
  auto camera = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_get_camera(map, &camera.descriptor, nullptr));
  MLN_TEST_INVALID_STATE(mln_test_completion_settle(&camera));
  const mln_lat_lng coordinates[] = {{0, 0}, {1, 1}};
  auto fit = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_camera_for_lat_lngs(
    map, coordinates, 2, nullptr, &fit.descriptor, nullptr
  ));
  MLN_TEST_INVALID_STATE(mln_test_completion_settle(&fit));
  auto snapshot = mln_map_snapshot{.size = sizeof(mln_map_snapshot)};
  MLN_TEST_INVALID_STATE(mln_map_get_snapshot(map, &snapshot, nullptr));

  // The owner releases a failed map as it does any other.
  auto released = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_release(map, &released.descriptor, nullptr));
  MLN_TEST_OK(mln_test_completion_settle(&released));
  mln_test_destroy_runtime(runtime);
}

}  // namespace

MLN_TEST_GROUP { RUN_TEST(work_queued_on_a_failed_map_reports_invalid_state); }
