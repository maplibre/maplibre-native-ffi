// mln::core::Completion, the state machine behind every asynchronous
// completion: delivery once, in order, and only after acceptance.

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "completion/completion.hpp"

#include "maplibre_native_c.h"
#include "support/harness.h"
#include "support/status.h"
#include "unity.h"

namespace {

// Counts what one completion descriptor did, and in which order.
struct CompletionProbe {
  std::atomic_uint calls = 0;
  std::atomic_uint releases = 0;
  // 0 before delivery, 1 after the callback, 2 after the release, -1 once
  // either arrived out of order.
  std::atomic_int phase = 0;
  std::atomic_int status = MLN_STATUS_INVALID_STATE;
};

void record_completion(
  void* user_data, const mln_completion_result* result
) noexcept {
  auto& probe = *static_cast<CompletionProbe*>(user_data);
  auto expected = 0;
  if (!probe.phase.compare_exchange_strong(expected, 1)) {
    probe.phase.store(-1);
  }
  probe.status.store(result->status);
  probe.calls.fetch_add(1);
}

void record_release(void* user_data) noexcept {
  auto& probe = *static_cast<CompletionProbe*>(user_data);
  auto expected = 1;
  if (!probe.phase.compare_exchange_strong(expected, 2)) {
    probe.phase.store(-1);
  }
  probe.releases.fetch_add(1);
}

auto descriptor_for(CompletionProbe& probe) -> mln_completion {
  return mln_completion{
    .size = sizeof(mln_completion),
    .callback = record_completion,
    .user_data = &probe,
    .release_user_data = record_release,
  };
}

// A completion resolved before its acceptance holds the result until accept
// runs, then delivers it exactly once and releases the user data after.
void inline_resolution_waits_for_acceptance() {
  auto probe = CompletionProbe{};
  auto completion =
    std::make_shared<mln::core::Completion>(descriptor_for(probe));
  mln::core::complete_value(
    completion, MLN_STATUS_OK, std::string{}, std::uint32_t{7}
  );
  TEST_ASSERT_EQUAL_UINT(0, probe.calls.load());

  completion->accept();
  completion->resolve([](const mln_completion&) {});
  TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
  TEST_ASSERT_EQUAL_INT(2, probe.phase.load());
  MLN_TEST_OK(probe.status.load());
}

// A rejected completion leaves the user data with the caller and delivers
// nothing.
void rejection_leaves_the_user_data() {
  auto probe = CompletionProbe{};
  {
    auto completion =
      std::make_shared<mln::core::Completion>(descriptor_for(probe));
    completion->reject();
  }
  TEST_ASSERT_EQUAL_UINT(0, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(0, probe.releases.load());
}

// An accepted completion nothing resolves reports MLN_STATUS_CANCELLED when it
// is destroyed.
void abandonment_reports_cancelled() {
  auto probe = CompletionProbe{};
  {
    auto completion =
      std::make_shared<mln::core::Completion>(descriptor_for(probe));
    completion->accept();
  }
  TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
  TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, probe.status.load());
}

// Acceptance and resolution racing on two threads still deliver once, with the
// release after the callback.
void acceptance_and_resolution_race_once() {
  for (auto iteration = 0; iteration < 100; ++iteration) {
    auto probe = CompletionProbe{};
    auto completion =
      std::make_shared<mln::core::Completion>(descriptor_for(probe));
    auto accept = std::thread{[completion]() { completion->accept(); }};
    auto resolve = std::thread{[completion]() {
      mln::core::complete(completion, MLN_STATUS_OK);
    }};
    accept.join();
    resolve.join();
    TEST_ASSERT_EQUAL_UINT(1, probe.calls.load());
    TEST_ASSERT_EQUAL_UINT(1, probe.releases.load());
    TEST_ASSERT_EQUAL_INT(2, probe.phase.load());
  }
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(inline_resolution_waits_for_acceptance);
  RUN_TEST(rejection_leaves_the_user_data);
  RUN_TEST(abandonment_reports_cancelled);
  RUN_TEST(acceptance_and_resolution_race_once);
}
