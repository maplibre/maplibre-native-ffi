// Dart ports: binding support posts notifications to a Dart isolate through
// a post_cobject function, which these cases fake. A closed isolate rejects a
// post, and binding support must carry on without allocating.

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "internal/support/allocation_faults.hpp"
#include "internal/support/checks.hpp"
#include "maplibre_native_c.h"
#include "maplibre_native_c/binding_support.h"
#include "support/harness.h"
#include "support/status.h"
#include "support/wait.h"
#include "unity.h"

namespace {

using mln::native_tests::BackgroundChecks;
using mln::native_tests::ok_without_allocations;

// Dart_CObject types that binding support posts.
constexpr auto dart_int64 = std::int32_t{3};
constexpr auto dart_array = std::int32_t{6};

void a_dart_wake_posts_without_allocating_after_the_host_closes() {
  struct Message {
    std::int32_t type;
    std::int64_t value;
  };
  static auto checks = BackgroundChecks{};
  static unsigned deliveries = 0;
  static std::int64_t last_value = -1;
  deliveries = 0;
  last_value = -1;
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    checks.check(
      port == 17 && message->type == dart_int64, "an invalid Dart message"
    );
    ++deliveries;
    last_value = message->value;
    return false;  // A closed isolate port rejects delivery.
  };
  auto wake = mln_wake{};
  MLN_TEST_OK(mln_binding_dart_wake_create(
    reinterpret_cast<void*>(post), 17, &wake, nullptr
  ));
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    wake.callback(wake.user_data);
    wake.release_user_data(wake.user_data);
    return MLN_STATUS_OK;
  });
  TEST_ASSERT_EQUAL_UINT(2, deliveries);
  TEST_ASSERT_EQUAL_INT64(1, last_value);
}

void dart_notification_ports_copy_records_before_retirement() {
  struct Message {
    std::int32_t type;
    union {
      std::int64_t integer;
      struct {
        std::intptr_t count;
        Message** values;
      } array;
    };
  };
  static auto checks = BackgroundChecks{};
  static auto deliveries = unsigned{};
  deliveries = 0;
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    checks.check(port == 19, "an unexpected notification port");
    if (deliveries++ == 0) {
      checks.check(
        message->type == dart_array && message->array.count == 4,
        "the tile notification changed shape"
      );
      checks.check(
        message->array.values[0]->integer ==
            MLN_BINDING_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &&
          message->array.values[1]->integer == 5 &&
          message->array.values[2]->integer == 7 &&
          message->array.values[3]->integer == 9,
        "the tile notification lost its copied values"
      );
    } else {
      checks.check(
        message->type == dart_int64 && message->integer == 0,
        "the notification port did not retire"
      );
    }
    return false;
  };
  auto* context =
    mln_binding_dart_port_create(reinterpret_cast<void*>(post), 19);
  TEST_ASSERT_NOT_NULL(context);
  const auto callback = reinterpret_cast<mln_custom_source_tile_callback>(
    mln_binding_dart_port_function(
      MLN_BINDING_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE
    )
  );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    callback(context, {5, 7, 9});
    mln_binding_dart_port_release(context);
    mln_binding_dart_port_release(context);
    callback(context, {0, 0, 0});
    return MLN_STATUS_OK;
  });
  TEST_ASSERT_NULL_MESSAGE(checks.failure(), checks.failure());
  TEST_ASSERT_EQUAL_UINT_MESSAGE(
    2, deliveries, "a retired notification port accepted another callback"
  );
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_dart_wake_posts_without_allocating_after_the_host_closes);
  RUN_TEST(dart_notification_ports_copy_records_before_retirement);
}
