#include <cstddef>
#include <cstdlib>
#include <new>

#include "allocation_faults.hpp"

namespace {
thread_local bool reject_allocations = false;
thread_local std::size_t allocations_before_failure = 0;
thread_local bool fail_one_allocation = false;
thread_local void (*allocation_failure_hook)(void*) = nullptr;
thread_local void* allocation_failure_data = nullptr;
}  // namespace

// The replacements cover every allocation in this executable, the static
// library's included. They fail only on a thread inside an AllocationFaults
// scope.
auto operator new(std::size_t size) -> void* {
  if (reject_allocations) throw std::bad_alloc{};
  if (fail_one_allocation) {
    if (allocations_before_failure == 0) {
      fail_one_allocation = false;
      if (allocation_failure_hook != nullptr) {
        allocation_failure_hook(allocation_failure_data);
      }
      throw std::bad_alloc{};
    }
    --allocations_before_failure;
  }
  if (auto* allocation = std::malloc(size == 0 ? 1 : size)) return allocation;
  throw std::bad_alloc{};
}
auto operator new[](std::size_t size) -> void* { return ::operator new(size); }
auto operator new(std::size_t size, const std::nothrow_t&) noexcept -> void* {
  try {
    return ::operator new(size);
  } catch (...) {
    return nullptr;
  }
}
auto operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
  -> void* {
  return ::operator new(size, tag);
}
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept {
  std::free(allocation);
}
void operator delete[](void* allocation, std::size_t) noexcept {
  std::free(allocation);
}
void operator delete(void* allocation, const std::nothrow_t&) noexcept {
  ::operator delete(allocation);
}
void operator delete[](void* allocation, const std::nothrow_t&) noexcept {
  ::operator delete(allocation);
}

namespace mln::native_tests {

AllocationFaults::AllocationFaults() noexcept { reject_allocations = true; }

AllocationFaults::AllocationFaults(
  std::size_t successful_allocations, void (*before_failure)(void*),
  void* user_data
) noexcept {
  allocations_before_failure = successful_allocations;
  fail_one_allocation = true;
  allocation_failure_hook = before_failure;
  allocation_failure_data = user_data;
}

AllocationFaults::~AllocationFaults() {
  reject_allocations = false;
  fail_one_allocation = false;
  allocation_failure_hook = nullptr;
  allocation_failure_data = nullptr;
}

auto AllocationFaults::active() noexcept -> bool {
  try {
    auto* unexpected = ::operator new(1);
    ::operator delete(unexpected);
    return false;
  } catch (const std::bad_alloc&) {
    return true;
  }
}

}  // namespace mln::native_tests
