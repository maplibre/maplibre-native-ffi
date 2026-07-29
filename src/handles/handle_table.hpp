#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mln::core {

// Public handles are 64-bit generational ids rather than object addresses.
// Keying a registry on a pointer cannot prove identity: the allocator may hand
// a later object the address a retired handle still names, so a stale handle
// either misses the registry or binds the wrong object. Packing a per-slot
// generation into the handle makes that second outcome impossible, because a
// retired handle's generation never matches the slot again.
//
// Layout, most significant bit first:
//
//   bits 63..56  kind        1..255; 0 never appears in a live handle
//   bits 55..36  index       slot within this kind's table
//   bits 35..0   generation  slot reuse counter, starting at 1
//
// A live handle always carries a nonzero kind, so the value 0 is the null
// handle for every type and cannot collide with a real one.
enum class HandleKind : std::uint8_t {
  Runtime = 1,
  Map = 2,
  MapProjection = 3,
  RenderSession = 4,
  OfflineRegionSnapshot = 5,
  OfflineRegionList = 6,
  JsonSnapshot = 7,
  StyleIdList = 8,
  FeatureQueryResult = 9,
  FeatureExtensionResult = 10,
  WakeSource = 11,
  ResourceRequest = 12,
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

// Why a well-formed handle did not resolve. Every fault reports
// MLN_STATUS_INVALID_ARGUMENT; the distinction lives in the message so a host
// can tell a released handle from a mismatched or forged one.
enum class HandleFault : std::uint8_t {
  Null,
  NotAHandle,
  WrongKind,
  Unknown,
  Stale,
};

// Spells the C typedef name for a kind, or nullptr when the byte names no
// registered kind.
[[nodiscard]] auto handle_kind_name(std::uint8_t kind) noexcept -> const char*;

// Records the thread-local diagnostic for a handle that failed to resolve.
auto set_handle_fault_error(
  HandleKind expected, std::uint64_t handle, HandleFault fault
) noexcept -> void;

// Classifies a handle that failed to resolve, given whether its index fell
// within the table's high-water mark.
[[nodiscard]] auto classify_handle_fault(
  HandleKind expected, std::uint64_t handle, bool index_in_range
) noexcept -> HandleFault;

// Declared once per handle object type, next to that type's definition.
//
//   template <> struct HandleTraits<MapObject> {
//     static constexpr HandleKind kind = HandleKind::Map;
//     static constexpr bool leasable = false;
//   };
template <typename Object>
struct HandleTraits;

// Thrown when a kind's index space is full. Every insert site sits inside
// mln::c_api::status_boundary, which reports it as MLN_STATUS_NATIVE_ERROR.
class HandleTableExhausted final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// A per-kind slot table mapping generational ids to owned objects.
//
// Locking contract: no entry point holds two handle-table mutexes at once.
// Keep it that way — the tables are independent, and a second edge would need
// an ordering rule the rest of the C API does not have.
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
    return insert_locked(std::move(object));
  }

  auto insert_locked(std::shared_ptr<Object> object) -> std::uint64_t {
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
    const auto index = static_cast<std::uint64_t>(slots_.size());
    slots_.push_back(Slot{.generation = 1, .object = std::move(object)});
    return encode_handle(Traits::kind, index, 1);
  }

  // Borrows the object a handle names, or returns nullptr after recording the
  // thread-local diagnostic.
  //
  // The returned pointer outlives this call's lock, so callers must be on a
  // thread that cannot concurrently retire the handle. That is exactly the
  // owner-thread contract every thread-affine type already carries. Callers on
  // a foreign thread hold mutex() and use resolve_locked(), or lease().
  [[nodiscard]] auto resolve(std::uint64_t handle) const -> Object* {
    const std::scoped_lock lock(mutex_);
    return resolve_locked(handle);
  }

  [[nodiscard]] auto resolve_locked(std::uint64_t handle) const -> Object* {
    auto* object = try_resolve_locked(handle);
    if (object == nullptr) {
      set_handle_fault_error(Traits::kind, handle, fault_for(handle));
    }
    return object;
  }

  // Same lookup without touching thread-local diagnostics, for code reachable
  // from a MapLibre worker thread or a deferred callback, where writing an
  // error would clobber the diagnostic of an unrelated entry point on the same
  // stack.
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
  // only for kinds whose teardown tolerates running on a foreign thread, so a
  // lease can never move an owner-thread object's destructor off its owner.
  [[nodiscard]] auto lease(std::uint64_t handle) const
    -> std::shared_ptr<Object>
    requires(Traits::leasable)
  {
    const std::scoped_lock lock(mutex_);
    const auto* slot = find_slot(handle);
    if (slot == nullptr) {
      set_handle_fault_error(Traits::kind, handle, fault_for(handle));
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
    if (slot.generation < handle_max_generation) {
      ++slot.generation;
      free_indices_.push_back(static_cast<std::uint32_t>(index));
    }
    // A slot whose generation is exhausted is retired instead of recycled, so
    // a handle value is never reused even in the limit.
    return object;
  }

  [[nodiscard]] auto contains(std::uint64_t handle) const noexcept -> bool {
    const std::scoped_lock lock(mutex_);
    return find_slot(handle) != nullptr;
  }

 private:
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
    return classify_handle_fault(
      Traits::kind, handle, handle_index_of(handle) < slots_.size()
    );
  }

  mutable std::mutex mutex_;
  std::vector<Slot> slots_;
  std::vector<std::uint32_t> free_indices_;
};

// One process-global table per object type. The whole C API links into a
// single target, so the function-local static is unique.
template <typename Object>
auto handle_table() -> HandleTable<Object>& {
  static auto value = HandleTable<Object>{};
  return value;
}

}  // namespace mln::core
