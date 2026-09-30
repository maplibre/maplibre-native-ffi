#pragma once

// Waits and checks for the internal suite's C++ cases.

#include <atomic>

#include "support/wait.h"

namespace mln::native_tests {

// Waits within the default deadline until `ready()` holds. State the library
// publishes without a pulse is re-checked every few milliseconds.
template <typename Predicate>
auto await(Predicate ready, const char* what) -> bool {
  return mln_test_await(
    [](void* context) -> bool { return (*static_cast<Predicate*>(context))(); },
    &ready, mln_test_deadline_default(), what
  );
}

// Collects a failure seen on a thread that cannot fail the case itself, such
// as a library thread inside a callback, for the case to assert on afterwards.
class BackgroundChecks {
 public:
  void check(bool holds, const char* failure) noexcept {
    if (holds) return;
    auto* expected = static_cast<const char*>(nullptr);
    first_.compare_exchange_strong(expected, failure);
  }
  // The first failure, or null when every check held.
  [[nodiscard]] auto failure() const noexcept -> const char* {
    return first_.load();
  }

 private:
  std::atomic<const char*> first_{nullptr};
};

}  // namespace mln::native_tests
