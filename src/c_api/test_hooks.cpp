#define MLN_BUILDING_C

#include "c_api/test_hooks.hpp"

#if defined(MLN_FFI_ENABLE_TEST_HOOKS)

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "completion/completion.hpp"
#include "render/render_session_common.hpp"

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
auto inline_resolution_waits_for_acceptance() -> const char* {
  auto probe = CompletionProbe{};
  auto completion =
    std::make_shared<mln::core::Completion>(descriptor_for(probe));
  mln::core::complete_value(
    completion, MLN_STATUS_OK, std::string{}, std::uint32_t{7}
  );
  if (probe.calls.load() != 0) {
    return "a completion resolved before acceptance delivered early";
  }
  completion->accept();
  completion->resolve([](const mln_completion&) {});
  if (probe.calls.load() != 1) return "acceptance did not deliver exactly once";
  if (probe.releases.load() != 1) return "acceptance did not release once";
  if (probe.phase.load() != 2) return "the release did not follow the callback";
  if (probe.status.load() != MLN_STATUS_OK) {
    return "the delivered status was not the resolved one";
  }
  return nullptr;
}

// A rejected completion leaves the user data with the caller and delivers
// nothing.
auto rejection_leaves_the_user_data() -> const char* {
  auto probe = CompletionProbe{};
  {
    auto completion =
      std::make_shared<mln::core::Completion>(descriptor_for(probe));
    completion->reject();
  }
  if (probe.calls.load() != 0) return "a rejected completion delivered";
  if (probe.releases.load() != 0) {
    return "a rejected completion released the caller's user data";
  }
  return nullptr;
}

// An accepted completion nothing resolves reports MLN_STATUS_CANCELLED when it
// is destroyed.
auto abandonment_reports_cancelled() -> const char* {
  auto probe = CompletionProbe{};
  {
    auto completion =
      std::make_shared<mln::core::Completion>(descriptor_for(probe));
    completion->accept();
  }
  if (probe.calls.load() != 1) return "an abandoned completion did not deliver";
  if (probe.releases.load() != 1) {
    return "an abandoned completion did not release once";
  }
  if (probe.status.load() != MLN_STATUS_CANCELLED) {
    return "an abandoned completion did not report MLN_STATUS_CANCELLED";
  }
  return nullptr;
}

// Acceptance and resolution racing on two threads still deliver once.
auto acceptance_and_resolution_race_once() -> const char* {
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
    if (probe.calls.load() != 1) {
      return "a raced completion did not deliver exactly once";
    }
    if (probe.releases.load() != 1) {
      return "a raced completion did not release exactly once";
    }
    if (probe.phase.load() != 2) {
      return "a raced release did not follow the callback";
    }
  }
  return nullptr;
}

}  // namespace

extern "C" auto mln_test_hook_completion_contract(void) -> const char* {
  if (const auto* failure = inline_resolution_waits_for_acceptance()) {
    return failure;
  }
  if (const auto* failure = rejection_leaves_the_user_data()) return failure;
  if (const auto* failure = abandonment_reports_cancelled()) return failure;
  return acceptance_and_resolution_race_once();
}

extern "C" auto mln_test_hook_enqueue_blocking_render_operation(
  mln_render_session session, std::atomic_bool* entered,
  const std::atomic_bool* release, const mln_completion* completion
) -> mln_status {
  return mln::core::enqueue_blocking_test_render_operation(
    session, entered, release, completion
  );
}

#endif  // MLN_FFI_ENABLE_TEST_HOOKS
