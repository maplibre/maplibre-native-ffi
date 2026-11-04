#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <vulkan/vulkan_core.h>

#include "maplibre_native_c/base.h"

namespace mln::core {

// D3D11 textures shared through NT handles, whose memory Vulkan imports with
// VK_KHR_external_memory_win32. Drivers import D3D11 textures into Vulkan, but
// don't export Vulkan memory as one that D3D11 opens, so D3D11 makes them.
// This file keeps <windows.h> and the D3D headers away from mbgl's headers.

// A D3D11 device on the DXGI adapter of a Vulkan GPU, which makes a ring's
// textures.
class DxgiTextureDevice;

// Makes a D3D11 device on the adapter whose LUID is `luid`, and checks that it
// shares textures through NT handles. Sets the thread's error on failure.
auto create_dxgi_texture_device(
  const std::array<uint8_t, 8>& luid,
  std::shared_ptr<const DxgiTextureDevice>& out_device
) -> mln_status;

// A DXGI_FORMAT_B8G8R8A8_UNORM texture with a shared NT handle, and the Vulkan
// memory that bind() imports from it. Destroy the image that it binds first.
class DxgiSharedTexture {
 public:
  // Throws on failure.
  DxgiSharedTexture(
    std::shared_ptr<const DxgiTextureDevice> device, uint32_t width,
    uint32_t height
  );
  DxgiSharedTexture(const DxgiSharedTexture&) = delete;
  auto operator=(const DxgiSharedTexture&) -> DxgiSharedTexture& = delete;
  DxgiSharedTexture(DxgiSharedTexture&&) = delete;
  auto operator=(DxgiSharedTexture&&) -> DxgiSharedTexture& = delete;
  ~DxgiSharedTexture();

  // Binds `image`, a VK_FORMAT_B8G8R8A8_UNORM image of the texture's size
  // created for VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT, to memory
  // imported from the texture. Throws on failure.
  void bind(
    VkDevice device, PFN_vkGetDeviceProcAddr get_proc,
    const VkPhysicalDeviceMemoryProperties& memory, VkImage image
  );
  [[nodiscard]] auto handle() const -> void* { return handle_; }

 private:
  std::shared_ptr<const DxgiTextureDevice> device_;
  // The ID3D11Texture2D.
  void* texture_ = nullptr;
  void* handle_ = nullptr;
  VkDevice memory_device_ = VK_NULL_HANDLE;
  PFN_vkFreeMemory free_memory_ = nullptr;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
};

}  // namespace mln::core
