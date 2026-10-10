#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <mln/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_queue_access.hpp"

namespace {

// The functions a session's dispatcher resolved before install_queue_access()
// replaced them.
struct QueueFunctions {
  PFN_vkQueueSubmit queue_submit = nullptr;
  PFN_vkQueuePresentKHR queue_present = nullptr;
  PFN_vkCreateFence create_fence = nullptr;
  PFN_vkWaitForFences wait_for_fences = nullptr;
  PFN_vkDestroyFence destroy_fence = nullptr;
};

// One session's registration. Only that session's dispatcher reaches the
// slot's entry points, and the slot lasts while its backend can call them.
struct Slot {
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  QueueFunctions functions;
  // Held for each native submission or present on the queue, and never across
  // a wait. Every session registered on the queue shares it.
  std::shared_ptr<std::mutex> submit_mutex;
};

// More Vulkan sessions than any process keeps attached at once.
constexpr std::size_t slot_count = 128;

struct Registry {
  std::mutex mutex;
  std::array<std::optional<Slot>, slot_count> slots;
  // The submission lock of each queue that a live slot names.
  std::unordered_map<VkQueue, std::weak_ptr<std::mutex>> submit_mutexes;
};

auto registry() -> Registry& {
  // Leaked, so a driver thread still tearing a session down while the process
  // exits never finds the registry destroyed.
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  static auto* instance = new Registry();
  return *instance;
}

auto find_slot(std::size_t index) -> std::optional<Slot> {
  auto& state = registry();
  const auto lock = std::scoped_lock{state.mutex};
  return state.slots.at(index);
}

// Waits for everything submitted to the slot's queue so far, without holding
// its lock during the wait.
auto drain_queue(const Slot& slot) -> VkResult {
  const auto& functions = slot.functions;
  const auto create_info = VkFenceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
  };
  VkFence fence = VK_NULL_HANDLE;
  auto result =
    functions.create_fence(slot.device, &create_info, nullptr, &fence);
  if (result != VK_SUCCESS) {
    return result;
  }
  {
    const auto lock = std::scoped_lock{*slot.submit_mutex};
    // An empty batch still signals its fence, once all earlier work on the
    // queue has completed.
    result = functions.queue_submit(slot.queue, 0, nullptr, fence);
  }
  if (result == VK_SUCCESS) {
    result = functions.wait_for_fences(
      slot.device, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()
    );
  }
  functions.destroy_fence(slot.device, fence, nullptr);
  return result;
}

// A session's dispatcher reaches these only through its own slot, and mbgl
// names only the device and queue it was given, so a missing slot or another
// handle means a broken invariant rather than a call to pass through.
constexpr auto unregistered_result = VK_ERROR_INITIALIZATION_FAILED;

auto slot_queue_submit(
  std::size_t index, VkQueue queue, uint32_t submit_count,
  const VkSubmitInfo* submits, VkFence fence
) -> VkResult {
  const auto slot = find_slot(index);
  if (!slot || slot->queue != queue) {
    return unregistered_result;
  }
  const auto lock = std::scoped_lock{*slot->submit_mutex};
  return slot->functions.queue_submit(queue, submit_count, submits, fence);
}

auto slot_queue_present(
  std::size_t index, VkQueue queue, const VkPresentInfoKHR* present_info
) -> VkResult {
  const auto slot = find_slot(index);
  if (
    !slot || slot->queue != queue || slot->functions.queue_present == nullptr
  ) {
    return unregistered_result;
  }
  const auto lock = std::scoped_lock{*slot->submit_mutex};
  return slot->functions.queue_present(queue, present_info);
}

auto slot_queue_wait_idle(std::size_t index, VkQueue queue) -> VkResult {
  const auto slot = find_slot(index);
  if (!slot || slot->queue != queue) {
    return unregistered_result;
  }
  return drain_queue(*slot);
}

auto slot_device_wait_idle(std::size_t index, VkDevice device) -> VkResult {
  const auto slot = find_slot(index);
  if (!slot || slot->device != device) {
    return unregistered_result;
  }
  return drain_queue(*slot);
}

template <std::size_t Index>
struct SlotEntryPoints {
  static VKAPI_ATTR auto VKAPI_CALL queue_submit(
    VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,
    VkFence fence
  ) -> VkResult {
    return slot_queue_submit(Index, queue, submit_count, submits, fence);
  }

  static VKAPI_ATTR auto VKAPI_CALL
  queue_present(VkQueue queue, const VkPresentInfoKHR* present_info)
    -> VkResult {
    return slot_queue_present(Index, queue, present_info);
  }

  static VKAPI_ATTR auto VKAPI_CALL queue_wait_idle(VkQueue queue) -> VkResult {
    return slot_queue_wait_idle(Index, queue);
  }

  static VKAPI_ATTR auto VKAPI_CALL device_wait_idle(VkDevice device)
    -> VkResult {
    return slot_device_wait_idle(Index, device);
  }
};

struct EntryPoints {
  PFN_vkQueueSubmit queue_submit;
  PFN_vkQueuePresentKHR queue_present;
  PFN_vkQueueWaitIdle queue_wait_idle;
  PFN_vkDeviceWaitIdle device_wait_idle;
};

template <std::size_t... Indices>
constexpr auto make_entry_points(std::index_sequence<Indices...> /*indices*/)
  -> std::array<EntryPoints, sizeof...(Indices)> {
  return {EntryPoints{
    .queue_submit = &SlotEntryPoints<Indices>::queue_submit,
    .queue_present = &SlotEntryPoints<Indices>::queue_present,
    .queue_wait_idle = &SlotEntryPoints<Indices>::queue_wait_idle,
    .device_wait_idle = &SlotEntryPoints<Indices>::device_wait_idle,
  }...};
}

constexpr auto entry_points =
  make_entry_points(std::make_index_sequence<slot_count>{});

}  // namespace

namespace mln::core {

void VulkanQueueAccess::install_queue_access(
  mln::vulkan::DispatchLoaderDynamic& dispatcher, VkDevice device, VkQueue queue
) {
  const auto functions = QueueFunctions{
    .queue_submit = dispatcher.vkQueueSubmit,
    .queue_present = dispatcher.vkQueuePresentKHR,
    .create_fence = dispatcher.vkCreateFence,
    .wait_for_fences = dispatcher.vkWaitForFences,
    .destroy_fence = dispatcher.vkDestroyFence,
  };
  if (
    slot_ != no_slot || device == VK_NULL_HANDLE || queue == VK_NULL_HANDLE ||
    functions.queue_submit == nullptr || functions.create_fence == nullptr ||
    functions.wait_for_fences == nullptr || functions.destroy_fence == nullptr
  ) {
    return;
  }

  {
    auto& state = registry();
    const auto lock = std::scoped_lock{state.mutex};
    auto index = std::size_t{0};
    while (index < slot_count && state.slots.at(index)) {
      ++index;
    }
    if (index == slot_count) {
      throw std::runtime_error(
        "too many Vulkan render sessions are attached in this process"
      );
    }
    auto& known = state.submit_mutexes[queue];
    auto submit_mutex = known.lock();
    if (!submit_mutex) {
      submit_mutex = std::make_shared<std::mutex>();
      known = submit_mutex;
    }
    state.slots.at(index) = Slot{
      .device = device,
      .queue = queue,
      .functions = functions,
      .submit_mutex = std::move(submit_mutex),
    };
    slot_ = index;
  }

  const auto& entry = entry_points.at(slot_);
  dispatcher.vkQueueSubmit = entry.queue_submit;
  dispatcher.vkQueueWaitIdle = entry.queue_wait_idle;
  dispatcher.vkDeviceWaitIdle = entry.device_wait_idle;
  if (functions.queue_present != nullptr) {
    dispatcher.vkQueuePresentKHR = entry.queue_present;
  }
}

VulkanQueueAccess::~VulkanQueueAccess() { release_queue_access(); }

void VulkanQueueAccess::release_queue_access() noexcept {
  const auto index = std::exchange(slot_, no_slot);
  if (index == no_slot) {
    return;
  }
  auto& state = registry();
  const auto lock = std::scoped_lock{state.mutex};
  // install_queue_access() took the slot, so the index is in bounds, and this
  // function must not throw.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-*)
  auto& slot = state.slots[index];
  if (!slot) {
    return;
  }
  const VkQueue queue = slot->queue;
  slot.reset();
  const auto found = state.submit_mutexes.find(queue);
  if (found != state.submit_mutexes.end() && found->second.expired()) {
    state.submit_mutexes.erase(found);
  }
}

}  // namespace mln::core
