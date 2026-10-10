#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "maplibre_native_c/base.h"

namespace mln::core {

// Public handles are 64-bit generational ids, laid out most significant bit
// first:
//
//   bits 63..56  kind        1..255; 0 never appears in a live handle
//   bits 55..36  index       slot within this kind's table
//   bits 35..0   generation  slot reuse counter, starting at 1
//
// A live handle always carries a nonzero kind, so 0 is the null handle for
// every type.
enum class HandleKind : std::uint8_t {
  Runtime = 1,
  Map = 2,
  MapProjection = 3,
  RenderSession = 4,
  ResourceRequest = 12,
  EventBatch = 16,
  AcquiredFrame = 20,
  RenderFrameBatch = 21,
  GeoJsonSourceData = 22,
};

inline constexpr auto handle_generation_bits = std::uint32_t{36};
inline constexpr auto handle_index_bits = std::uint32_t{20};

inline constexpr auto handle_max_generation =
  (std::uint64_t{1} << handle_generation_bits) - 1;
inline constexpr auto handle_max_index =
  (std::uint64_t{1} << handle_index_bits) - 1;

[[nodiscard]] constexpr auto encode_handle(
  HandleKind kind, std::uint64_t index, std::uint64_t generation
) noexcept -> std::uint64_t {
  return (static_cast<std::uint64_t>(kind)
          << (handle_index_bits + handle_generation_bits)) |
         (index << handle_generation_bits) | generation;
}

[[nodiscard]] constexpr auto handle_kind_of(std::uint64_t handle) noexcept
  -> std::uint8_t {
  return static_cast<std::uint8_t>(
    handle >> (handle_index_bits + handle_generation_bits)
  );
}

[[nodiscard]] constexpr auto handle_index_of(std::uint64_t handle) noexcept
  -> std::uint64_t {
  return (handle >> handle_generation_bits) & handle_max_index;
}

[[nodiscard]] constexpr auto handle_generation_of(std::uint64_t handle) noexcept
  -> std::uint64_t {
  return handle & handle_max_generation;
}

// Why a handle did not resolve.
enum class HandleFault : std::uint8_t {
  // The null handle.
  Null,
  // A value whose kind bits name no handle type.
  NotAHandle,
  // A handle of another type.
  WrongKind,
  // A value of the right type that this process never issued.
  Unknown,
  // A handle this process issued whose object has since been released.
  Stale,
};

// The status a fault reports. A stale handle once named a live object, so the
// call is out of order with that object's lifetime and reports
// MLN_STATUS_INVALID_STATE. Every other fault is a bad argument and reports
// MLN_STATUS_INVALID_ARGUMENT.
[[nodiscard]] constexpr auto handle_fault_status(HandleFault fault) noexcept
  -> mln_status {
  return fault == HandleFault::Stale ? MLN_STATUS_INVALID_STATE
                                     : MLN_STATUS_INVALID_ARGUMENT;
}

// Returns the C typedef name for a kind, or nullptr for an unregistered kind.
[[nodiscard]] auto handle_kind_name(std::uint8_t kind) noexcept -> const char*;

// Records the thread-local diagnostic and status for a handle that failed to
// resolve, and returns that status. An owner that retires a handle's object
// before the table does, such as a disposal in progress, reports the handle
// through this with HandleFault::Stale.
auto report_handle_fault(
  HandleKind expected, std::uint64_t handle, HandleFault fault
) noexcept -> mln_status;

// Returns the status of the handle fault this thread recorded last. A caller
// returns it straight after resolve(), lease(), or report_handle_fault()
// fails, so the status matches the diagnostic that failure recorded.
[[nodiscard]] auto recorded_handle_fault_status() noexcept -> mln_status;

// `issued` reports whether the table issued this handle's value at some point.
[[nodiscard]] auto classify_handle_fault(
  HandleKind expected, std::uint64_t handle, bool issued
) noexcept -> HandleFault;

// Declared once per handle object type, next to that type's definition.
//
//   template <> struct HandleTraits<MapObject> {
//     static constexpr HandleKind kind = HandleKind::Map;
//     static constexpr bool leasable = false;
//   };
template <typename Object>
struct HandleTraits;

// Thrown when a kind's index space is full.
class HandleTableExhausted final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// A per-kind slot table mapping generational ids to owned objects.
//
// Locking contract: no entry point holds two handle-table mutexes at once.
// There is no ordering rule between tables.
template <typename Object>
class HandleTable {
 public:
  using Traits = HandleTraits<Object>;

  HandleTable() = default;
  HandleTable(const HandleTable&) = delete;
  HandleTable(HandleTable&&) = delete;
  auto operator=(const HandleTable&) -> HandleTable& = delete;
  auto operator=(HandleTable&&) -> HandleTable& = delete;
  ~HandleTable() = default;

  // Guards this table. Callers that must act on a resolved object without the
  // handle being retired in between hold this across the check and the act.
  [[nodiscard]] auto mutex() const noexcept -> std::mutex& { return mutex_; }

  auto insert(std::shared_ptr<Object> object) -> std::uint64_t {
    const std::scoped_lock lock(mutex_);
    if (!free_indices_.empty()) {
      const auto index = free_indices_.back();
      free_indices_.pop_back();
      auto& slot = slots_.at(index);
      slot.object = std::move(object);
      return encode_handle(Traits::kind, index, slot.generation);
    }
    if (slots_.size() > handle_max_index) {
      throw HandleTableExhausted{"handle table is full"};
    }
    // Reserve recycling capacity before publishing ownership. Retiring any
    // existing handle can then return its slot without allocating.
    if (free_indices_.capacity() < slots_.size() + 1) {
      free_indices_.reserve(
        std::max(slots_.size() + 1, free_indices_.capacity() * 2)
      );
    }
    const auto index = static_cast<std::uint64_t>(slots_.size());
    slots_.push_back(Slot{.generation = 1, .object = std::move(object)});
    return encode_handle(Traits::kind, index, 1);
  }

  // Borrows the object a handle names, or returns nullptr after recording the
  // thread-local diagnostic and status.
  //
  // The returned pointer outlives this call's lock, so the caller must be on a
  // thread that cannot concurrently retire the handle. A foreign thread holds
  // mutex() and uses resolve_locked(), or uses lease().
  [[nodiscard]] auto resolve(std::uint64_t handle) const -> Object* {
    const std::scoped_lock lock(mutex_);
    return resolve_locked(handle);
  }

  [[nodiscard]] auto resolve_locked(std::uint64_t handle) const -> Object* {
    auto* object = try_resolve_locked(handle);
    if (object == nullptr) {
      static_cast<void>(
        report_handle_fault(Traits::kind, handle, fault_for(handle))
      );
    }
    return object;
  }

  // Same lookup without touching thread-local diagnostics. Use this from a
  // MapLibre worker thread or a deferred callback, where writing an error would
  // clobber the diagnostic of an unrelated entry point on the same stack.
  [[nodiscard]] auto try_resolve(std::uint64_t handle) const noexcept
    -> Object* {
    const std::scoped_lock lock(mutex_);
    return try_resolve_locked(handle);
  }

  [[nodiscard]] auto try_resolve_locked(std::uint64_t handle) const noexcept
    -> Object* {
    const auto* slot = find_slot(handle);
    return slot == nullptr ? nullptr : slot->object.get();
  }

  // Keeps the object readable after this table's lock is released. Available
  // only for kinds whose teardown tolerates running on a foreign thread.
  [[nodiscard]] auto lease(std::uint64_t handle) const
    -> std::shared_ptr<Object>
    requires(Traits::leasable)
  {
    const std::scoped_lock lock(mutex_);
    return lease_locked(handle);
  }

  // The same lease for a caller that already holds mutex().
  [[nodiscard]] auto lease_locked(std::uint64_t handle) const
    -> std::shared_ptr<Object>
    requires(Traits::leasable)
  {
    const auto* slot = find_slot(handle);
    if (slot == nullptr) {
      static_cast<void>(
        report_handle_fault(Traits::kind, handle, fault_for(handle))
      );
      return nullptr;
    }
    return slot->object;
  }

  [[nodiscard]] auto try_lease(std::uint64_t handle) const noexcept
    -> std::shared_ptr<Object>
    requires(Traits::leasable)
  {
    const std::scoped_lock lock(mutex_);
    const auto* slot = find_slot(handle);
    return slot == nullptr ? nullptr : slot->object;
  }

  // Retires a handle and returns what it named, or nullptr when it did not
  // resolve. The slot's generation advances before this returns, so every
  // outstanding copy of the handle is stale from here on.
  auto remove(std::uint64_t handle) -> std::shared_ptr<Object> {
    const std::scoped_lock lock(mutex_);
    return remove_locked(handle);
  }

  auto remove_locked(std::uint64_t handle) -> std::shared_ptr<Object> {
    if (find_slot(handle) == nullptr) {
      return nullptr;
    }
    const auto index = handle_index_of(handle);
    auto& slot = slots_[index];
    auto object = std::move(slot.object);
    slot.object.reset();
    ++slot.generation;
    // A slot whose generation is exhausted is retired instead of recycled, so
    // a handle value is never reused.
    if (slot.generation <= handle_max_generation) {
      free_indices_.push_back(static_cast<std::uint32_t>(index));
    }
    return object;
  }

 private:
  // `generation` is the one the slot issues next, or the one it issued last
  // while `object` is set. Every lower generation was issued and retired. A
  // retired slot's generation exceeds handle_max_generation.
  struct Slot {
    std::uint64_t generation = 1;
    std::shared_ptr<Object> object;
  };

  [[nodiscard]] auto find_slot(std::uint64_t handle) const noexcept
    -> const Slot* {
    if (handle_kind_of(handle) != static_cast<std::uint8_t>(Traits::kind)) {
      return nullptr;
    }
    const auto index = handle_index_of(handle);
    if (index >= slots_.size()) {
      return nullptr;
    }
    const auto& slot = slots_[index];
    if (
      slot.object == nullptr || slot.generation != handle_generation_of(handle)
    ) {
      return nullptr;
    }
    return &slot;
  }

  [[nodiscard]] auto fault_for(std::uint64_t handle) const noexcept
    -> HandleFault {
    const auto index = handle_index_of(handle);
    const auto generation = handle_generation_of(handle);
    const auto issued = index < slots_.size() && generation != 0 &&
                        generation < slots_[index].generation;
    return classify_handle_fault(Traits::kind, handle, issued);
  }

  mutable std::mutex mutex_;
  std::vector<Slot> slots_;
  std::vector<std::uint32_t> free_indices_;
};

// Intentionally leaked because destroying live handles during C++ static
// teardown can use resources that have already been destroyed.
template <typename Object>
auto handle_table() -> HandleTable<Object>& {
  static auto* const value = new HandleTable<Object>{};
  return *value;
}

}  // namespace mln::core
