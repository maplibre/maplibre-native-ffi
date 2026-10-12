#include <cstddef>
#include <cstdlib>
#include <new>

#include "allocation_faults.hpp"

namespace {
thread_local bool reject_allocations = false;
}  // namespace

// The replacements cover every allocation in this executable, the static
// library's included. They fail only on a thread inside an AllocationFaults
// scope.
auto operator new(std::size_t size) -> void* {
  if (reject_allocations) throw std::bad_alloc{};
  if (auto* allocation = std::malloc(size == 0 ? 1 : size)) return allocation;
  throw std::bad_alloc{};
}
auto operator new[](std::size_t size) -> void* { return ::operator new(size); }
void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept {
  std::free(allocation);
}
void operator delete[](void* allocation, std::size_t) noexcept {
  std::free(allocation);
}

namespace mln::native_tests {

AllocationFaults::AllocationFaults() noexcept { reject_allocations = true; }

AllocationFaults::~AllocationFaults() { reject_allocations = false; }

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
