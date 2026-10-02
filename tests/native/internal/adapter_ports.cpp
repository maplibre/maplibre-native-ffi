// Dart ports: the adapter posts records to a Dart isolate through a
// post_cobject function, which these cases fake. A closed isolate rejects a
// post, and the adapter must then free what it posted, without allocating.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

#include "internal/support/allocation_faults.hpp"
#include "internal/support/checks.hpp"
#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"
#include "runtime/runtime.hpp"
#include "support/harness.h"
#include "support/status.h"
#include "support/wait.h"
#include "unity.h"

namespace {

using mln::native_tests::await;
using mln::native_tests::BackgroundChecks;
using mln::native_tests::ok_without_allocations;

// Dart_CObject types the adapter posts.
constexpr auto dart_int64 = std::int32_t{3};
constexpr auto dart_array = std::int32_t{6};
constexpr auto dart_native_pointer = std::int32_t{11};

auto create_runtime(std::atomic_uint& wake_releases) -> mln_runtime {
  auto options = mln_runtime_options_default();
  options.event_wake.callback = [](void*) {};
  options.event_wake.user_data = &wake_releases;
  options.event_wake.release_user_data = [](void* context) {
    ++*static_cast<std::atomic_uint*>(context);
    mln_test_pulse();
  };
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
  return runtime;
}

// A map creation completion whose isolate closed before delivery frees the
// map result it carried, on the runtime worker.
void undelivered_dart_completion_disposes_its_owned_result() {
  struct Message {
    std::int32_t type;
    union {
      std::int64_t integer;
      struct {
        std::intptr_t count;
        Message** values;
      } array;
      struct {
        std::intptr_t pointer;
        std::intptr_t size;
        void (*finalize)(void*, void*);
      } native_pointer;
    };
  };
  static auto checks = BackgroundChecks{};
  static auto deliveries = std::atomic_uint{};
  deliveries = 0;
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    checks.check(
      port == 23 && message->type == dart_array && message->array.count == 2,
      "the completion port message changed shape"
    );
    checks.check(
      message->array.values[0]->integer == 31, "the completion token changed"
    );
    const auto& payload = *message->array.values[1];
    checks.check(
      payload.type == dart_native_pointer &&
        payload.native_pointer.pointer != 0,
      "the completion message lost its native finalizer"
    );
    checks.check(
      ok_without_allocations([&] {
        payload.native_pointer.finalize(
          nullptr, reinterpret_cast<void*>(payload.native_pointer.pointer)
        );
        return MLN_STATUS_OK;
      }),
      "finalizing the result allocated"
    );
    ++deliveries;
    mln_test_pulse();
    return false;
  };
  auto releases = std::atomic_uint{};
  const auto runtime = create_runtime(releases);
  auto weak = std::weak_ptr{mln::core::lease_runtime(runtime)};
  auto completion = mln_completion{};
  MLN_TEST_OK(mln_adapter_dart_completion_create(
    MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map),
    reinterpret_cast<void*>(post), 23, 31, &completion, nullptr
  ));
  MLN_TEST_OK(mln_map_create(runtime, nullptr, &completion, nullptr));
  TEST_ASSERT_TRUE(
    await([&] { return deliveries.load() == 1; }, "the completion post")
  );
  TEST_ASSERT_NULL_MESSAGE(checks.failure(), checks.failure());
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    return mln_runtime_dispose(runtime, nullptr);
  });
  TEST_ASSERT_TRUE(await(
    [&] { return weak.expired() && releases.load() == 1; },
    "the runtime to retire"
  ));
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
            MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE &&
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
    mln_adapter_dart_port_create(reinterpret_cast<void*>(post), 19);
  TEST_ASSERT_NOT_NULL(context);
  const auto callback =
    reinterpret_cast<mln_custom_geometry_source_tile_callback>(
      mln_adapter_dart_port_function(
        MLN_ADAPTER_DART_PORT_CUSTOM_GEOMETRY_SOURCE_OPTIONS_FETCH_TILE
      )
    );
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    callback(context, {5, 7, 9});
    mln_adapter_dart_port_release(context);
    mln_adapter_dart_port_release(context);
    callback(context, {0, 0, 0});
    return MLN_STATUS_OK;
  });
  TEST_ASSERT_NULL_MESSAGE(checks.failure(), checks.failure());
  TEST_ASSERT_EQUAL_UINT_MESSAGE(
    2, deliveries, "a retired notification port accepted another callback"
  );
}

void dart_deferred_callbacks_post_records_before_retirement() {
  struct Message {
    std::int32_t type;
    union {
      std::int64_t integer;
      struct {
        std::intptr_t count;
        Message** values;
      } array;
      struct {
        std::intptr_t pointer;
        std::intptr_t size;
        void (*finalize)(void*, void*);
      } native_pointer;
    };
  };
  static auto checks = BackgroundChecks{};
  static auto deliveries = unsigned{};
  deliveries = 0;
  const auto post = +[](std::int64_t port, Message* message) -> bool {
    checks.check(port == 29, "an unexpected deferred callback port");
    if (deliveries++ != 0) {
      checks.check(
        message->type == dart_int64 && message->integer == 0,
        "the deferred callback context did not retire"
      );
      return false;
    }
    checks.check(
      message->type == dart_array && message->array.count == 2 &&
        message->array.values[0]->integer == MLN_ADAPTER_DEFERRED_LOG_CALLBACK,
      "the deferred callback message changed shape"
    );
    const auto& payload = *message->array.values[1];
    checks.check(
      payload.type == dart_native_pointer &&
        payload.native_pointer.pointer != 0,
      "the deferred record lost its native finalizer"
    );
    if (payload.native_pointer.pointer == 0) return false;
    const auto* record = reinterpret_cast<mln_adapter_deferred_call_record*>(
      payload.native_pointer.pointer
    );
    const auto* arguments =
      static_cast<const mln_adapter_log_callback_arguments*>(record->arguments);
    checks.check(
      arguments->code == 7 && std::strcmp(arguments->message, "deferred") == 0,
      "the deferred record lost its copied arguments"
    );
    // A closed port finalizes the undelivered record.
    payload.native_pointer.finalize(
      nullptr, reinterpret_cast<void*>(payload.native_pointer.pointer)
    );
    return false;
  };
  void* context = nullptr;
  MLN_TEST_OK(mln_adapter_dart_deferred_callback_create(
    MLN_ADAPTER_DEFERRED_LOG_CALLBACK, reinterpret_cast<void*>(post), 29,
    &context, nullptr
  ));
  auto* address =
    mln_adapter_deferred_callback_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK);
  const auto callback = reinterpret_cast<mln_log_callback>(address);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    1,
    callback(
      context, MLN_LOG_SEVERITY_INFO, MLN_LOG_EVENT_GENERAL, 7, "deferred"
    ),
    "the deferred log callback did not consume its record"
  );
  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_NULL_MESSAGE(checks.failure(), checks.failure());
  TEST_ASSERT_EQUAL_UINT(2, deliveries);
}

void dart_ports_release_after_the_host_closes() {
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
  MLN_TEST_OK(mln_adapter_dart_wake_create(
    reinterpret_cast<void*>(post), 17, &wake, nullptr
  ));
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    wake.callback(wake.user_data);
    wake.release_user_data(wake.user_data);
    return MLN_STATUS_OK;
  });
  TEST_ASSERT_EQUAL_UINT(2, deliveries);
  TEST_ASSERT_EQUAL_INT64(1, last_value);

  auto* arena = mln_adapter_arena_create();
  TEST_ASSERT_NOT_NULL(arena);
  auto* context =
    mln_adapter_arena_allocate(arena, 32, alignof(std::max_align_t));
  TEST_ASSERT_NOT_NULL(context);
  auto* initial = static_cast<unsigned char*>(context);
  for (std::size_t i = 0; i < 32; ++i) TEST_ASSERT_EQUAL_UINT8(0, initial[i]);
  std::memset(initial, 0x5a, 32);
  auto* aligned = mln_adapter_arena_allocate(arena, 4096, 256);
  TEST_ASSERT_NOT_NULL(aligned);
  TEST_ASSERT_EQUAL_UINT64(0, reinterpret_cast<std::uintptr_t>(aligned) % 256);
  std::memset(aligned, 0xa5, 4096);
  TEST_ASSERT_NULL(mln_adapter_arena_allocate(
    arena, std::numeric_limits<std::size_t>::max(), 16
  ));
  for (std::size_t i = 0; i < 32; ++i) {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(
      0x5a, initial[i], "arena growth invalidated earlier storage"
    );
  }

  auto runtime_releases = std::atomic_uint{};
  const auto runtime = create_runtime(runtime_releases);
  MLN_TEST_OK(mln_adapter_arena_adopt_handle(arena, runtime, nullptr));
  static auto context_releases = unsigned{};
  context_releases = 0;
  MLN_TEST_OK(mln_adapter_arena_adopt_release(
    arena, [](void*) { ++context_releases; }, nullptr, nullptr
  ));
  auto registration = std::uint64_t{};
  MLN_TEST_OK(mln_adapter_dart_release_register(
    reinterpret_cast<void*>(post), 17, context, arena, &registration, nullptr
  ));
  MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
    mln_adapter_dart_release(context);
    mln_adapter_dart_release(context);
    return MLN_STATUS_OK;
  });
  TEST_ASSERT_EQUAL_UINT_MESSAGE(
    3, deliveries, "the Dart release did not retire exactly once"
  );
  TEST_ASSERT_EQUAL_INT64(static_cast<std::int64_t>(registration), last_value);
  TEST_ASSERT_TRUE(
    await([&] { return runtime_releases.load() == 1; }, "the runtime to retire")
  );
  TEST_ASSERT_EQUAL_UINT_MESSAGE(
    1, context_releases, "a closed isolate leaked an adopted context release"
  );

  // The retired address may come back from the allocator for another
  // registration, which must get a notification ID of its own.
  arena = mln_adapter_arena_create();
  auto second_registration = std::uint64_t{};
  MLN_TEST_OK(mln_adapter_dart_release_register(
    reinterpret_cast<void*>(post), 17, context, arena, &second_registration,
    nullptr
  ));
  TEST_ASSERT_NOT_EQUAL_UINT64(registration, second_registration);
  mln_adapter_dart_release(context);
  TEST_ASSERT_NULL_MESSAGE(checks.failure(), checks.failure());
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(dart_ports_release_after_the_host_closes);
  RUN_TEST(dart_notification_ports_copy_records_before_retirement);
  RUN_TEST(dart_deferred_callbacks_post_records_before_retirement);
  RUN_TEST(undelivered_dart_completion_disposes_its_owned_result);
}
