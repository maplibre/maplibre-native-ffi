#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>

#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"

namespace mln::capture {

struct Record {
  mln_adapter_completion_record view{};
  std::uint32_t kind = 0;
  bool claimed = false;
};

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

// Both passes traverse the same generated code. The first validates sizes and
// pointers; the second writes into a single allocation owned by Record.
template <bool Writing>
struct Arena {
  std::byte* storage = nullptr;
  std::size_t offset = sizeof(Record);
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
  if (source == nullptr) throw std::invalid_argument{"null completion span"};
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
  if (source == nullptr) throw std::invalid_argument{"null completion bytes"};
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

// One deferred call and the storage that follows it in the same allocation.
struct DeferredRecord {
  mln_adapter_deferred_call_record view{};
  bool claimed = false;
};

// Hands a copied call to the listener of a deferred callback context. Returns
// false, leaving the record with the caller, when context does not defer kind.
auto deliver_deferred(
  void* context, std::uint32_t kind, DeferredRecord* record
) noexcept -> bool;

// Copies one call's arguments into a single allocation and delivers it. The
// generated adapter returns the callback's failure result when this returns
// false.
template <class Arguments, class Capture>
auto defer(
  void* context, std::uint32_t kind, const Arguments& source, Capture capture
) noexcept -> bool {
  static_assert(std::is_trivially_destructible_v<DeferredRecord>);
  std::byte* storage = nullptr;
  try {
    Arena<false> measure{nullptr, sizeof(DeferredRecord)};
    static_cast<void>(measure.template allocate<Arguments>(1));
    static_cast<void>(capture(measure, source));
    storage = static_cast<std::byte*>(::operator new(measure.offset));
    auto* record = new (storage) DeferredRecord{};
    Arena<true> write{storage, sizeof(DeferredRecord), measure.offset};
    auto* arguments = write.template allocate<Arguments>(1);
    *arguments = capture(write, source);
    record->view = {record, kind, arguments};
    if (deliver_deferred(context, kind, record)) return true;
  } catch (...) {
  }
  ::operator delete(storage);
  return false;
}

#include "c_api/callback_capture_generated.inc"

inline auto destroy_deferred(DeferredRecord* record) noexcept -> void {
  if (record == nullptr) return;
  if (!record->claimed)
    deferred_discard(record->view.callback, record->view.arguments);
  ::operator delete(record);
}

inline auto copy(const mln_completion_result& source, std::uint32_t kind)
  -> Record* {
  Arena<false> measure;
  static_cast<void>(buffer(measure, source.diagnostic));
  if (source.status == MLN_STATUS_OK) {
    static_cast<void>(value(measure, source, kind));
  }
  auto* storage = static_cast<std::byte*>(::operator new(measure.offset));
  auto* record = new (storage) Record{};
  try {
    Arena<true> write{storage, sizeof(Record), measure.offset};
    record->kind = kind;
    record->view.owner = record;
    record->view.result = source;
    record->view.result.diagnostic = buffer(write, source.diagnostic);
    record->view.result.value = nullptr;
    if (source.status == MLN_STATUS_OK) {
      record->view.result.value = value(write, source, kind);
    } else {
      record->view.result.value_count = 0;
    }
    return record;
  } catch (...) {
    record->~Record();
    ::operator delete(storage);
    throw;
  }
}

inline auto destroy(Record* record) noexcept -> void {
  if (record == nullptr) return;
  if (!record->claimed) discard(record->kind, record->view.result);
  record->~Record();
  ::operator delete(record);
}

}  // namespace mln::capture
