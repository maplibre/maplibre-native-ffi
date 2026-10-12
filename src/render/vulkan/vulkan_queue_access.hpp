#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include <mln/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan_core.h>

#include "render/queue_lock.hpp"

namespace mln::core {

// Keeps a session's queue calls inside the queue the host gave it.
//
// vkDeviceWaitIdle requires external synchronization of every queue on the
// device, and mbgl calls it when it rebuilds a swapchain and when it tears a
// backend down. A host that submits on its own queue at the same time breaks
// that rule even when the session has a queue to itself.
// install_queue_access() points the backend's dispatcher at functions that
// never wait on the whole device:
//
// - vkDeviceWaitIdle drains the session's own queue, by submitting an empty
//   batch with a fence and waiting on that fence. A fence signals only after
//   everything submitted to its queue before it completes, so this waits for
//   all of the session's work and touches no other queue, including the queue
//   of another session on the same device.
// - vkQueueWaitIdle drains the queue the same way.
// - vkQueueSubmit and vkQueuePresentKHR hold a per-queue lock for the call, so
//   two sessions given the same queue never submit on it at once.
//
// Each submission, the drain's included, and each present also holds the
// host's queue lock when the session has one, taken inside the per-queue lock,
// so the host can share the queue with the session. Once the process has begun
// to exit, such a call returns VK_ERROR_DEVICE_LOST without calling the host's
// lock or reaching the queue. No lock is held across a wait.
//
// Each session calls through the device functions its own dispatcher resolved,
// so a host that interposes on those functions sees only its own session's
// calls.
//
// Vulkan entry points carry no user data, so each registration takes one of a
// fixed set of slots, each with its own entry points. A registration past the
// last slot throws, which fails the attach.
//
// A backend inherits this class ahead of mln::vulkan::RendererBackend. Base
// classes are destroyed in reverse order, so the registration outlives mbgl's
// teardown, which runs in the RendererBackend destructor and still waits on the
// device through the dispatcher.
class VulkanQueueAccess {
 public:
  // `host_lock` is the host's lock on the queue, or null when it has none.
  explicit VulkanQueueAccess(std::shared_ptr<const QueueLock> host_lock)
      : host_lock_(std::move(host_lock)) {}
  VulkanQueueAccess(const VulkanQueueAccess&) = delete;
  auto operator=(const VulkanQueueAccess&) -> VulkanQueueAccess& = delete;
  VulkanQueueAccess(VulkanQueueAccess&&) = delete;
  auto operator=(VulkanQueueAccess&&) -> VulkanQueueAccess& = delete;
  ~VulkanQueueAccess();

  // Registers `queue` of `device` with the functions `dispatcher` resolved, and
  // routes the dispatcher's device and queue waits, submissions, and presents
  // through the registration. Call it once, after the dispatcher has its
  // device functions. A dispatcher missing a function this needs is left as
  // it is, unless the host passed a queue lock, which then throws. Also throws
  // when every slot is taken.
  void install_queue_access(
    mln::vulkan::DispatchLoaderDynamic& dispatcher, VkDevice device,
    VkQueue queue
  );

  // Prepares a backend for destruction on a thread other than its driver's,
  // as abandon destroys it. First, from here on, a device or queue wait that
  // fails reports VK_ERROR_DEVICE_LOST. mbgl's destructors catch only that
  // failure, so another one, such as a fence that cannot be created, would
  // otherwise terminate the process. Then it waits for the work the session
  // submitted to its queue, taking the host's queue lock only to submit the
  // wait. Returns whether the backend may now be destroyed: the queue drained
  // or the device is lost. Any other failure leaves work that may still use
  // the backend's objects.
  [[nodiscard]] auto drain_for_teardown() noexcept -> bool;

  // Leaves the registry ahead of destruction, for a backend that detach or
  // abandon keeps: it is never destroyed and never calls Vulkan again, and the
  // host may reuse its handles for another session. This also lets go of the
  // host's queue lock.
  void release_queue_access() noexcept;

 private:
  static constexpr auto no_slot = static_cast<std::size_t>(-1);
  std::size_t slot_ = no_slot;
  // Moved into the registration, which holds it while a call can use it.
  std::shared_ptr<const QueueLock> host_lock_;
};

}  // namespace mln::core
