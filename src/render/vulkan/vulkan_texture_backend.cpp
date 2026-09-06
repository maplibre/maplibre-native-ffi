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

class VulkanTextureBackend::VulkanTextureRenderableResource final
    : public mln::vulkan::SurfaceRenderableResource {
 public:
  explicit VulkanTextureRenderableResource(VulkanTextureBackend& backend_)
      : SurfaceRenderableResource(backend_) {}

  void createPlatformSurface() override {}
  void bind() override {}

  void init_sampled(vk::Extent2D sampled_extent) {
    init_sampled_color(sampled_extent);
    create_color_image_views();
    initDepthStencil();
    create_render_pass(
      vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
    );
    create_framebuffers();
  }

  // Rebuilds everything sized with the texture and keeps the render pass. mbgl
  // keys its pipeline cache on the render pass handle, so destroying the pass
  // strands every cached pipeline and lets a recycled handle hit a pipeline
  // built for a different pass. Attachment formats and layouts do not vary with
  // size.
  void resize_sampled(vk::Extent2D sampled_extent) {
    backend.getDevice()->waitIdle(backend.getDispatcher());
    swapchainFramebuffers.clear();
    swapchainImageViews.clear();
    swapchainImages.clear();
    colorAllocations.clear();
    readTexture.reset();

    init_sampled_color(sampled_extent);
    create_color_image_views();
    initDepthStencil();
    create_framebuffers();
  }

  // Whether a replacement image can use the render pass this resource already
  // has, which needs the format and both layouts the pass was built around.
  [[nodiscard]] auto matches_borrowed(
    const mln_vulkan_borrowed_texture_descriptor& descriptor
  ) const -> bool {
    return colorFormat == static_cast<vk::Format>(descriptor.format) &&
           initial_layout_ ==
             static_cast<vk::ImageLayout>(descriptor.initial_layout) &&
           final_layout_ ==
             static_cast<vk::ImageLayout>(descriptor.final_layout);
  }

  // Renders into a different caller-owned image from here on. The caller has
  // already established that it matches the live render pass, which is kept.
  void set_borrowed(
    const mln_vulkan_borrowed_texture_descriptor& descriptor, uint32_t width,
    uint32_t height
  ) {
    backend.getDevice()->waitIdle(backend.getDispatcher());
    swapchainFramebuffers.clear();
    swapchainImages.clear();
    init_borrowed(descriptor, width, height);
  }

  void init_borrowed(
    const mln_vulkan_borrowed_texture_descriptor& descriptor, uint32_t width,
    uint32_t height
  ) {
    usesBorrowedImage = true;
    borrowedImage =
      vk::Image(mln::core::vulkan_handle_from_abi<VkImage>(descriptor.image));
    borrowedImageView = vk::ImageView(
      mln::core::vulkan_handle_from_abi<VkImageView>(descriptor.image_view)
    );
    colorFormat = static_cast<vk::Format>(descriptor.format);
    extent = vk::Extent2D(width, height);
    swapchainImages.push_back(borrowedImage);

    initDepthStencil();
    create_render_pass(
      static_cast<vk::ImageLayout>(descriptor.initial_layout),
      static_cast<vk::ImageLayout>(descriptor.final_layout)
    );
    create_framebuffers();
  }

  void swap() override {
    // Submit the recorded frame. A discarded borrowed render then restores the
    // host initial layout so the next render in this update matches the layout
    // contract.
    SurfaceRenderableResource::swap();
    if (
      mln::core::discard_renderable_present && usesBorrowedImage &&
      initial_layout_ != final_layout_ &&
      initial_layout_ != vk::ImageLayout::eUndefined
    ) {
      restore_borrowed_initial_layout();
    }
    // This resource is only used by VulkanTextureBackend, so the downcast is
    // invariant within this file.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    static_cast<mln::vulkan::Context&>(backend.getContext()).waitFrame();
  }

  [[nodiscard]] auto image() const -> VkImage {
    if (usesBorrowedImage) {
      return static_cast<VkImage>(borrowedImage);
    }
    return static_cast<VkImage>(getAcquiredImage());
  }

  [[nodiscard]] auto image_view() const -> VkImageView {
    if (usesBorrowedImage) {
      return static_cast<VkImageView>(borrowedImageView);
    }
    return static_cast<VkImageView>(
      swapchainImageViews.at(getAcquiredImageIndex()).get()
    );
  }

  [[nodiscard]] auto format() const -> VkFormat {
    return static_cast<VkFormat>(colorFormat);
  }

 private:
  void init_sampled_color(vk::Extent2D sampled_extent) {
    const auto image_count = backend.getMaxFrames();
    colorAllocations.reserve(image_count);
    swapchainImages.reserve(image_count);

    colorFormat = vk::Format::eR8G8B8A8Unorm;
    extent = sampled_extent;

    const auto image_usage = vk::ImageUsageFlagBits::eColorAttachment |
                             vk::ImageUsageFlagBits::eSampled |
                             vk::ImageUsageFlagBits::eTransferSrc;
    auto image_create_info =
      vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(colorFormat)
        .setExtent({sampled_extent.width, sampled_extent.height, 1})
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

    for (auto index = uint32_t{}; index < image_count; ++index) {
      auto allocation =
        std::make_unique<mln::vulkan::ImageAllocation>(backend.getAllocator());
      if (!allocation->create(allocation_create_info, image_create_info)) {
        throw std::runtime_error(
          "Vulkan sampled color texture allocation failed"
        );
      }
      swapchainImages.push_back(allocation->image);
      colorAllocations.push_back(std::move(allocation));
    }
  }

  void create_color_image_views() {
    const auto& device = backend.getDevice();
    const auto& dispatcher = backend.getDispatcher();

    swapchainImageViews.reserve(swapchainImages.size());
    auto image_view_create_info =
      vk::ImageViewCreateInfo()
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(colorFormat)
        .setComponents(vk::ComponentMapping())
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    for (const auto& image : swapchainImages) {
      image_view_create_info.setImage(image);
      swapchainImageViews.push_back(device->createImageViewUnique(
        image_view_create_info, nullptr, dispatcher
      ));
      const auto index = swapchainImageViews.size() - 1;
      backend.setDebugName(
        image, "TextureSessionImage_" + std::to_string(index)
      );
      backend.setDebugName(
        swapchainImageViews.back().get(),
        "TextureSessionImageView_" + std::to_string(index)
      );
    }
  }

  // Idempotent: a live pass is left alone so the pipelines mbgl cached against
  // it stay reachable. A caller changing an attachment format or layout must
  // drop the pass first, which also retires those pipelines.
  void create_render_pass(
    vk::ImageLayout initial_layout, vk::ImageLayout final_layout
  ) {
    initial_layout_ = initial_layout;
    final_layout_ = final_layout;
    if (renderPass) {
      return;
    }

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
        .setInitialLayout(initial_layout)
        .setFinalLayout(final_layout),
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
          vk::PipelineStageFlagBits::eEarlyFragmentTests |
          vk::PipelineStageFlagBits::eLateFragmentTests
        )
        .setDstStageMask(
          vk::PipelineStageFlagBits::eEarlyFragmentTests |
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

  void create_framebuffers() {
    const auto& device = backend.getDevice();
    const auto& dispatcher = backend.getDispatcher();

    swapchainFramebuffers.reserve(swapchainImageViews.size());
    auto framebuffer_create_info = vk::FramebufferCreateInfo()
                                     .setRenderPass(renderPass.get())
                                     .setAttachmentCount(2)
                                     .setWidth(extent.width)
                                     .setHeight(extent.height)
                                     .setLayers(1);
    const auto image_count =
      usesBorrowedImage ? size_t{1} : swapchainImageViews.size();
    for (auto index = size_t{}; index < image_count; ++index) {
      const std::array<vk::ImageView, 2> image_views = {
        color_image_view(index), depthAllocation->imageView.get()
      };
      framebuffer_create_info.setAttachments(image_views);
      swapchainFramebuffers.push_back(device->createFramebufferUnique(
        framebuffer_create_info, nullptr, dispatcher
      ));
    }
  }

  [[nodiscard]] auto color_image_view(size_t index) const -> vk::ImageView {
    if (usesBorrowedImage) {
      return borrowedImageView;
    }
    return swapchainImageViews.at(index).get();
  }

  void restore_borrowed_initial_layout() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& context_impl =
      static_cast<mln::vulkan::Context&>(backend.getContext());
    context_impl.waitFrame();
    context_impl.submitOneTimeCommand(
      [&](const vk::UniqueCommandBuffer& command_buffer) -> void {
        const auto barrier =
          vk::ImageMemoryBarrier()
            .setImage(borrowedImage)
            .setOldLayout(final_layout_)
            .setNewLayout(initial_layout_)
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

  bool usesBorrowedImage = false;
  vk::Image borrowedImage;
  vk::ImageView borrowedImageView;
  // What the live render pass was built around, for matching replacements.
  vk::ImageLayout initial_layout_ = vk::ImageLayout::eUndefined;
  vk::ImageLayout final_layout_ = vk::ImageLayout::eUndefined;
};

VulkanTextureBackend::VulkanTextureBackend(
  const mln_vulkan_owned_texture_descriptor& descriptor, mln::Size size,
  std::size_t ring_depth
)
    : mln::vulkan::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      descriptor_(descriptor),
      slot_resources_(ring_depth),
      slot_sizes_(ring_depth) {
  initSharedDevice();
}

VulkanTextureBackend::VulkanTextureBackend(
  const mln_vulkan_borrowed_texture_descriptor& descriptor, mln::Size size
)
    : mln::vulkan::RendererBackend(mln::gfx::ContextMode::Unique),
      mln::gfx::HeadlessBackend(size),
      descriptor_(owned_descriptor_from_borrowed(descriptor)),
      borrowed_descriptor_(descriptor),
      uses_borrowed_texture_(true),
      slot_resources_(1),
      slot_sizes_(1) {
  initSharedDevice();
}

VulkanTextureBackend::~VulkanTextureBackend() {
  auto guard = mln::gfx::BackendScope{*this};
  resource.reset();
  slot_resources_.clear();
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
    slot_sizes_[selected_slot_] = size;
  }
  return *this;
}

auto VulkanTextureBackend::matches_borrowed_target(
  const mln_vulkan_borrowed_texture_descriptor& descriptor
) const -> bool {
  // Nothing is built yet, so there is no render pass to be incompatible with.
  if (!resource) {
    return true;
  }
  return getResource<VulkanTextureRenderableResource>().matches_borrowed(
    descriptor
  );
}

void VulkanTextureBackend::set_borrowed_target(
  const mln_vulkan_borrowed_texture_descriptor& descriptor
) {
  const auto new_size =
    mln::Size{descriptor.physical_width, descriptor.physical_height};
  if (!resource) {
    borrowed_descriptor_ = descriptor;
    setSize(new_size);
    return;
  }
  getResource<VulkanTextureRenderableResource>().set_borrowed(
    descriptor, new_size.width, new_size.height
  );
  borrowed_descriptor_ = descriptor;
  size = new_size;
}

void VulkanTextureBackend::resize(mln::Size new_size) {
  // Slots keep their old resources until the common ring selects a released
  // slot for rendering at the new extent.
  size = new_size;
}

auto VulkanTextureBackend::readStillImage() -> mln::PremultipliedImage {
  prepareRenderResources();

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
  if (!commandPool) {
    initCommandPool();
  }
}

auto VulkanTextureBackend::frame_resources(std::size_t slot)
  -> VulkanTextureFrameResources {
  if (slot >= slot_resources_.size()) {
    return {};
  }
  if (slot == selected_slot_) {
    prepareRenderResources();
  } else if (slot_resources_[slot] == nullptr) {
    const auto previous = selected_slot_;
    if (!select_slot(slot)) return {};
    prepareRenderResources();
    static_cast<void>(select_slot(previous));
  }
  auto* rendered = slot == selected_slot_
                     ? &getResource<VulkanTextureRenderableResource>()
                     : static_cast<VulkanTextureRenderableResource*>(
                         slot_resources_[slot].get()
                       );
  return VulkanTextureFrameResources{
    .image = rendered->image(),
    .image_view = rendered->image_view(),
    .device = device.get(),
    .format = rendered->format(),
  };
}

auto VulkanTextureBackend::select_slot(std::size_t slot) -> bool {
  if (slot >= slot_resources_.size()) return false;
  if (slot == selected_slot_) {
    if (resource != nullptr && slot_sizes_[slot] != size) resource.reset();
    return true;
  }
  slot_resources_[selected_slot_] = std::move(resource);
  if (slot_resources_[slot] != nullptr && slot_sizes_[slot] != size) {
    slot_resources_[slot].reset();
  }
  resource = std::move(slot_resources_[slot]);
  selected_slot_ = slot;
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
  physicalDeviceFeatures = physicalDevice.getFeatures(dispatcher);
}

void VulkanTextureBackend::initSwapchain() {
  auto& renderable = getDefaultRenderable();
  auto& renderable_resource =
    renderable.getResource<VulkanTextureRenderableResource>();
  const auto& size = renderable.getSize();

  maxFrames = 1;
  if (uses_borrowed_texture_) {
    renderable_resource.init_borrowed(
      borrowed_descriptor_, size.width, size.height
    );
  } else {
    renderable_resource.init_sampled(vk::Extent2D{size.width, size.height});
  }
}

// Base override requires a member function.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
auto VulkanTextureBackend::getDeviceExtensions() -> std::vector<const char*> {
  return {};
}

}  // namespace mln::core
