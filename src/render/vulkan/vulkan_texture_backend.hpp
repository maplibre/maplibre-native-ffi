#pragma once

#include <memory>
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
#include "render/vulkan/vulkan_queue_access.hpp"

namespace mln::core {

struct VulkanTextureFrameResources {
  VkImage image = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkDevice device = nullptr;
  VkFormat format = VK_FORMAT_UNDEFINED;
};

// VulkanQueueAccess comes first so that it outlives mbgl's teardown.
class VulkanTextureBackend final : private VulkanQueueAccess,
                                   public mln::vulkan::RendererBackend,
                                   public mln::gfx::HeadlessBackend {
 private:
  class VulkanTextureRenderableResource;

 public:
  // `queue_lock` is the host's lock on the graphics queue, or null.
  VulkanTextureBackend(
    const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth, std::shared_ptr<const QueueLock> queue_lock
  );
  // Renders slot i into the target's textures[i].
  VulkanTextureBackend(
    const VulkanBorrowedTarget& target, mln::Size size,
    std::shared_ptr<const QueueLock> queue_lock
  );
  VulkanTextureBackend(const VulkanTextureBackend&) = delete;
  auto operator=(const VulkanTextureBackend&) -> VulkanTextureBackend& = delete;
  VulkanTextureBackend(VulkanTextureBackend&&) = delete;
  auto operator=(VulkanTextureBackend&&) -> VulkanTextureBackend& = delete;
  ~VulkanTextureBackend() override;

  using VulkanQueueAccess::drain_for_teardown;
  using VulkanQueueAccess::release_queue_access;

  auto getDefaultRenderable() -> mln::gfx::Renderable& override;
  // Follows a new physical size. Each ring slot keeps its image until the slot
  // is selected again, which rebuilds it at the new size.
  void set_ring_size(mln::Size new_size);
  // Whether replacement images can use the render pass already built.
  [[nodiscard]] auto matches_borrowed_target(
    const mln_vulkan_borrowed_texture_descriptor& descriptor
  ) const -> bool;
  // Renders slot i into the target's textures[i] from here on. The caller has
  // already established that the images match the live render passes.
  void set_borrowed_target(const VulkanBorrowedTarget& target);
  // The layout a rendered image is left in, which an acquired frame reports.
  [[nodiscard]] auto frame_layout() const -> VkImageLayout;
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
  // Renders into `slot` from the next prepareRenderResources() on.
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
  // The borrowed format and layouts. Its textures array is null; the images
  // are in borrowed_images_.
  mln_vulkan_borrowed_texture_descriptor borrowed_descriptor_{};
  // The caller's image for each slot, or empty for a session-owned ring.
  std::vector<mln_vulkan_borrowed_texture> borrowed_images_;
  bool uses_borrowed_texture_ = false;
  std::size_t slot_count_;
  std::size_t selected_slot_ = 0;
  // Set by select_slot() and set_borrowed_target(), so the next
  // prepareRenderResources() builds the selected slot at the current size.
  bool selection_pending_ = true;
};

}  // namespace mln::core
