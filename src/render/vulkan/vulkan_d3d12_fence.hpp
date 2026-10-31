#pragma once

#include <vulkan/vulkan_core.h>

namespace mln::core {

// D3D12 fences shared as Vulkan timeline semaphores, through
// VK_KHR_external_semaphore_win32. They exist only on Windows, so elsewhere
// each call fails. This file keeps <windows.h> away from mbgl's headers.

// The NT handle of a timeline semaphore made exportable as a D3D12 fence, or
// null on failure. close_d3d12_fence() closes it.
auto export_d3d12_fence(
  VkDevice device, PFN_vkGetDeviceProcAddr get_proc, VkSemaphore semaphore
) -> void*;
// Imports a D3D12 fence's NT handle into a timeline semaphore, which then
// shares the fence. The handle stays the caller's.
auto import_d3d12_fence(
  VkDevice device, PFN_vkGetDeviceProcAddr get_proc, VkSemaphore semaphore,
  void* handle
) -> bool;
void close_d3d12_fence(void* handle) noexcept;

}  // namespace mln::core
