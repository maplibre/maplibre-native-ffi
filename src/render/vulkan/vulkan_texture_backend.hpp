#pragma once

#include <vector>

#include <mln/gfx/headless_backend.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/gfx/renderer_backend.hpp>
#include <mln/util/image.hpp>
#include <mln/util/size.hpp>
#include <mln/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan_core.h>

#include "maplibre_native_c/texture.h"
#include "render/render_session_common.hpp"

namespace mln::core {

struct VulkanTextureFrameResources {
  VkImage image = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkDevice device = nullptr;
  VkFormat format = VK_FORMAT_UNDEFINED;
};

class VulkanTextureBackend final : public mln::vulkan::RendererBackend,
                                   public mln::gfx::HeadlessBackend {
 private:
  class VulkanTextureRenderableResource;

 public:
  VulkanTextureBackend(
    const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth
  );
  VulkanTextureBackend(
    const mln_vulkan_borrowed_texture_descriptor& descriptor, mln::Size size
  );
  VulkanTextureBackend(const VulkanTextureBackend&) = delete;
  auto operator=(const VulkanTextureBackend&) -> VulkanTextureBackend& = delete;
  VulkanTextureBackend(VulkanTextureBackend&&) = delete;
  auto operator=(VulkanTextureBackend&&) -> VulkanTextureBackend& = delete;
  ~VulkanTextureBackend() override;

  auto getDefaultRenderable() -> mln::gfx::Renderable& override;
  // Follows a new physical size. Each ring slot keeps its resource until the
  // slot is selected again, which rebuilds it at the new size.
  void set_ring_size(mln::Size new_size);
  // Whether a replacement image can use the render pass already in hand.
  [[nodiscard]] auto matches_borrowed_target(
    const mln_vulkan_borrowed_texture_descriptor& descriptor
  ) const -> bool;
  // Renders into a different caller-owned image from here on. The caller has
  // already established that it matches the live render pass.
  void set_borrowed_target(
    const mln_vulkan_borrowed_texture_descriptor& descriptor
  );
  [[nodiscard]] auto context_descriptor() const
    -> const mln_vulkan_context_descriptor& {
    return descriptor_.context;
  }
  auto readStillImage() -> mln::PremultipliedImage override;
  auto getRendererBackend() -> mln::gfx::RendererBackend* override;
  void activate() override;
  void deactivate() override;

  void prepareRenderResources();
  // The image of the slot this backend is rendering into.
  auto frame_resources() -> VulkanTextureFrameResources;
  auto select_slot(std::size_t slot) -> bool;

 protected:
  void initInstance() override;
  void initDebug() override;
  void initSurface() override;
  void initDevice() override;
  void initSwapchain() override;
  auto getDeviceExtensions() -> std::vector<const char*> override;

 private:
  void initSharedDevice();
  mln_vulkan_owned_texture_descriptor descriptor_;
  mln_vulkan_borrowed_texture_descriptor borrowed_descriptor_{};
  bool uses_borrowed_texture_ = false;
  RenderableSlotRing ring_;
};

}  // namespace mln::core
