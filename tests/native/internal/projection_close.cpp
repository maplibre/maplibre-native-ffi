// Standalone projection close against a conversion on another thread: the
// close waits for a call already running, and a call that leased the handle
// before the close but runs after it reports the handle as stale.

#include <atomic>
#include <cstring>
#include <thread>

#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

constexpr auto origin = mln_lat_lng{.latitude = 0.0, .longitude = 0.0};

auto convert(mln_map_projection projection) -> mln_screen_point {
  auto point = mln_screen_point{};
  MLN_TEST_OK(
    mln_map_projection_pixel_for_lat_lng(projection, origin, &point, nullptr)
  );
  return point;
}

// A conversion on a thread of its own, which records what it returned. No
// assertion runs while it is live, since a failed one would leave the thread
// writing into a returned frame; the case joins it first.
struct Conversion {
  explicit Conversion(mln_map_projection projection)
      : thread([this, projection] {
          diagnostic.size = sizeof(diagnostic);
          status = mln_map_projection_pixel_for_lat_lng(
            projection, origin, &point, &diagnostic
          );
          mln_test_pulse();
        }) {}

  std::atomic_int status{-1};
  mln_screen_point point{};
  mln_diagnostic diagnostic{};
  std::thread thread;
};

void close_waits_for_a_running_conversion() {
  SyncPointScope sync;
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  const auto projection = mln_test_map_create_projection(map);
  const auto expected = convert(projection);
  const auto calls = sync.hits(SyncPoint::ProjectionCallRunning);

  sync.hold(SyncPoint::ProjectionCallRunning);
  Conversion conversion(projection);
  const auto running =
    sync.wait_for_hits(SyncPoint::ProjectionCallRunning, calls + 1);
  auto close = std::thread([&] {
    mln_map_projection_release(projection);
    mln_test_pulse();
  });
  const auto waited = sync.wait_for_hits(SyncPoint::ProjectionCloseWaits, 1);
  sync.release(SyncPoint::ProjectionCallRunning);
  close.join();
  conversion.thread.join();

  TEST_ASSERT_TRUE_MESSAGE(running, "the conversion never ran");
  TEST_ASSERT_TRUE_MESSAGE(waited, "the close did not wait for the conversion");
  MLN_TEST_OK_MESSAGE(conversion.status.load(), conversion.diagnostic.message);
  TEST_ASSERT_EQUAL_DOUBLE(expected.x, conversion.point.x);
  TEST_ASSERT_EQUAL_DOUBLE(expected.y, conversion.point.y);
  auto point = mln_screen_point{};
  MLN_TEST_INVALID_STATE(
    mln_map_projection_pixel_for_lat_lng(projection, origin, &point, nullptr)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void a_conversion_that_leased_before_close_reports_a_stale_handle() {
  SyncPointScope sync;
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  const auto projection = mln_test_map_create_projection(map);

  sync.hold(SyncPoint::ProjectionCallLeased);
  Conversion conversion(projection);
  const auto leased = sync.wait_for_hits(SyncPoint::ProjectionCallLeased, 1);
  mln_map_projection_release(projection);
  sync.release(SyncPoint::ProjectionCallLeased);
  conversion.thread.join();

  TEST_ASSERT_TRUE_MESSAGE(leased, "the conversion never leased the handle");
  MLN_TEST_INVALID_STATE(conversion.status.load());
  TEST_ASSERT_NOT_NULL_MESSAGE(
    std::strstr(conversion.diagnostic.message, "stale"),
    conversion.diagnostic.message
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(close_waits_for_a_running_conversion);
  RUN_TEST(a_conversion_that_leased_before_close_reports_a_stale_handle);
}
