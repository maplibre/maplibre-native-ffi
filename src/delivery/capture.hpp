#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "delivery/dispose.hpp"
#include "maplibre_native_c.h"

namespace mln::delivery {

// A present empty span keeps its presence after its source storage expires.
// No element exists or may be read through this suitably aligned sentinel.
template <class T>
struct alignas(T) EmptyStorage {
  std::byte bytes[sizeof(T)];
};

template <class T>
inline EmptyStorage<T> empty_storage{};

template <class T>
auto empty_pointer() noexcept -> T* {
  return reinterpret_cast<T*>(empty_storage<T>.bytes);
}

// Copies run twice over the same generated code. The measuring pass validates
// sizes and pointers and sums the bytes that the copy needs; the writing pass
// copies into one allocation of exactly that size, after the header that
// starts it.
template <bool Writing>
struct Arena {
  std::byte* storage = nullptr;
  std::size_t offset = 0;
  std::size_t capacity = std::numeric_limits<std::size_t>::max();

  template <class T>
  auto allocate(std::size_t count) -> T* {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(alignof(T) <= alignof(std::max_align_t));
    if (count == 0) return nullptr;
    constexpr auto alignment = alignof(T);
    const auto padding = (alignment - offset % alignment) % alignment;
    if (padding > capacity - offset) throw std::bad_alloc{};
    offset += padding;
    if (count > (capacity - offset) / sizeof(T)) throw std::bad_alloc{};
    const auto start = offset;
    offset += count * sizeof(T);
    if constexpr (Writing) return reinterpret_cast<T*>(storage + start);
    return nullptr;
  }
};

template <bool Writing, class T, class Convert>
auto span(
  Arena<Writing>& arena, const T* source, std::size_t count, Convert convert
) -> T* {
  if (count == 0) return source == nullptr ? nullptr : empty_pointer<T>();
  if (source == nullptr) throw std::invalid_argument{"null span"};
  auto* destination = arena.template allocate<T>(count);
  for (std::size_t index = 0; index < count; ++index) {
    const auto copied = convert(arena, source[index]);
    if constexpr (Writing) destination[index] = copied;
  }
  return destination;
}

template <bool Writing, class T>
auto bytes(Arena<Writing>& arena, const T* source, std::size_t count) -> T* {
  if (count == 0) return source == nullptr ? nullptr : empty_pointer<T>();
  if (source == nullptr) throw std::invalid_argument{"null bytes"};
  auto* destination = arena.template allocate<T>(count);
  if constexpr (Writing) std::memcpy(destination, source, count * sizeof(T));
  return destination;
}

template <bool Writing>
auto buffer(Arena<Writing>& arena, mln_buffer_view value) -> mln_buffer_view {
  value.data =
    bytes(arena, static_cast<const std::byte*>(value.data), value.size);
  return value;
}

template <bool Writing>
auto string(Arena<Writing>& arena, const char* value) -> const char* {
  if (value == nullptr) return nullptr;
  const auto length = std::strlen(value);
  if (length == std::numeric_limits<std::size_t>::max()) throw std::bad_alloc{};
  return bytes(arena, value, length + 1);
}

// The record that each delivery type copies from a delivered callback's
// arguments, and the mln_delivery_payload member that holds it.
template <mln_delivery_type Type>
struct Delivered;

// The generated copies: capture_element(), discard_element(), and
// element_owns_handles() for each completion value type and each delivered
// callback's record, and a Delivered specialization for each delivered
// callback. A value type without an overload fails to compile where it is
// delivered.
#include "delivery/capture_generated.inc"

}  // namespace mln::delivery
