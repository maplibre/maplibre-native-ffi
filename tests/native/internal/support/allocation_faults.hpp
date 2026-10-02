#pragma once

// Allocation fault injection for the internal suite. The suite links the
// static library, so its replacement operator new covers the library's own
// allocations without changing the shipped library.

#include "maplibre_native_c.h"
#include "unity.h"

namespace mln::native_tests {

// While a scope is live, every operator new on the constructing thread throws
// std::bad_alloc. Other threads, including the library's own, allocate as
// usual, so native cleanup elsewhere may still allocate.
class AllocationFaults {
 public:
  AllocationFaults() noexcept;
  ~AllocationFaults();
  AllocationFaults(const AllocationFaults&) = delete;
  auto operator=(const AllocationFaults&) -> AllocationFaults& = delete;

  // Whether an allocation on this thread fails right now, which proves the
  // replacement took effect on this platform.
  [[nodiscard]] static auto active() noexcept -> bool;
};

// Runs `work` with this thread's allocations failing, and reports whether the
// injection took effect and `work` returned MLN_STATUS_OK. Safe on a library
// thread, where a case cannot assert.
template <typename Work>
auto ok_without_allocations(Work work) -> bool {
  const auto faults = AllocationFaults{};
  return AllocationFaults::active() && work() == MLN_STATUS_OK;
}

}  // namespace mln::native_tests

// Asserts that `work` succeeds with this thread's allocations failing.
#define MLN_TEST_ASSERT_OK_WITHOUT_ALLOCATIONS(work)                       \
  TEST_ASSERT_TRUE_MESSAGE(                                                \
    mln::native_tests::ok_without_allocations(work),                       \
    "the call failed with allocations disabled, or injection was inactive" \
  )
