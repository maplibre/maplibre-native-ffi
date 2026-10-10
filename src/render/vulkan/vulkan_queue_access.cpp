// The dispatch header sets the vulkan.hpp configuration this translation unit
// has to agree on, so it stays the first include.
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mln/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_queue_access.hpp"

#include "render/vulkan/vulkan_dispatch.hpp"

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

struct Registration {
  std::uint64_t id = 0;
  QueueFunctions functions;
};

struct QueueEntry {
  VkDevice device = VK_NULL_HANDLE;
  // Held for each native submission or present on the queue, and never across
  // a wait.
  std::mutex submit_mutex;
  // Every session registered on this queue, guarded by the registry mutex.
  // Their functions all reach the same queue, so the first one serves.
  std::vector<Registration> registrations;
};

struct QueueRef {
  VkQueue queue = VK_NULL_HANDLE;
  std::shared_ptr<QueueEntry> entry;
  QueueFunctions functions;
};

struct Registry {
  std::mutex mutex;
  std::unordered_map<VkQueue, std::shared_ptr<QueueEntry>> queues;
  std::uint64_t next_id = 1;
};

auto registry() -> Registry& {
  // Leaked, so a driver thread still tearing a session down while the process
  // exits never finds the registry destroyed.
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  static auto* instance = new Registry();
  return *instance;
}

auto find_queue(VkQueue queue) -> std::optional<QueueRef> {
  auto& state = registry();
  const auto lock = std::scoped_lock{state.mutex};
  const auto found = state.queues.find(queue);
  if (found == state.queues.end() || found->second->registrations.empty()) {
    return std::nullopt;
  }
  return QueueRef{
    .queue = queue,
    .entry = found->second,
    .functions = found->second->registrations.front().functions,
  };
}

auto find_device_queues(VkDevice device) -> std::vector<QueueRef> {
  auto& state = registry();
  const auto lock = std::scoped_lock{state.mutex};
  auto queues = std::vector<QueueRef>{};
  for (const auto& [queue, entry] : state.queues) {
    if (entry->device == device && !entry->registrations.empty()) {
      queues.push_back(
        QueueRef{
          .queue = queue,
          .entry = entry,
          .functions = entry->registrations.front().functions,
        }
      );
    }
  }
  return queues;
}

// Waits for everything submitted to the queue so far, without holding its
// lock during the wait.
auto drain_queue(const QueueRef& ref) -> VkResult {
  const auto& functions = ref.functions;
  const auto device = ref.entry->device;
  const auto create_info = VkFenceCreateInfo{
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
  };
  auto fence = VkFence{VK_NULL_HANDLE};
  auto result = functions.create_fence(device, &create_info, nullptr, &fence);
  if (result != VK_SUCCESS) {
    return result;
  }
  {
    const auto lock = std::scoped_lock{ref.entry->submit_mutex};
    // An empty batch still signals its fence, once all earlier work on the
    // queue has completed.
    result = functions.queue_submit(ref.queue, 0, nullptr, fence);
  }
  if (result == VK_SUCCESS) {
    result = functions.wait_for_fences(
      device, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()
    );
  }
  functions.destroy_fence(device, fence, nullptr);
  return result;
}

// Only a dispatcher that install_queue_access() changed reaches these, and its
// registration lasts while its backend can still call them, so a missing entry
// means a broken invariant rather than a queue to fall back to.
constexpr auto unregistered_result = VK_ERROR_INITIALIZATION_FAILED;

VKAPI_ATTR auto VKAPI_CALL queue_submit(
  VkQueue queue, uint32_t submit_count, const VkSubmitInfo* submits,
  VkFence fence
) -> VkResult {
  const auto ref = find_queue(queue);
  if (!ref) {
    return unregistered_result;
  }
  const auto lock = std::scoped_lock{ref->entry->submit_mutex};
  return ref->functions.queue_submit(queue, submit_count, submits, fence);
}

VKAPI_ATTR auto VKAPI_CALL
queue_present(VkQueue queue, const VkPresentInfoKHR* present_info) -> VkResult {
  const auto ref = find_queue(queue);
  if (!ref || ref->functions.queue_present == nullptr) {
    return unregistered_result;
  }
  const auto lock = std::scoped_lock{ref->entry->submit_mutex};
  return ref->functions.queue_present(queue, present_info);
}

VKAPI_ATTR auto VKAPI_CALL queue_wait_idle(VkQueue queue) -> VkResult {
  const auto ref = find_queue(queue);
  if (!ref) {
    return unregistered_result;
  }
  return drain_queue(*ref);
}

VKAPI_ATTR auto VKAPI_CALL device_wait_idle(VkDevice device) -> VkResult {
  const auto queues = find_device_queues(device);
  if (queues.empty()) {
    return unregistered_result;
  }
  auto result = VK_SUCCESS;
  for (const auto& ref : queues) {
    const auto drained = drain_queue(ref);
    if (result == VK_SUCCESS) {
      result = drained;
    }
  }
  return result;
}

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
    registration_ != 0 || device == VK_NULL_HANDLE || queue == VK_NULL_HANDLE ||
    functions.queue_submit == nullptr || functions.create_fence == nullptr ||
    functions.wait_for_fences == nullptr || functions.destroy_fence == nullptr
  ) {
    return;
  }

  {
    auto& state = registry();
    const auto lock = std::scoped_lock{state.mutex};
    auto& entry = state.queues[queue];
    if (!entry) {
      entry = std::make_shared<QueueEntry>();
      entry->device = device;
    }
    registration_ = state.next_id++;
    entry->registrations.push_back(
      Registration{.id = registration_, .functions = functions}
    );
  }
  queue_ = queue;

  dispatcher.vkQueueSubmit = &queue_submit;
  dispatcher.vkQueueWaitIdle = &queue_wait_idle;
  dispatcher.vkDeviceWaitIdle = &device_wait_idle;
  if (functions.queue_present != nullptr) {
    dispatcher.vkQueuePresentKHR = &queue_present;
  }
}

VulkanQueueAccess::~VulkanQueueAccess() { release_queue_access(); }

void VulkanQueueAccess::release_queue_access() noexcept {
  const auto id = std::exchange(registration_, 0);
  if (id == 0) {
    return;
  }
  auto& state = registry();
  const auto lock = std::scoped_lock{state.mutex};
  const auto found = state.queues.find(queue_);
  if (found == state.queues.end()) {
    return;
  }
  auto& registrations = found->second->registrations;
  std::erase_if(registrations, [id](const Registration& registration) {
    return registration.id == id;
  });
  if (registrations.empty()) {
    state.queues.erase(found);
  }
}

}  // namespace mln::core
