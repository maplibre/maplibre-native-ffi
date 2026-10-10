#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/headless_backend.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/gfx/renderer_backend.hpp>
#include <mln/util/image.hpp>
#include <mln/util/size.hpp>
#include <mln/vulkan/buffer_resource.hpp>
#include <mln/vulkan/context.hpp>
#include <mln/vulkan/renderable_resource.hpp>
#include <mln/vulkan/renderer_backend.hpp>
#include <mln/vulkan/texture2d.hpp>

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_texture_backend.hpp"

#include "maplibre_native_c/texture.h"
#include "render/discard_present.hpp"
#include "render/vulkan/vulkan_dispatch.hpp"
#include "render/vulkan/vulkan_handle.hpp"

namespace {

auto owned_descriptor_from_borrowed(
  const mln_vulkan_borrowed_texture_descriptor& descriptor
) -> mln_vulkan_owned_texture_descriptor {
  return mln_vulkan_owned_texture_descriptor{
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    .extent = descriptor.extent,
    .context = descriptor.context,
  };
}

}  // namespace

namespace mln::core {

// One resource serves every slot of the ring. mbgl keys its pipeline cache on
// the render pass handle, so the slots share one render pass, built once for
// the backend's lifetime, and one depth/stencil attachment, since only one
// slot renders at a time. Each slot has its own color image and framebuffer.
class VulkanTextureBackend::VulkanTextureRenderableResource final
    : public mln::vulkan::SurfaceRenderableResource {
 public:
  explicit VulkanTextureRenderableResource(VulkanTextureBackend& owner)
      : SurfaceRenderableResource(owner),
        owner_(owner),
        slots_(owner.slot_count_) {}

  void createPlatformSurface() override {}
  void bind() override {}

  [[nodiscard]] auto getFramebuffer() const
    -> const vk::UniqueFramebuffer& override {
    return slots_.at(selected_).framebuffer;
  }

  [[nodiscard]] auto has_render_pass() const -> bool {
    return static_cast<bool>(renderPass);
  }

  // Renders into `slot` at `size` from here on, building whatever the slot
  // lacks. swap() waits for each frame, so the attachments a rebuild replaces
  // are idle. A slot whose frame the host holds is never selected, so its
  // color image survives a rebuild of another slot.
  void select(std::size_t slot, vk::Extent2D size) {
    selected_ = slot;
    extent = size;
    if (!renderPass) {
      colorFormat =
        owner_.uses_borrowed_texture_
          ? static_cast<vk::Format>(owner_.borrowed_descriptor_.format)
          : vk::Format::eR8G8B8A8Unorm;
      build_depth(size);
      create_render_pass();
    } else if (depth_extent_ != size) {
      build_depth(size);
    }

    auto& current = slots_.at(slot);
    if (current.extent != size || !current.image) {
      current = Slot{};
      if (owner_.uses_borrowed_texture_) {
        const auto& texture = owner_.borrowed_images_.at(slot);
        current.image =
          vk::Image(mln::core::vulkan_handle_from_abi<VkImage>(texture.image));
        current.view = vk::ImageView(
          mln::core::vulkan_handle_from_abi<VkImageView>(texture.image_view)
        );
      } else {
        allocate_color(current, slot, size);
      }
      current.extent = size;
    }
    if (!current.framebuffer) {
      create_framebuffer(current);
    }
  }

  // Forgets every slot's image, so each one takes its replacement when it is
  // next selected. The caller has waited for the GPU to finish with them.
  void drop_slots() {
    for (auto& slot : slots_) {
      slot = Slot{};
    }
  }

  void swap() override {
    // Submit the recorded frame. A discarded borrowed render then restores the
    // host initial layout so the next render in this update matches the layout
    // contract.
    SurfaceRenderableResource::swap();
    const auto initial_layout =
      static_cast<vk::ImageLayout>(owner_.borrowed_descriptor_.initial_layout);
    const auto final_layout =
      static_cast<vk::ImageLayout>(owner_.borrowed_descriptor_.final_layout);
    if (
      mln::core::discard_renderable_present && owner_.uses_borrowed_texture_ &&
      initial_layout != final_layout &&
      initial_layout != vk::ImageLayout::eUndefined
    ) {
      restore_borrowed_initial_layout(initial_layout, final_layout);
    }
    // This resource is only used by VulkanTextureBackend, so the downcast is
    // invariant within this file.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    static_cast<mln::vulkan::Context&>(backend.getContext()).waitFrame();
  }

  [[nodiscard]] auto image() const -> VkImage {
    return static_cast<VkImage>(slots_.at(selected_).image);
  }

  [[nodiscard]] auto image_view() const -> VkImageView {
    return static_cast<VkImageView>(slots_.at(selected_).view);
  }

  [[nodiscard]] auto format() const -> VkFormat {
    return static_cast<VkFormat>(colorFormat);
  }

 private:
  struct Slot {
    // Declared in reverse of the order they must be destroyed in.
    mln::vulkan::UniqueImageAllocation allocation;
    vk::UniqueImageView owned_view;
    vk::Image image;
    vk::ImageView view;
    vk::UniqueFramebuffer framebuffer;
    // What the image and framebuffer were built at; zero until built.
    vk::Extent2D extent;
  };

  // Every slot's framebuffer names the depth attachment, so replacing it drops
  // each one; a slot builds its framebuffer again when it is next selected.
  void build_depth(vk::Extent2D size) {
    for (auto& slot : slots_) {
      slot.framebuffer.reset();
    }
    initDepthStencil();
    depth_extent_ = size;
  }

  void allocate_color(Slot& slot, std::size_t index, vk::Extent2D size) {
    const auto image_usage =
      vk::ImageUsageFlags{vk::ImageUsageFlagBits::eColorAttachment} |
      vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc;
    const auto image_create_info =
      vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(colorFormat)
        .setExtent({size.width, size.height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(image_usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined);

    auto allocation_create_info = VmaAllocationCreateInfo{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocation_create_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    slot.allocation =
      std::make_unique<mln::vulkan::ImageAllocation>(backend.getAllocator());
    if (!slot.allocation->create(allocation_create_info, image_create_info)) {
      slot.allocation.reset();
      throw std::runtime_error(
        "Vulkan sampled color texture allocation failed"
      );
    }
    slot.image = slot.allocation->image;

    const auto image_view_create_info =
      vk::ImageViewCreateInfo()
        .setImage(slot.image)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(colorFormat)
        .setComponents(vk::ComponentMapping())
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    slot.owned_view = backend.getDevice()->createImageViewUnique(
      image_view_create_info, nullptr, backend.getDispatcher()
    );
    slot.view = slot.owned_view.get();
    backend.setDebugName(
      slot.image, "TextureSessionImage_" + std::to_string(index)
    );
    backend.setDebugName(
      slot.view, "TextureSessionImageView_" + std::to_string(index)
    );
  }

  [[nodiscard]] auto initial_layout() const -> vk::ImageLayout {
    return owner_.uses_borrowed_texture_
             ? static_cast<vk::ImageLayout>(
                 owner_.borrowed_descriptor_.initial_layout
               )
             : vk::ImageLayout::eUndefined;
  }

  [[nodiscard]] auto final_layout() const -> vk::ImageLayout {
    return static_cast<vk::ImageLayout>(owner_.frame_layout());
  }

  // Built once: a replacement target must match its format and layouts, which
  // VulkanTextureBackend::matches_borrowed_target checks.
  void create_render_pass() {
    const auto& device = backend.getDevice();
    const auto& dispatcher = backend.getDispatcher();

    const std::array<vk::AttachmentDescription, 2> attachments = {
      vk::AttachmentDescription()
        .setFormat(colorFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(initial_layout())
        .setFinalLayout(final_layout()),
      vk::AttachmentDescription()
        .setFormat(depthFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
    };
    const auto color_attachment_ref =
      vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    const auto depth_attachment_ref = vk::AttachmentReference(
      1, vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    const auto subpass =
      vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setColorAttachments(color_attachment_ref)
        .setPDepthStencilAttachment(&depth_attachment_ref);
    const std::array<vk::SubpassDependency, 3> dependencies = {
      vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask({})
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite),
      vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(
          vk::PipelineStageFlags{
            vk::PipelineStageFlagBits::eEarlyFragmentTests
          } |
          vk::PipelineStageFlagBits::eLateFragmentTests
        )
        .setDstStageMask(
          vk::PipelineStageFlags{
            vk::PipelineStageFlagBits::eEarlyFragmentTests
          } |
          vk::PipelineStageFlagBits::eLateFragmentTests
        )
        .setSrcAccessMask({})
        .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite),
      vk::SubpassDependency()
        .setSrcSubpass(0)
        .setDstSubpass(VK_SUBPASS_EXTERNAL)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead),
    };
    const auto render_pass_create_info = vk::RenderPassCreateInfo()
                                           .setAttachments(attachments)
                                           .setSubpasses(subpass)
                                           .setDependencies(dependencies);
    renderPass = device->createRenderPassUnique(
      render_pass_create_info, nullptr, dispatcher
    );
  }

  void create_framebuffer(Slot& slot) {
    const std::array<vk::ImageView, 2> image_views = {
      slot.view, depthAllocation->imageView.get()
    };
    const auto framebuffer_create_info = vk::FramebufferCreateInfo()
                                           .setRenderPass(renderPass.get())
                                           .setAttachments(image_views)
                                           .setWidth(slot.extent.width)
                                           .setHeight(slot.extent.height)
                                           .setLayers(1);
    slot.framebuffer = backend.getDevice()->createFramebufferUnique(
      framebuffer_create_info, nullptr, backend.getDispatcher()
    );
  }

  void restore_borrowed_initial_layout(
    vk::ImageLayout initial_layout, vk::ImageLayout final_layout
  ) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& context_impl =
      static_cast<mln::vulkan::Context&>(backend.getContext());
    context_impl.waitFrame();
    const auto rendered = slots_.at(selected_).image;
    context_impl.submitOneTimeCommand(
      [&](const vk::UniqueCommandBuffer& command_buffer) -> void {
        const auto barrier =
          vk::ImageMemoryBarrier()
            .setImage(rendered)
            .setOldLayout(final_layout)
            .setNewLayout(initial_layout)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        command_buffer->pipelineBarrier(
          vk::PipelineStageFlagBits::eColorAttachmentOutput,
          vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, nullptr,
          nullptr, barrier, backend.getDispatcher()
        );
      }
    );
  }

  VulkanTextureBackend& owner_;
  std::vector<Slot> slots_;
  std::size_t selected_ = 0;
  vk::Extent2D depth_extent_;
};

VulkanTextureBackend::VulkanTextureBackend(
  const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size,
  std::size_t ring_depth, std::shared_ptr<const QueueLock> queue_lock
)
    : VulkanQueueAccess(std::move(queue_lock)),
      mln::vulkan::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      descriptor_(descriptor),
      slot_count_(ring_depth) {
  initSharedDevice();
}

VulkanTextureBackend::VulkanTextureBackend(
  const VulkanBorrowedTarget& target, mln::Size size,
  std::shared_ptr<const QueueLock> queue_lock
)
    : VulkanQueueAccess(std::move(queue_lock)),
      mln::vulkan::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      descriptor_(owned_descriptor_from_borrowed(target.descriptor)),
      borrowed_descriptor_(target.descriptor),
      borrowed_images_(target.textures),
      uses_borrowed_texture_(true),
      slot_count_(target.textures.size()) {
  initSharedDevice();
}

VulkanTextureBackend::~VulkanTextureBackend() {
  auto guard = mln::gfx::BackendScope{*this};
  resource.reset();
  getThreadPool().runRenderJobs(true);
}

void VulkanTextureBackend::initSharedDevice() {
  dispatcher = vulkan_dispatch_loader(descriptor_.context);

  initFrameCapture();
  initInstance();
  vulkan_init_instance_dispatch(dispatcher, descriptor_.context);
  initDebug();
  initSurface();
  initDevice();

  physicalDeviceProperties = physicalDevice.getProperties(dispatcher);
}

auto VulkanTextureBackend::getDefaultRenderable() -> mln::gfx::Renderable& {
  if (!resource) {
    resource = std::make_unique<VulkanTextureRenderableResource>(*this);
  }
  return *this;
}

auto VulkanTextureBackend::matches_borrowed_target(
  const mln_vulkan_borrowed_texture_descriptor& descriptor
) const -> bool {
  // Nothing is built yet, so there is no render pass to be incompatible with.
  if (
    !resource ||
    !getResource<VulkanTextureRenderableResource>().has_render_pass()
  ) {
    return true;
  }
  // The ring's pass was built around these, and mbgl keys its pipelines on the
  // pass, so a replacement keeps them to keep those pipelines usable.
  return descriptor.format == borrowed_descriptor_.format &&
         descriptor.initial_layout == borrowed_descriptor_.initial_layout &&
         descriptor.final_layout == borrowed_descriptor_.final_layout;
}

void VulkanTextureBackend::set_borrowed_target(
  const VulkanBorrowedTarget& target
) {
  const auto new_size = mln::Size{
    target.descriptor.physical_width, target.descriptor.physical_height
  };
  // Once this wait returns, the session is done with the outgoing images, and
  // the host may destroy them when set_target completes. With one frame in
  // flight, the frame fence covers the session's last frame, and every other
  // submission waits on its own fence before returning. swap() already waits
  // on the frame fence, so this wait only keeps the guarantee local. A device
  // wait would also need the host's queues, which the session must not touch.
  if (context) {
    // VulkanTextureBackend always constructs a Vulkan renderer context.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    if (!static_cast<mln::vulkan::Context&>(*context).waitFrame()) {
      throw std::runtime_error("Vulkan frame fence wait failed");
    }
  }
  borrowed_descriptor_ = target.descriptor;
  borrowed_images_ = target.textures;
  setRenderableSize(new_size);
  // Each slot builds a framebuffer over its new image when it is next
  // selected. The render pass stays: mbgl keys its pipeline cache on the pass,
  // so a new pass would compile every pipeline again on each replacement,
  // which a host makes on every resize.
  if (resource) {
    getResource<VulkanTextureRenderableResource>().drop_slots();
  }
  selection_pending_ = true;
}

auto VulkanTextureBackend::frame_layout() const -> VkImageLayout {
  return uses_borrowed_texture_
           ? static_cast<VkImageLayout>(borrowed_descriptor_.final_layout)
           : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void VulkanTextureBackend::set_ring_size(mln::Size new_size) {
  // A slot keeps its image until it is next selected, which rebuilds it at the
  // new extent, so a frame the host holds stays intact.
  setRenderableSize(new_size);
}

auto VulkanTextureBackend::readStillImage() -> mln::PremultipliedImage {
  prepareRenderResources();

  const auto size = getSize();
  auto image = mln::PremultipliedImage(size);
  const auto image_size = image.bytes();
  const auto& allocator = getAllocator();
  const auto buffer_info = vk::BufferCreateInfo()
                             .setSize(image_size)
                             .setUsage(vk::BufferUsageFlagBits::eTransferDst)
                             .setSharingMode(vk::SharingMode::eExclusive);

  auto allocation_info = VmaAllocationCreateInfo{};
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
  allocation_info.requiredFlags =
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  allocation_info.flags =
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
    VMA_ALLOCATION_CREATE_MAPPED_BIT;

  auto buffer_allocation = mln::vulkan::BufferAllocation{allocator};
  if (!buffer_allocation.create(allocation_info, buffer_info)) {
    throw std::runtime_error("Vulkan readback buffer allocation failed");
  }

  // VulkanTextureBackend always constructs a Vulkan renderer context.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& context_impl = static_cast<mln::vulkan::Context&>(getContext());
  auto& resource_impl = getResource<VulkanTextureRenderableResource>();
  const auto source_image = vk::Image(resource_impl.image());
  context_impl.waitFrame();
  context_impl.submitOneTimeCommand(
    [&](const vk::UniqueCommandBuffer& command_buffer) -> void {
      const auto to_transfer =
        vk::ImageMemoryBarrier()
          .setImage(source_image)
          .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
          .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
          .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
          .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
          .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
      command_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, to_transfer,
        getDispatcher()
      );

      const auto region =
        vk::BufferImageCopy()
          .setBufferOffset(0)
          .setBufferRowLength(0)
          .setBufferImageHeight(0)
          .setImageSubresource(
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1)
          )
          .setImageOffset({0, 0, 0})
          .setImageExtent({size.width, size.height, 1});
      command_buffer->copyImageToBuffer(
        source_image, vk::ImageLayout::eTransferSrcOptimal,
        buffer_allocation.buffer, region, getDispatcher()
      );

      const auto to_shader_read =
        vk::ImageMemoryBarrier()
          .setImage(source_image)
          .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
          .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
          .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
          .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
          .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
      command_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr,
        to_shader_read, getDispatcher()
      );
    }
  );

  if (buffer_allocation.mappedBuffer == nullptr) {
    if (
      vmaMapMemory(
        allocator, buffer_allocation.allocation, &buffer_allocation.mappedBuffer
      ) != VK_SUCCESS
    ) {
      throw std::runtime_error("Vulkan readback host memory map failed");
    }
    std::memcpy(image.data.get(), buffer_allocation.mappedBuffer, image_size);
    vmaUnmapMemory(allocator, buffer_allocation.allocation);
    buffer_allocation.mappedBuffer = nullptr;
  } else {
    std::memcpy(image.data.get(), buffer_allocation.mappedBuffer, image_size);
  }
  return image;
}

auto VulkanTextureBackend::getRendererBackend() -> mln::gfx::RendererBackend* {
  return this;
}

void VulkanTextureBackend::activate() {}

void VulkanTextureBackend::deactivate() {}

void VulkanTextureBackend::prepareRenderResources() {
  if (allocator == nullptr) {
    initAllocator();
  }
  if (!resource) {
    initSwapchain();
  }
  if (selection_pending_) {
    const auto size = getSize();
    getResource<VulkanTextureRenderableResource>().select(
      selected_slot_, vk::Extent2D{size.width, size.height}
    );
    selection_pending_ = false;
  }
  if (!commandPool) {
    initCommandPool();
  }
}

auto VulkanTextureBackend::frame_resources() -> VulkanTextureFrameResources {
  prepareRenderResources();
  auto& rendered = getResource<VulkanTextureRenderableResource>();
  return VulkanTextureFrameResources{
    .image = rendered.image(),
    .image_view = rendered.image_view(),
    .device = device.get(),
    .format = rendered.format(),
  };
}

auto VulkanTextureBackend::select_slot(std::size_t slot) -> bool {
  if (slot >= slot_count_) {
    return false;
  }
  selected_slot_ = slot;
  selection_pending_ = true;
  return true;
}

void VulkanTextureBackend::initInstance() {
  usingSharedContext = true;
  instance = vk::UniqueInstance(
    static_cast<VkInstance>(descriptor_.context.instance),
    mln::vulkan::ObjectDestroy<vk::detail::NoParent>(nullptr, dispatcher)
  );
}

void VulkanTextureBackend::initDebug() {}

void VulkanTextureBackend::initSurface() {}

void VulkanTextureBackend::initDevice() {
  const auto physical_devices = instance->enumeratePhysicalDevices(dispatcher);
  auto* const requested_physical_device =
    static_cast<VkPhysicalDevice>(descriptor_.context.physical_device);
  auto found_physical_device = false;
  for (const auto& candidate : physical_devices) {
    if (static_cast<VkPhysicalDevice>(candidate) == requested_physical_device) {
      physicalDevice = candidate;
      found_physical_device = true;
      break;
    }
  }
  if (!found_physical_device) {
    throw std::runtime_error(
      "Vulkan physical_device does not belong to instance"
    );
  }
  device = vk::UniqueDevice(
    static_cast<VkDevice>(descriptor_.context.device),
    mln::vulkan::ObjectDestroy<vk::detail::NoParent>(nullptr, dispatcher)
  );
  vulkan_init_device_dispatch(dispatcher, device.get(), descriptor_.context);
  graphicsQueueIndex =
    static_cast<int32_t>(descriptor_.context.graphics_queue_family_index);
  presentQueueIndex = -1;
  graphicsQueue = static_cast<VkQueue>(descriptor_.context.graphics_queue);
  install_queue_access(
    dispatcher, device.get(), static_cast<VkQueue>(graphicsQueue)
  );
  physicalDeviceFeatures = physicalDevice.getFeatures(dispatcher);
}

void VulkanTextureBackend::initSwapchain() {
  // One frame in flight: swap() waits for each frame before the ring hands it
  // out.
  maxFrames = 1;
  getDefaultRenderable();
}

// Base override requires a member function.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto VulkanTextureBackend::getDeviceExtensions() -> std::vector<const char*> {
  return {};
}

}  // namespace mln::core
