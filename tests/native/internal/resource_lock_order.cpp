// Lock order around the resource provider and transform registrations. Their
// callbacks run on MapLibre threads under a per-runtime shared lock, and
// writers wait for them under the exclusive one. Neither side may hold the
// process-wide runtime registry lock while it waits, or one runtime's callback
// stalls every other runtime.
//
// The sync points order the threads: each case releases a callback once the
// thread it races is waiting at the lock under test. The writer points fire
// only when a writer finds the lock in use, so a writer that skips the wait
// never fires one, and its case fails at the deadline.

#include <atomic>
#include <cstdint>
#include <cstring>

#include "internal/support/resources.hpp"
#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

namespace {

using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

// The styles these cases download never reach a network: a provider answers
// them with an error, or a transform keeps the custom scheme, which the HTTP
// stack rejects without a connection.
constexpr auto offline_style_url = "custom://offline-style.json";
constexpr auto lookup_blocking_style_url = "custom://lookup-blocking.json";
constexpr auto lookup_probe_style_url = "custom://lookup-probe.json";

// An unknown decision becomes a handled provider error.
constexpr auto provider_error_decision = UINT32_MAX;

// A second runtime on its own thread, which calls a barrier once its trigger
// says the first runtime is inside the wait under test.
struct OtherRuntime {
  std::atomic_bool ready = false;
  std::atomic_bool call_done = false;
  std::atomic_int status = MLN_STATUS_NATIVE_ERROR;
  const SyncPointScope* sync_points = nullptr;
  // Set once the first runtime's racing thread has started; the point and hit
  // count are read after it.
  std::atomic_bool armed = false;
  SyncPoint point = SyncPoint::ResourceTransformExclusive;
  std::atomic_int hits = 0;
};

void run_other_runtime(void* argument) {
  auto& other = *static_cast<OtherRuntime*>(argument);
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  const auto options = mln_runtime_options_default();
  const auto created = mln_runtime_create(&options, &runtime, nullptr);
  if (created != MLN_STATUS_OK) {
    other.status = created;
    mln_test_flag_set(&other.call_done);
    return;
  }
  mln_test_flag_set(&other.ready);
  static_cast<void>(mln_test_wait_for_flag(&other.armed));
  static_cast<void>(
    other.sync_points->wait_for_hits(other.point, other.hits.load())
  );
  other.status = mln_test_runtime_barrier(runtime);
  mln_test_flag_set(&other.call_done);
  static_cast<void>(mln_test_runtime_close(runtime));
}

// Blocks until the other runtime's call returns, which it cannot do while
// anything holds the registry lock. Records whether it saw the call finish.
struct BlockingTransform {
  const char* only_url = nullptr;
  OtherRuntime* other = nullptr;
  std::atomic_bool entered = false;
  std::atomic_bool observed_call = false;
  std::atomic_bool released = false;
};

auto blocking_transform(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) -> mln_status {
  static_cast<void>(kind);
  auto& transform = *static_cast<BlockingTransform*>(user_data);
  if (out_response != nullptr) out_response->url = nullptr;
  if (
    transform.only_url != nullptr &&
    (url == nullptr || std::strstr(url, transform.only_url) == nullptr)
  ) {
    return MLN_STATUS_OK;
  }
  mln_test_flag_set(&transform.entered);
  static_cast<void>(mln_test_wait_for_flag(&transform.other->call_done));
  transform.observed_call = transform.other->call_done.load();
  return MLN_STATUS_OK;
}

void mark_transform_released(void* user_data) {
  mln_test_flag_set(&static_cast<BlockingTransform*>(user_data)->released);
}

// Runtime teardown waits for an in-flight transform callback without holding
// the registry lock, so calls on an unrelated runtime keep running.
void runtime_teardown_leaves_other_runtimes_responsive() {
  auto sync_points = SyncPointScope{};
  auto other = OtherRuntime{.sync_points = &sync_points};
  auto* other_thread = mln_test_thread_start(run_other_runtime, &other);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&other.ready));

  auto runtime = mln_test_create_runtime();
  auto probe = BlockingTransform{.other = &other};
  const auto transform = mln_resource_transform{
    .size = sizeof(mln_resource_transform),
    .callback = blocking_transform,
    .user_data = &probe,
    .release_user_data = mark_transform_released,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln::native_tests::set_resource_transform(runtime, transform)
  );
  TEST_ASSERT_TRUE(
    mln::native_tests::start_offline_download(runtime, offline_style_url)
  );
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &probe.entered));

  // The other runtime calls in once teardown reaches the exclusive lock, where
  // it waits for the transform above.
  other.point = SyncPoint::ResourceTransformExclusive;
  other.hits = sync_points.hits(SyncPoint::ResourceTransformExclusive) + 1;
  mln_test_flag_set(&other.armed);
  mln_test_destroy_runtime(runtime);
  mln_test_thread_join(other_thread);

  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.released));
  TEST_ASSERT_TRUE_MESSAGE(
    probe.observed_call.load(),
    "a call on an unrelated runtime stalled behind runtime teardown"
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, other.status.load());
}

// Parks the file source thread just before it looks up the resource
// transform, until a writer is waiting on the transform's exclusive lock, so
// the lookup queues behind that writer.
struct LookupProvider {
  const SyncPointScope* sync_points = nullptr;
  OtherRuntime* other = nullptr;
  std::atomic_bool entered = false;
  std::atomic_bool writer_pending = false;
  std::atomic_int writer_hits = 0;
  std::atomic_bool lookup_reached = false;
};

auto lookup_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) -> uint32_t {
  static_cast<void>(handle);
  auto& provider = *static_cast<LookupProvider*>(user_data);
  if (
    request == nullptr || request->requested_url == nullptr ||
    std::strstr(request->requested_url, lookup_probe_style_url) == nullptr
  ) {
    return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
  }
  mln_test_flag_set(&provider.entered);
  static_cast<void>(mln_test_wait_for_flag(&provider.writer_pending));
  static_cast<void>(provider.sync_points->wait_for_hits(
    SyncPoint::ResourceTransformExclusive, provider.writer_hits.load()
  ));
  // The next lookup is this thread's own, which the other runtime waits for.
  provider.other->point = SyncPoint::ResourceTransformLookup;
  provider.other->hits =
    provider.sync_points->hits(SyncPoint::ResourceTransformLookup) + 1;
  mln_test_flag_set(&provider.other->armed);
  mln_test_flag_set(&provider.lookup_reached);
  return MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
}

// A file source thread's transform lookup releases the registry lock before
// it waits on the per-runtime transform lock. The lookup below queues behind a
// pending writer, where holding the registry lock would stall every unrelated
// runtime.
void resource_transform_lookup_leaves_other_runtimes_responsive() {
  auto sync_points = SyncPointScope{};
  auto other = OtherRuntime{.sync_points = &sync_points};
  auto* other_thread = mln_test_thread_start(run_other_runtime, &other);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&other.ready));

  auto runtime = mln_test_create_runtime();
  auto provider_probe =
    LookupProvider{.sync_points = &sync_points, .other = &other};
  const auto provider = mln_resource_provider{
    .size = sizeof(mln_resource_provider),
    .callback = lookup_provider,
    .user_data = &provider_probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln::native_tests::set_resource_provider(runtime, provider)
  );
  auto transform_probe =
    BlockingTransform{.only_url = lookup_blocking_style_url, .other = &other};
  const auto transform = mln_resource_transform{
    .size = sizeof(mln_resource_transform),
    .callback = blocking_transform,
    .user_data = &transform_probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln::native_tests::set_resource_transform(runtime, transform)
  );

  // The first download parks a transform callback inside the shared lock.
  TEST_ASSERT_TRUE(
    mln::native_tests::start_offline_download(
      runtime, lookup_blocking_style_url
    )
  );
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &transform_probe.entered));
  // The second parks a file source thread one step ahead of the lookup.
  TEST_ASSERT_TRUE(
    mln::native_tests::start_offline_download(runtime, lookup_probe_style_url)
  );
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &provider_probe.entered));

  // Clearing waits for the parked transform callback, so it is the pending
  // writer the lookup queues behind.
  provider_probe.writer_hits =
    sync_points.hits(SyncPoint::ResourceTransformExclusive) + 1;
  mln_test_flag_set(&provider_probe.writer_pending);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln::native_tests::clear_resource_transform(runtime)
  );
  mln_test_thread_join(other_thread);

  TEST_ASSERT_TRUE_MESSAGE(
    provider_probe.lookup_reached.load(),
    "the file source thread never reached the transform lookup"
  );
  TEST_ASSERT_TRUE_MESSAGE(
    transform_probe.observed_call.load(),
    "a call on an unrelated runtime stalled behind a transform lookup"
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, other.status.load());
  mln_test_destroy_runtime(runtime);
}

// A provider callback that returns only once a writer is waiting for it on the
// provider's exclusive lock, or once the deadline passes without one.
struct ParkedProvider {
  const SyncPointScope* sync_points = nullptr;
  std::atomic_bool entered = false;
  std::atomic_bool writer_started = false;
  std::atomic_int writer_hits = 0;
  std::atomic_bool writer_waited = false;
  std::atomic_bool callback_returned = false;
  std::atomic_bool released = false;
};

auto parked_provider(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) -> uint32_t {
  static_cast<void>(request);
  static_cast<void>(handle);
  auto& provider = *static_cast<ParkedProvider*>(user_data);
  mln_test_flag_set(&provider.entered);
  static_cast<void>(mln_test_wait_for_flag(&provider.writer_started));
  provider.writer_waited = provider.sync_points->wait_for_hits(
    SyncPoint::ResourceProviderExclusive, provider.writer_hits.load()
  );
  mln_test_flag_set(&provider.callback_returned);
  return provider_error_decision;
}

void mark_provider_released(void* user_data) {
  mln_test_flag_set(&static_cast<ParkedProvider*>(user_data)->released);
}

auto install_parked_provider(mln_runtime runtime, ParkedProvider& probe)
  -> mln_status {
  const auto provider = mln_resource_provider{
    .size = sizeof(mln_resource_provider),
    .callback = parked_provider,
    .user_data = &probe,
    .release_user_data = mark_provider_released,
  };
  return mln::native_tests::set_resource_provider(runtime, provider);
}

// Arms the provider callback to return once the next writer reaches the
// provider's exclusive lock.
void start_writer(const SyncPointScope& sync_points, ParkedProvider& probe) {
  probe.writer_hits =
    sync_points.hits(SyncPoint::ResourceProviderExclusive) + 1;
  mln_test_flag_set(&probe.writer_started);
}

// The clear command reaches its terminal event only after a provider callback
// that was already running returns.
void clearing_resource_provider_waits_for_in_flight_callback() {
  auto sync_points = SyncPointScope{};
  auto runtime = mln_test_create_runtime();
  auto probe = ParkedProvider{.sync_points = &sync_points};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, install_parked_provider(runtime, probe));
  TEST_ASSERT_TRUE(
    mln::native_tests::start_offline_download(runtime, offline_style_url)
  );
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &probe.entered));

  start_writer(sync_points, probe);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln::native_tests::clear_resource_provider(runtime)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    probe.writer_waited.load(),
    "the clear never waited for the running provider callback"
  );
  TEST_ASSERT_TRUE_MESSAGE(
    probe.callback_returned.load(),
    "the clear completed while a provider callback was still running"
  );
  mln_test_destroy_runtime(runtime);
}

// Native keeps the provider's user_data until every in-flight callback
// returns, even though the public runtime release does not wait for teardown.
void runtime_teardown_waits_for_in_flight_provider_callback() {
  auto sync_points = SyncPointScope{};
  auto runtime = mln_test_create_runtime();
  auto probe = ParkedProvider{.sync_points = &sync_points};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, install_parked_provider(runtime, probe));
  TEST_ASSERT_TRUE(
    mln::native_tests::start_offline_download(runtime, offline_style_url)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.entered));

  start_writer(sync_points, probe);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.released));
  TEST_ASSERT_TRUE_MESSAGE(
    probe.writer_waited.load(),
    "runtime teardown never waited for the running provider callback"
  );
  TEST_ASSERT_TRUE_MESSAGE(
    probe.callback_returned.load(),
    "runtime teardown released the provider while its callback still ran"
  );
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(runtime_teardown_leaves_other_runtimes_responsive);
  RUN_TEST(resource_transform_lookup_leaves_other_runtimes_responsive);
  RUN_TEST(clearing_resource_provider_waits_for_in_flight_callback);
  RUN_TEST(runtime_teardown_waits_for_in_flight_provider_callback);
}
