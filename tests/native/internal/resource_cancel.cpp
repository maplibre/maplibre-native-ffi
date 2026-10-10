// Cancel callbacks on handled resource requests: when MapLibre's cancel hook
// runs one, and how a release on another thread waits for one still running.

#include <atomic>
#include <cstdint>

#include "internal/support/resources.hpp"
#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/test_support.h"

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

constexpr std::uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

auto style_response() -> mln_resource_response {
  return mln_resource_response{
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .error_reason = MLN_RESOURCE_ERROR_REASON_NONE,
    .bytes = inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
  };
}

struct CompletedRequest {
  std::atomic_bool provider_entered = false;
  std::atomic_int cancel_count = 0;
  std::atomic_int release_count = 0;
  std::atomic_int register_status = MLN_STATUS_NATIVE_ERROR;
  std::atomic<mln_resource_request_handle> handle = MLN_HANDLE_NULL;
};

void count_cancel(void* user_data) {
  ++static_cast<CompletedRequest*>(user_data)->cancel_count;
  mln_test_pulse();
}

void count_cancel_release(void* user_data) {
  ++static_cast<CompletedRequest*>(user_data)->release_count;
  mln_test_pulse();
}

// Registers a cancel callback, then answers the request inline.
auto complete_inline_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) -> uint32_t {
  static_cast<void>(request);
  auto& probe = *static_cast<CompletedRequest*>(user_data);
  probe.handle = handle;
  auto cancelled = true;
  const mln_resource_request_cancel_handler handler{
    .size = sizeof(mln_resource_request_cancel_handler),
    .callback = count_cancel,
    .user_data = &probe,
    .release_user_data = count_cancel_release,
  };
  probe.register_status = mln_resource_request_set_cancel_callback(
    handle, &handler, &cancelled, nullptr
  );
  const auto response = style_response();
  static_cast<void>(mln_resource_request_complete(handle, &response, nullptr));
  mln_test_flag_set(&probe.provider_entered);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

// MapLibre runs its cancel hook on every request teardown, including after the
// response was delivered. The C API reports cancellation only for a request the
// provider has not completed. The hook's sync point is the fence: once it has
// run, a cancel callback it was going to run has run.
void cancel_callback_skips_a_completed_request() {
  auto sync_points = SyncPointScope{};
  auto probe = CompletedRequest{};
  auto runtime = mln_test_create_runtime();
  const auto provider = mln_resource_provider{
    .size = sizeof(mln_resource_provider),
    .callback = complete_inline_provider,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln::native_tests::set_resource_provider(runtime, provider));
  auto map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://cancel-style.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.provider_entered));
  MLN_TEST_OK(probe.register_status.load());
  const auto handle = probe.handle.load();

  // Let the response reach the style before the map goes away.
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(
    sync_points.wait_for_hits(SyncPoint::ResourceRequestCancelled, 1)
  );
  TEST_ASSERT_EQUAL_INT(0, probe.cancel_count.load());
  auto cancelled = true;
  MLN_TEST_OK(mln_resource_request_is_cancelled(handle, &cancelled, nullptr));
  TEST_ASSERT_FALSE(cancelled);
  TEST_ASSERT_EQUAL_INT(0, probe.release_count.load());

  // A registration whose callback never ran retires with the request.
  mln_resource_request_release(handle);
  TEST_ASSERT_EQUAL_INT(0, probe.cancel_count.load());
  TEST_ASSERT_EQUAL_INT(1, probe.release_count.load());
  mln_test_destroy_runtime(runtime);
}

// A cancel callback that runs on the runtime worker while the case releases
// the request on its own thread. It returns once that release is blocked on
// it, and records whether the release had already returned.
struct BlockingCancel {
  const SyncPointScope* sync_points = nullptr;
  bool self_release = false;
  bool waiter_drains_instead_of_releasing = false;
  std::atomic_bool provider_entered = false;
  std::atomic_int register_status = MLN_STATUS_NATIVE_ERROR;
  std::atomic_bool register_reported_cancelled = true;
  std::atomic<mln_resource_request_handle> handle = MLN_HANDLE_NULL;
  std::atomic_int status_after_self_release = MLN_STATUS_OK;
  std::atomic_int complete_status_after_self_release = MLN_STATUS_OK;
  std::atomic_bool callback_entered = false;
  std::atomic_bool release_started = false;
  std::atomic_bool release_returned = false;
  std::atomic_bool release_returned_during_callback = false;
  std::atomic_bool callback_returned = false;
};

void block_in_cancel(void* user_data) {
  auto& probe = *static_cast<BlockingCancel*>(user_data);
  if (probe.self_release) {
    const auto handle = probe.handle.load();
    mln_resource_request_release(handle);
    // The entry outlives this callback, but the handle is released: every
    // other entry point reports it as such.
    auto cancelled = false;
    probe.status_after_self_release =
      mln_resource_request_is_cancelled(handle, &cancelled, nullptr);
    const auto response = style_response();
    probe.complete_status_after_self_release =
      mln_resource_request_complete(handle, &response, nullptr);
  }
  mln_test_flag_set(&probe.callback_entered);
  static_cast<void>(mln_test_wait_for_flag(&probe.release_started));
  static_cast<void>(
    probe.sync_points->wait_for_hits(SyncPoint::ResourceRequestCancelWait, 1)
  );
  probe.release_returned_during_callback = probe.release_returned.load();
  mln_test_flag_set(&probe.callback_returned);
}

auto blocking_cancel_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) -> uint32_t {
  static_cast<void>(request);
  auto& probe = *static_cast<BlockingCancel*>(user_data);
  probe.handle = handle;
  auto cancelled = true;
  const mln_resource_request_cancel_handler handler{
    .size = sizeof(mln_resource_request_cancel_handler),
    .callback = block_in_cancel,
    .user_data = &probe,
    .release_user_data = nullptr,
  };
  probe.register_status = mln_resource_request_set_cancel_callback(
    handle, &handler, &cancelled, nullptr
  );
  probe.register_reported_cancelled = cancelled;
  mln_test_flag_set(&probe.provider_entered);
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

// user_data may be freed once release returns, so release waits for a callback
// still running on another thread. Destroying the map cancels its style request
// on the runtime worker, so the release runs on the case's thread.
void run_release_waits_for_in_flight_cancel_callback(BlockingCancel& probe) {
  auto runtime = mln_test_create_runtime();
  const auto provider = mln_resource_provider{
    .size = sizeof(mln_resource_provider),
    .callback = blocking_cancel_provider,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln::native_tests::set_resource_provider(runtime, provider));
  auto map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://blocking-cancel.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.provider_entered));
  MLN_TEST_OK(probe.register_status.load());
  TEST_ASSERT_FALSE(probe.register_reported_cancelled.load());

  mln_test_destroy_map(map);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.callback_entered));
  mln_test_flag_set(&probe.release_started);
  const auto handle = probe.handle.load();
  if (probe.waiter_drains_instead_of_releasing) {
    MLN_TEST_OK(mln_resource_request_wait_until_retired(handle, nullptr));
  } else {
    mln_resource_request_release(handle);
  }
  const auto callback_returned_before_release = probe.callback_returned.load();
  mln_test_flag_set(&probe.release_returned);
  TEST_ASSERT_FALSE_MESSAGE(
    probe.release_returned_during_callback.load(),
    "releasing the request returned while its cancel callback was running"
  );
  TEST_ASSERT_TRUE(callback_returned_before_release);
  if (probe.self_release) {
    MLN_TEST_INVALID_STATE(probe.status_after_self_release.load());
    MLN_TEST_INVALID_STATE(probe.complete_status_after_self_release.load());
  } else {
    mln_resource_request_release(handle);
  }
  auto cancelled = false;
  MLN_TEST_INVALID_STATE(
    mln_resource_request_is_cancelled(handle, &cancelled, nullptr)
  );
  mln_test_destroy_runtime(runtime);
}

void release_waits_for_in_flight_cancel_callback() {
  auto sync_points = SyncPointScope{};
  auto probe = BlockingCancel{.sync_points = &sync_points};
  run_release_waits_for_in_flight_cancel_callback(probe);
}

// Release is idempotent, so a release from the case's thread must still find
// the request and wait after the callback released it from inside. Inside that
// window the released handle already rejects every other entry point.
void release_waits_for_a_cancel_callback_that_released_itself() {
  auto sync_points = SyncPointScope{};
  auto probe =
    BlockingCancel{.sync_points = &sync_points, .self_release = true};
  run_release_waits_for_in_flight_cancel_callback(probe);
}

// Teardown drains requests through the wait-until-retired call, so a request
// whose callback released it from inside is drained only once the callback
// returns.
void wait_until_retired_waits_for_a_self_releasing_cancel_callback() {
  auto sync_points = SyncPointScope{};
  auto probe = BlockingCancel{
    .sync_points = &sync_points,
    .self_release = true,
    .waiter_drains_instead_of_releasing = true,
  };
  run_release_waits_for_in_flight_cancel_callback(probe);
}

// A request whose cancel callback never runs, released on a host thread whose
// release_user_data returns only once every other retirement of the request is
// blocked on it: the drain, and in the second case the provider's own
// retirement when it passes the request through after that release.
struct BlockingRegistrationRelease {
  const SyncPointScope* sync_points = nullptr;
  // Release on a host thread while the provider is still deciding, then pass
  // the request through, so the provider thread retires it a second time.
  bool release_during_decision = false;
  mln_test_thread* releaser = nullptr;
  std::atomic_bool provider_entered = false;
  std::atomic_int register_status = MLN_STATUS_NATIVE_ERROR;
  std::atomic<mln_resource_request_handle> handle = MLN_HANDLE_NULL;
  std::atomic_bool release_entered = false;
  std::atomic_bool drain_returned = false;
  std::atomic_bool drain_returned_during_release = false;
  std::atomic_bool release_returned = false;
};

void never_cancelled(void* user_data) { static_cast<void>(user_data); }

void block_in_registration_release(void* user_data) {
  auto& probe = *static_cast<BlockingRegistrationRelease*>(user_data);
  mln_test_flag_set(&probe.release_entered);
  const auto waiters = probe.release_during_decision ? 2 : 1;
  static_cast<void>(probe.sync_points->wait_for_hits(
    SyncPoint::ResourceRequestCancelWait, waiters
  ));
  probe.drain_returned_during_release = probe.drain_returned.load();
  mln_test_flag_set(&probe.release_returned);
}

void release_request_on_this_thread(void* argument) {
  mln_resource_request_release(
    static_cast<BlockingRegistrationRelease*>(argument)->handle.load()
  );
}

auto blocking_registration_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) -> uint32_t {
  static_cast<void>(request);
  auto& probe = *static_cast<BlockingRegistrationRelease*>(user_data);
  probe.handle = handle;
  auto cancelled = true;
  const mln_resource_request_cancel_handler handler{
    .size = sizeof(mln_resource_request_cancel_handler),
    .callback = never_cancelled,
    .user_data = &probe,
    .release_user_data = block_in_registration_release,
  };
  probe.register_status = mln_resource_request_set_cancel_callback(
    handle, &handler, &cancelled, nullptr
  );
  if (!probe.release_during_decision) {
    mln_test_flag_set(&probe.provider_entered);
    return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
  }
  probe.releaser =
    mln_test_thread_start(release_request_on_this_thread, &probe);
  mln_test_flag_set(&probe.provider_entered);
  static_cast<void>(mln_test_wait_for_flag(&probe.release_entered));
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

void run_drain_waits_for_an_unrun_registrations_release(
  BlockingRegistrationRelease& probe
) {
  auto runtime = mln_test_create_runtime();
  const auto provider = mln_resource_provider{
    .size = sizeof(mln_resource_provider),
    .callback = blocking_registration_provider,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln::native_tests::set_resource_provider(runtime, provider));
  auto map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_style_url(map, "custom://blocking-registration.json")
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.provider_entered));
  MLN_TEST_OK(probe.register_status.load());

  auto* releaser =
    probe.release_during_decision
      ? probe.releaser
      : mln_test_thread_start(release_request_on_this_thread, &probe);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.release_entered));
  MLN_TEST_OK(
    mln_resource_request_wait_until_retired(probe.handle.load(), nullptr)
  );
  const auto release_returned_before_drain = probe.release_returned.load();
  mln_test_flag_set(&probe.drain_returned);
  mln_test_thread_join(releaser);
  TEST_ASSERT_FALSE_MESSAGE(
    probe.drain_returned_during_release.load(),
    "the drain returned while release_user_data was running"
  );
  TEST_ASSERT_TRUE(release_returned_before_drain);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A host frees what release_user_data uses once a drain returns, so the drain
// also waits for the release of a registration whose callback never ran.
void wait_until_retired_waits_for_an_unrun_registrations_release() {
  auto sync_points = SyncPointScope{};
  auto probe = BlockingRegistrationRelease{.sync_points = &sync_points};
  run_drain_waits_for_an_unrun_registrations_release(probe);
}

// A provider that passes a request through retires it again on its own thread.
// That retirement finds the registration already taken, and it must not hand
// the request to the drain while the host's release is still running.
void a_second_retirement_waits_for_an_unrun_registrations_release() {
  auto sync_points = SyncPointScope{};
  auto probe = BlockingRegistrationRelease{
    .sync_points = &sync_points,
    .release_during_decision = true,
  };
  run_drain_waits_for_an_unrun_registrations_release(probe);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(cancel_callback_skips_a_completed_request);
  RUN_TEST(release_waits_for_in_flight_cancel_callback);
  RUN_TEST(release_waits_for_a_cancel_callback_that_released_itself);
  RUN_TEST(wait_until_retired_waits_for_a_self_releasing_cancel_callback);
  RUN_TEST(wait_until_retired_waits_for_an_unrun_registrations_release);
  RUN_TEST(a_second_retirement_waits_for_an_unrun_registrations_release);
}
