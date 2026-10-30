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

// Waits on the frame fence. A failed wait, such as a lost device, leaves the
// frame's GPU work unfinished, so the render or readback fails instead of
// publishing that frame as complete.
void wait_frame(const mln::vulkan::Context& context) {
  if (!context.waitFrame()) {
    throw std::runtime_error("Vulkan frame fence wait failed");
  }
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
  // lacks. A slot whose frame the host holds is never selected, so its color
  // image survives a rebuild of another slot.
  void select(std::size_t slot, vk::Extent2D size) {
    // A queue-ordered frame may still render, and a frame released in queue
    // order may still be read, so a rebuild first waits for the shared queue.
    const auto& built = slots_.at(slot);
    if (
      renderPass &&
      (depth_extent_ != size || (built.image && built.extent != size))
    )
      backend.getGraphicsQueue().waitIdle(backend.getDispatcher());
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
      if (slot.release) {
        backend.getDevice()->freeCommandBuffers(
          owner_.sync_pool_.get(), slot.release, backend.getDispatcher()
        );
      }
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
    // A discarded render publishes no frame, so its image stays.
    auto release = vk::CommandBuffer{};
    if (!mln::core::discard_renderable_present && owner_.handoff_family()) {
      auto& current = slots_.at(selected_);
      if (!current.release) {
        current.release = owner_.record_release(current.image);
      }
      release = current.release;
    }
    owner_.submit_after_frame(release);
    // Otherwise the frame is acquirable once submitted. The next frame's begin
    // waits for it, so one frame at most is in flight.
    if (owner_.producer_sync_kind_ != MLN_GPU_SYNC_CPU_COMPLETE) return;
    // This resource is only used by VulkanTextureBackend, so the downcast is
    // invariant within this file.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    wait_frame(static_cast<mln::vulkan::Context&>(backend.getContext()));
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
    // A borrowed image's release to its owner, recorded on first use. Each
    // render takes the image back without a transfer, since it clears it.
    vk::CommandBuffer release;
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
    // The external dependencies span every stage the queue runs, so they also
    // order the host's reads that a queue-ordered frame shares the queue with:
    // before the render overwrites the image, and after its writes and final
    // layout. ALL_COMMANDS means only the graphics stages in a subpass
    // dependency, so transfer and compute are named.
    auto queue_stages =
      vk::PipelineStageFlags{vk::PipelineStageFlagBits::eAllGraphics} |
      vk::PipelineStageFlagBits::eTransfer;
    const auto families =
      backend.getPhysicalDevice().getQueueFamilyProperties(dispatcher);
    if (
      families.at(static_cast<std::size_t>(backend.getGraphicsQueueIndex()))
        .queueFlags &
      vk::QueueFlagBits::eCompute
    )
      queue_stages |= vk::PipelineStageFlagBits::eComputeShader;
    const std::array<vk::SubpassDependency, 3> dependencies = {
      vk::SubpassDependency()
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(queue_stages)
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
        .setDstStageMask(queue_stages)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eMemoryRead),
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
    wait_frame(context_impl);
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
  // A queue-ordered frame may still render, and the host's reads of one it
  // released may still run, so the ring's images wait for the shared queue.
  if (graphicsQueue) {
    try {
      graphicsQueue.waitIdle(dispatcher);
    } catch (const vk::DeviceLostError&) {
      // A lost device has finished every submission. Any other failure leaves
      // work in flight, so it ends the process, as mbgl's teardown does.
    }
  }
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
  // pass, so a replacement keeps them to keep those pipelines usable. It keeps
  // the owner too, since the outgoing images may still be with it.
  return descriptor.format == borrowed_descriptor_.format &&
         descriptor.initial_layout == borrowed_descriptor_.initial_layout &&
         descriptor.final_layout == borrowed_descriptor_.final_layout &&
         descriptor.owner == borrowed_descriptor_.owner;
}

void VulkanTextureBackend::set_borrowed_target(
  const VulkanBorrowedTarget& target
) {
  const auto new_size = mln::Size{
    target.descriptor.physical_width, target.descriptor.physical_height
  };
  // Once this wait returns, the session is done with the outgoing images, and
  // the host may destroy them when set_target completes. The queue drains past
  // the session's last frame and the handoffs that follow it. A device wait
  // would also need the host's other queues, which the session must not touch.
  if (context) graphicsQueue.waitIdle(dispatcher);
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

auto VulkanTextureBackend::producer_sync() const -> mln_gpu_sync {
  return mln_gpu_sync{
    .size = sizeof(mln_gpu_sync),
    .kind = producer_sync_kind_,
    .handle =
      timeline_
        ? vulkan_handle_to_abi(static_cast<VkSemaphore>(timeline_.get()))
        : 0,
    .value = timeline_value_,
  };
}

auto VulkanTextureBackend::begin_sync_commands() -> vk::CommandBuffer {
  if (!sync_pool_) {
    sync_pool_ = device->createCommandPoolUnique(
      vk::CommandPoolCreateInfo().setQueueFamilyIndex(
        static_cast<uint32_t>(graphicsQueueIndex)
      ),
      nullptr, dispatcher
    );
  }
  auto commands = device
                    ->allocateCommandBuffers(
                      vk::CommandBufferAllocateInfo()
                        .setCommandPool(sync_pool_.get())
                        .setLevel(vk::CommandBufferLevel::ePrimary)
                        .setCommandBufferCount(1),
                      dispatcher
                    )
                    .front();
  commands.begin(
    vk::CommandBufferBeginInfo().setFlags(
      vk::CommandBufferUsageFlagBits::eSimultaneousUse
    ),
    dispatcher
  );
  return commands;
}

auto VulkanTextureBackend::handoff_family() const -> std::optional<uint32_t> {
  if (!uses_borrowed_texture_) return std::nullopt;
  switch (borrowed_descriptor_.owner) {
    case MLN_VULKAN_IMAGE_OWNER_EXTERNAL:
      return VK_QUEUE_FAMILY_EXTERNAL;
    case MLN_VULKAN_IMAGE_OWNER_FOREIGN:
      return VK_QUEUE_FAMILY_FOREIGN_EXT;
    default:
      return std::nullopt;
  }
}

auto VulkanTextureBackend::record_release(vk::Image image)
  -> vk::CommandBuffer {
  const auto graphics = static_cast<uint32_t>(graphicsQueueIndex);
  const auto layout = static_cast<vk::ImageLayout>(frame_layout());
  const auto barrier =
    vk::ImageMemoryBarrier()
      .setImage(image)
      .setOldLayout(layout)
      .setNewLayout(layout)
      .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
      .setSrcQueueFamilyIndex(graphics)
      .setDstQueueFamilyIndex(handoff_family().value_or(graphics))
      .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  auto commands = begin_sync_commands();
  commands.pipelineBarrier(
    vk::PipelineStageFlagBits::eAllCommands,
    vk::PipelineStageFlagBits::eBottomOfPipe, {}, nullptr, nullptr, barrier,
    dispatcher
  );
  commands.end(dispatcher);
  return commands;
}

void VulkanTextureBackend::submit_after_frame(vk::CommandBuffer release) {
  const auto signals =
    producer_sync_kind_ == MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE;
  if (!release && !signals) return;
  if (signals && !timeline_) {
    auto type = vk::SemaphoreTypeCreateInfo()
                  .setSemaphoreType(vk::SemaphoreType::eTimeline)
                  .setInitialValue(0);
    timeline_ = device->createSemaphoreUnique(
      vk::SemaphoreCreateInfo().setPNext(&type), nullptr, dispatcher
    );
  }
  // A signal's first scope holds everything submitted to the queue before it,
  // so this batch signals once the frame and the release complete.
  const auto value = timeline_value_ + 1;
  const auto timeline =
    vk::TimelineSemaphoreSubmitInfo().setSignalSemaphoreValues(value);
  const auto semaphore = timeline_.get();
  auto submit = vk::SubmitInfo();
  if (release) submit.setCommandBuffers(release);
  if (signals) submit.setPNext(&timeline).setSignalSemaphores(semaphore);
  graphicsQueue.submit(submit, nullptr, dispatcher);
  if (signals) timeline_value_ = value;
  // A frame that completes on the CPU completes with its release.
  if (producer_sync_kind_ == MLN_GPU_SYNC_CPU_COMPLETE)
    graphicsQueue.waitIdle(dispatcher);
}

void VulkanTextureBackend::wait_timeline(
  VkSemaphore semaphore, uint64_t value
) {
  // A wait's second scope holds every later submission to the queue, so a
  // batch without commands orders the session's later work.
  const auto timeline =
    vk::TimelineSemaphoreSubmitInfo().setWaitSemaphoreValues(value);
  const auto waited = vk::Semaphore(semaphore);
  const auto stage =
    vk::PipelineStageFlags{vk::PipelineStageFlagBits::eAllCommands};
  graphicsQueue.submit(
    vk::SubmitInfo()
      .setPNext(&timeline)
      .setWaitSemaphores(waited)
      .setWaitDstStageMask(stage),
    nullptr, dispatcher
  );
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

  // The host only reads this buffer, so prefer cached memory: copying out of
  // uncached memory takes about 40 times as long on discrete GPUs. Cached
  // memory may not be coherent, so the copy invalidates it first.
  auto allocation_info = VmaAllocationCreateInfo{};
  allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
  allocation_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
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
  wait_frame(context_impl);
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

      // The fence wait alone does not make the copy visible to the host.
      const auto to_host =
        vk::BufferMemoryBarrier()
          .setBuffer(buffer_allocation.buffer)
          .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
          .setDstAccessMask(vk::AccessFlagBits::eHostRead)
          .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setOffset(0)
          .setSize(VK_WHOLE_SIZE);
      command_buffer->pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost,
        {}, nullptr, to_host, nullptr, getDispatcher()
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

  if (
    vmaInvalidateAllocation(
      allocator, buffer_allocation.allocation, 0, VK_WHOLE_SIZE
    ) != VK_SUCCESS
  ) {
    throw std::runtime_error("Vulkan readback host memory invalidate failed");
  }
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
