#pragma once

// The dispatch header sets the vulkan.hpp configuration that the dispatcher
// type depends on, so it comes before anything else that reaches vulkan.hpp.
#include <cstdint>

#include <mln/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_dispatch.hpp"

namespace mln::core {

// Keeps a session's queue calls inside the queue the host gave it.
//
// vkDeviceWaitIdle requires external synchronization of every queue on the
// device, and mbgl calls it when it rebuilds a swapchain and when it tears a
// backend down. A host that submits on its own queue at the same time breaks
// that rule even when the session has a queue to itself. install() points the
// backend's dispatcher at functions that never wait on the whole device:
//
// - vkDeviceWaitIdle drains each queue a session registered on the device, by
//   submitting an empty batch with a fence and waiting on that fence. A fence
//   signals only after everything submitted to its queue before it completes,
//   so this waits for all native work and touches no other queue.
// - vkQueueWaitIdle drains its queue the same way.
// - vkQueueSubmit and vkQueuePresentKHR hold a per-queue lock for the call, so
//   two sessions given the same queue never submit on it at once.
//
// No lock is held across a wait. The functions are plain function pointers
// with no user data, so their state lives in a process-wide registry keyed by
// queue and device.
//
// A backend inherits this class ahead of mln::vulkan::RendererBackend. Base
// classes are destroyed in reverse order, so the registration outlives mbgl's
// teardown, which runs in the RendererBackend destructor and still waits on the
// device through the dispatcher.
class VulkanQueueAccess {
 public:
  VulkanQueueAccess() = default;
  VulkanQueueAccess(const VulkanQueueAccess&) = delete;
  auto operator=(const VulkanQueueAccess&) -> VulkanQueueAccess& = delete;
  VulkanQueueAccess(VulkanQueueAccess&&) = delete;
  auto operator=(VulkanQueueAccess&&) -> VulkanQueueAccess& = delete;
  ~VulkanQueueAccess();

  // Registers `queue` of `device` with the functions `dispatcher` resolved, and
  // routes the dispatcher's device and queue waits, submissions, and presents
  // through the registry. Call it once, after the dispatcher has its device
  // functions. A dispatcher missing a function this needs is left as it is.
  void install_queue_access(
    mln::vulkan::DispatchLoaderDynamic& dispatcher, VkDevice device,
    VkQueue queue
  );

  // Leaves the registry ahead of destruction, for a backend that abandon
  // quarantines: it is never destroyed and never calls Vulkan again, and the
  // host may destroy the device and reuse its handles for another session.
  void release_queue_access() noexcept;

 private:
  VkQueue queue_ = VK_NULL_HANDLE;
  std::uint64_t registration_ = 0;
};

}  // namespace mln::core
