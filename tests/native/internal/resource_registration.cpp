// Rejected registrations retain host ownership even when acceptance throws.
#include <optional>

#include <mln/storage/response.hpp>
#include <mln/util/run_loop.hpp>

#include "internal/support/allocation_faults.hpp"
#include "internal/support/resources.hpp"
#include "resources/custom_resource_provider.hpp"
#include "runtime/runtime.hpp"
#include "support/resources.h"
#include "support/test_support.h"

namespace {
void rejected_provider_registration_queues_no_retirement() {
  auto unexpected_retirements = std::size_t{};
  for (auto budget = std::size_t{}; budget < 24; ++budget) {
    auto runtime = mln_runtime{MLN_HANDLE_NULL};
    auto options = mln_runtime_options_default();
    MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
    auto queue = mln_delivery_queue{MLN_HANDLE_NULL};
    const auto queue_options = mln_delivery_queue_options{
      .size = sizeof(mln_delivery_queue_options),
    };
    MLN_TEST_OK(mln_delivery_queue_create(&queue_options, &queue, nullptr));
    const auto provider = mln_resource_provider{
      .size = sizeof(mln_resource_provider),
      .queue = queue,
      .routes = nullptr,
      .route_count = 0,
    };
    auto completion = mln_test_completion_default(0);
    auto status = MLN_STATUS_OK;
    {
      const auto faults = mln::native_tests::AllocationFaults{budget};
      status = mln_runtime_set_resource_provider(
        runtime, &provider, &completion.descriptor, nullptr
      );
    }
    if (status == MLN_STATUS_OK) {
      MLN_TEST_OK(mln_test_completion_settle(&completion));
    } else {
      mln_test_completion_reject(&completion);
      mln_test_completion_destroy(&completion);
      auto batch = MLN_HANDLE_NULL;
      auto view = mln_delivery_batch_view{
        .size = sizeof(mln_delivery_batch_view),
      };
      const auto drained =
        mln_delivery_queue_drain(queue, &batch, &view, nullptr);
      if (drained == MLN_STATUS_OK) {
        unexpected_retirements += view.delivery_count;
        mln_delivery_batch_release(batch);
      }
    }
    auto released = mln_test_completion_default(0);
    MLN_TEST_OK(mln_runtime_release(runtime, &released.descriptor, nullptr));
    MLN_TEST_OK(mln_test_completion_settle(&released));
    mln_delivery_queue_release(queue);
  }
  TEST_ASSERT_EQUAL_size_t_MESSAGE(
    0, unexpected_retirements, "a rejected provider queued its retirement"
  );
}

void count_release(void* data) { ++*static_cast<int*>(data); }
auto provider_callback(
  void*, const mln_resource_request_info*, mln_resource_request handle
) -> void {
  mln_test_request_pass_through(handle);
}
void transform_callback(
  void*, mln_resource_kind, const char*, mln_resource_transform_request request
) {
  mln_resource_transform_request_release(request);
}
auto header_callback(
  void*, mln_resource_kind, const char*, mln_http_header_transform_response*
) -> mln_status {
  return MLN_STATUS_OK;
}

void rejected_callback_registrations_never_release_caller_data() {
  for (auto mode = 0; mode < 3; ++mode) {
    for (auto budget = std::size_t{0}; budget < 24; ++budget) {
      auto runtime = mln_runtime{MLN_HANDLE_NULL};
      auto options = mln_runtime_options_default();
      MLN_TEST_OK(mln_runtime_create(&options, &runtime, nullptr));
      auto releases = 0;
      const auto provider = mln_resource_provider{
        .size = sizeof(mln_resource_provider),
        .callback = provider_callback,
        .user_data = &releases,
        .release_user_data = count_release,
        .queue = MLN_HANDLE_NULL,
        .routes = nullptr,
        .route_count = 0
      };
      const auto transform = mln_resource_transform{
        .size = sizeof(mln_resource_transform),
        .callback = transform_callback,
        .user_data = &releases,
        .release_user_data = count_release,
        .queue = MLN_HANDLE_NULL
      };
      const auto header = mln_http_header_transform{
        .size = sizeof(mln_http_header_transform),
        .callback = header_callback,
        .user_data = &releases,
        .release_user_data = count_release,
        .rules = nullptr,
        .rule_count = 0
      };
      auto completion = mln_test_completion_default(0);
      auto status = MLN_STATUS_OK;
      {
        const auto faults = mln::native_tests::AllocationFaults{budget};
        if (mode == 0)
          status = mln_runtime_set_resource_provider(
            runtime, &provider, &completion.descriptor, nullptr
          );
        else if (mode == 1)
          status = mln_runtime_set_resource_transform(
            runtime, &transform, &completion.descriptor, nullptr
          );
        else
          status = mln_runtime_set_http_header_transform(
            runtime, &header, &completion.descriptor, nullptr
          );
      }
      if (status == MLN_STATUS_OK)
        MLN_TEST_OK(mln_test_completion_settle(&completion));
      else {
        mln_test_completion_reject(&completion);
        mln_test_completion_destroy(&completion);
      }
      auto released = mln_test_completion_default(0);
      MLN_TEST_OK(mln_runtime_release(runtime, &released.descriptor, nullptr));
      MLN_TEST_OK(mln_test_completion_settle(&released));
      TEST_ASSERT_EQUAL_INT(status == MLN_STATUS_OK ? 1 : 0, releases);
    }
  }
}

struct PersistentCopyFailure {
  mln_resource_provider_callback callback;
  void* data;
  std::optional<mln::native_tests::AllocationFaults> faults;
};

auto refuse_until_the_wrapper_returns(
  void* data, const mln_resource_request_info* request,
  mln_resource_request handle
) -> void {
  auto& probe = *static_cast<PersistentCopyFailure*>(data);
  probe.faults.emplace();
  probe.callback(probe.data, request, handle);
}

void a_failed_provider_copy_delivers_with_persistent_allocation_failure() {
  auto loop = mln::util::RunLoop{};
  const auto runtime = mln_test_create_runtime();
  auto queue = mln_delivery_queue{MLN_HANDLE_NULL};
  const auto options =
    mln_delivery_queue_options{.size = sizeof(mln_delivery_queue_options)};
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue, nullptr));
  const auto descriptor = mln_resource_provider{
    .size = sizeof(mln_resource_provider), .queue = queue
  };
  MLN_TEST_OK(mln::native_tests::set_resource_provider(runtime, descriptor));
  const auto live = mln::core::lease_runtime(runtime);
  auto provider = mln::core::acquire_resource_provider_for_platform_context(
    live->platform_context
  );
  auto probe =
    PersistentCopyFailure{provider->callback(), provider->user_data(), {}};
  auto failed = false;
  auto passed_through = false;
  auto request = mln::core::request_custom_resource(
    mln::Resource::style("custom://persistent-copy-failure"),
    "custom://persistent-copy-failure", refuse_until_the_wrapper_returns,
    &probe,
    [&](const mln::Response& response) { failed = response.error != nullptr; },
    [&](mln::FileSource::Callback) -> std::unique_ptr<mln::AsyncRequest> {
      passed_through = true;
      return nullptr;
    }
  );
  probe.faults.reset();
  // Faults cover failure publication; native callbacks run on the next loop
  // turn.
  auto reached = false;
  loop.invoke([&] {
    reached = true;
    loop.stop();
  });
  loop.run();
  request.reset();
  provider.reset();
  MLN_TEST_OK(mln::native_tests::clear_resource_provider(runtime));
  mln_test_destroy_runtime(runtime);
  mln_delivery_queue_release(queue);
  TEST_ASSERT_TRUE(reached);
  TEST_ASSERT_FALSE(passed_through);
  TEST_ASSERT_TRUE_MESSAGE(
    failed, "the failed copy left its request without a terminal response"
  );
}

void a_queued_cancel_precedes_retirement_when_allocations_fail() {
  auto loop = mln::util::RunLoop{};
  auto handle = mln_resource_request{MLN_HANDLE_NULL};
  auto request = mln::core::request_custom_resource(
    mln::Resource::style("custom://queued-cancel"), "custom://queued-cancel",
    [](
      void* data, const mln_resource_request_info*, mln_resource_request value
    ) -> void { *static_cast<mln_resource_request*>(data) = value; },
    &handle, [](const mln::Response&) {},
    [](mln::FileSource::Callback) -> std::unique_ptr<mln::AsyncRequest> {
      return nullptr;
    }
  );
  auto queue = mln_delivery_queue{MLN_HANDLE_NULL};
  const auto options =
    mln_delivery_queue_options{.size = sizeof(mln_delivery_queue_options)};
  MLN_TEST_OK(mln_delivery_queue_create(&options, &queue, nullptr));
  const auto handler = mln_resource_request_cancel_handler{
    .size = sizeof(mln_resource_request_cancel_handler), .queue = queue
  };
  auto cancelled = false;
  MLN_TEST_OK(mln_resource_request_set_cancel_callback(
    handle, &handler, &cancelled, nullptr
  ));
  TEST_ASSERT_FALSE(cancelled);
  {
    const auto faults = mln::native_tests::AllocationFaults{};
    request.reset();
  }
  auto batch = mln_delivery_batch{MLN_HANDLE_NULL};
  auto view = mln_delivery_batch_view{.size = sizeof(mln_delivery_batch_view)};
  MLN_TEST_OK(mln_delivery_queue_drain(queue, &batch, &view, nullptr));
  TEST_ASSERT_EQUAL_size_t(2, view.delivery_count);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL, view.deliveries[0].type
  );
  TEST_ASSERT_EQUAL_UINT64(
    handle, view.deliveries[0].payload.resource_request_cancel.handle
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RETIRED, view.deliveries[1].type);
  mln_delivery_batch_release(batch);
  mln_resource_request_release(handle);
  mln_delivery_queue_release(queue);
}

void pass_through_retry_starts_native_after_scheduling_failure() {
  for (auto budget = std::size_t{0}; budget < 8; ++budget) {
    auto loop = mln::util::RunLoop{};
    auto handle = mln_resource_request{MLN_HANDLE_NULL};
    auto starts = unsigned{0};
    auto request = mln::core::request_custom_resource(
      mln::Resource::style("custom://pass-through-allocation"),
      "custom://pass-through-allocation",
      [](
        void* data, const mln_resource_request_info*, mln_resource_request value
      ) -> void { *static_cast<mln_resource_request*>(data) = value; },
      &handle, [](const mln::Response&) {},
      [&](mln::FileSource::Callback) -> std::unique_ptr<mln::AsyncRequest> {
        ++starts;
        return nullptr;
      }
    );
    auto status = MLN_STATUS_OK;
    {
      const auto faults = mln::native_tests::AllocationFaults{budget};
      status = mln_resource_request_pass_through(handle, nullptr);
    }
    if (status == MLN_STATUS_NATIVE_ERROR)
      status = mln_resource_request_pass_through(handle, nullptr);
    loop.invoke([&] { loop.stop(); });
    loop.run();
    mln_resource_request_release(handle);
    request.reset();
    MLN_TEST_OK(status);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(
      1, starts, "pass-through retry left native loading unscheduled"
    );
  }
}

void reserved_decisions_are_cancelled_with_the_native_request() {
  for (const auto pass_through : {false, true}) {
    auto loop = mln::util::RunLoop{};
    auto handle = mln_resource_request{MLN_HANDLE_NULL};
    auto callbacks = unsigned{0};
    auto starts = unsigned{0};
    auto request = mln::core::request_custom_resource(
      mln::Resource::style("custom://cancelled-decision"),
      "custom://cancelled-decision",
      [](
        void* data, const mln_resource_request_info*, mln_resource_request value
      ) -> void { *static_cast<mln_resource_request*>(data) = value; },
      &handle, [&](const mln::Response&) { ++callbacks; },
      [&](mln::FileSource::Callback) -> std::unique_ptr<mln::AsyncRequest> {
        ++starts;
        return nullptr;
      }
    );
    if (pass_through) {
      MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS([&] {
        return mln_resource_request_pass_through(handle, nullptr);
      });
    } else {
      mln_resource_request_release(handle);
    }
    request.reset();
    mln_resource_request_release(handle);
    loop.invoke([&] { loop.stop(); });
    loop.run();
    TEST_ASSERT_EQUAL_UINT(0, starts);
    TEST_ASSERT_EQUAL_UINT(0, callbacks);
  }
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(reserved_decisions_are_cancelled_with_the_native_request);
  RUN_TEST(a_failed_provider_copy_delivers_with_persistent_allocation_failure);
  RUN_TEST(pass_through_retry_starts_native_after_scheduling_failure);
  RUN_TEST(a_queued_cancel_precedes_retirement_when_allocations_fail);
  RUN_TEST(rejected_provider_registration_queues_no_retirement);
  RUN_TEST(rejected_callback_registrations_never_release_caller_data);
}
