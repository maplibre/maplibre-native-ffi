#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_d3d12_fence.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
// vulkan_win32.h names types that windows.h declares.
#if defined(_WIN32)
#include <vulkan/vulkan_win32.h>
#endif

namespace mln::core {

#if defined(_WIN32)
auto export_d3d12_fence(
  VkDevice device, PFN_vkGetDeviceProcAddr get_proc, VkSemaphore semaphore
) -> void* {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto get_handle = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(
    get_proc(device, "vkGetSemaphoreWin32HandleKHR")
  );
  if (get_handle == nullptr) return nullptr;
  const auto info = VkSemaphoreGetWin32HandleInfoKHR{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
    .pNext = nullptr,
    .semaphore = semaphore,
    .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT,
  };
  auto* handle = HANDLE{nullptr};
  if (get_handle(device, &info, &handle) != VK_SUCCESS) return nullptr;
  return handle;
}

auto import_d3d12_fence(
  VkDevice device, PFN_vkGetDeviceProcAddr get_proc, VkSemaphore semaphore,
  void* handle
) -> bool {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto import = reinterpret_cast<PFN_vkImportSemaphoreWin32HandleKHR>(
    get_proc(device, "vkImportSemaphoreWin32HandleKHR")
  );
  if (import == nullptr) return false;
  const auto info = VkImportSemaphoreWin32HandleInfoKHR{
    .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR,
    .pNext = nullptr,
    .semaphore = semaphore,
    .flags = 0,
    .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT,
    .handle = handle,
    .name = nullptr,
  };
  return import(device, &info) == VK_SUCCESS;
}

void close_d3d12_fence(void* handle) noexcept {
  if (handle != nullptr) (void)CloseHandle(handle);
}
#else
auto export_d3d12_fence(
  VkDevice /*device*/, PFN_vkGetDeviceProcAddr /*get_proc*/,
  VkSemaphore /*semaphore*/
) -> void* {
  return nullptr;
}

auto import_d3d12_fence(
  VkDevice /*device*/, PFN_vkGetDeviceProcAddr /*get_proc*/,
  VkSemaphore /*semaphore*/, void* /*handle*/
) -> bool {
  return false;
}

void close_d3d12_fence(void* /*handle*/) noexcept {}
#endif

}  // namespace mln::core
