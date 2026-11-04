#include <algorithm>
#include <any>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/headless_backend.hpp>
#include <mln/util/size.hpp>

#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_texture_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/render_target.h"
#include "maplibre_native_c/texture.h"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"
#include "render/vulkan/vulkan_device.hpp"
#include "render/vulkan/vulkan_dispatch.hpp"
#include "render/vulkan/vulkan_handle.hpp"
#include "render/vulkan/vulkan_texture_backend.hpp"

// Linux and FreeBSD sessions can render into dmabufs that they allocate.
// Android and OpenHarmony define __linux__ too.
#if (defined(__linux__) || defined(__FreeBSD__)) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
#define MLN_FFI_VULKAN_DMA_BUFS 1
#include <algorithm>
#include <bit>
#include <stdexcept>

#include "render/dma_buf_session.hpp"
#endif

#if defined(_WIN32)
#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>

#include "render/dxgi_shared_texture_session.hpp"
#include "render/vulkan/vulkan_dxgi_texture.hpp"
#endif

#if defined(__ANDROID__)
#include <bit>
#include <stdexcept>

#include <android/hardware_buffer.h>

#include "platform/android/hardware_buffer.hpp"
#include "render/android_hardware_buffer_session.hpp"
#include "render/vulkan_device.hpp"
#endif

namespace {

using mln::core::VulkanBorrowedTarget;
using mln::core::VulkanBorrowedTextures;

// The shared descriptor defaults and validator are built without the Vulkan
// headers, so they spell these as plain zeros.
static_assert(VK_FORMAT_UNDEFINED == 0);
static_assert(VK_IMAGE_LAYOUT_UNDEFINED == 0);

auto validate_vulkan_handles(const mln_vulkan_context_descriptor& context)
  -> mln_status {
  auto* const instance = static_cast<VkInstance>(context.instance);
  auto* const physical_device =
    static_cast<VkPhysicalDevice>(context.physical_device);

  auto dispatcher = mln::core::vulkan_dispatch_loader(context);
  mln::core::vulkan_init_instance_dispatch(dispatcher, context);
  if (
    dispatcher.vkEnumeratePhysicalDevices == nullptr ||
    dispatcher.vkGetPhysicalDeviceQueueFamilyProperties == nullptr
  ) {
    mln::core::set_thread_error("Vulkan dispatch functions must resolve");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto physical_device_count = uint32_t{};
  auto result = dispatcher.vkEnumeratePhysicalDevices(
    instance, &physical_device_count, nullptr
  );
  if (result != VK_SUCCESS || physical_device_count == 0) {
    mln::core::set_thread_error(
      "Vulkan instance must expose at least one physical device"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto physical_devices = std::vector<VkPhysicalDevice>(physical_device_count);
  result = dispatcher.vkEnumeratePhysicalDevices(
    instance, &physical_device_count, physical_devices.data()
  );
  if (result != VK_SUCCESS) {
    mln::core::set_thread_error("failed to enumerate Vulkan physical devices");
    return MLN_STATUS_NATIVE_ERROR;
  }

  auto found_physical_device = false;
  for (auto* const candidate : physical_devices) {
    if (candidate == physical_device) {
      found_physical_device = true;
      break;
    }
  }
  if (!found_physical_device) {
    mln::core::set_thread_error(
      "Vulkan physical_device must belong to instance"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto queue_family_count = uint32_t{};
  dispatcher.vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device, &queue_family_count, nullptr
  );
  if (context.graphics_queue_family_index >= queue_family_count) {
    mln::core::set_thread_error(
      "Vulkan graphics_queue_family_index is out of range"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto queue_families =
    std::vector<VkQueueFamilyProperties>(queue_family_count);
  dispatcher.vkGetPhysicalDeviceQueueFamilyProperties(
    physical_device, &queue_family_count, queue_families.data()
  );
  const auto& queue_family =
    queue_families.at(context.graphics_queue_family_index);
  if (
    (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ||
    queue_family.queueCount == 0
  ) {
    mln::core::set_thread_error(
      "Vulkan graphics_queue_family_index must support graphics"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

// The sync kinds that the host's device adds. A device resolves an
// extension's commands only once the host enables it, which tells the session
// what the host enabled; whether it enabled the timelineSemaphore feature that
// Vulkan 1.2 or VK_KHR_timeline_semaphore offers is the host's own contract.
auto vulkan_sync_support(const mln_vulkan_context_descriptor& context)
  -> mln::core::GpuSyncSupport {
  auto support = mln::core::GpuSyncSupport{};
  auto dispatcher = mln::core::vulkan_dispatch_loader(context);
  mln::core::vulkan_init_instance_dispatch(dispatcher, context);
  if (
    dispatcher.vkGetPhysicalDeviceFeatures2 == nullptr ||
    dispatcher.vkGetPhysicalDeviceExternalSemaphoreProperties == nullptr
  )
    return support;
  mln::core::vulkan_init_device_dispatch(
    dispatcher, static_cast<VkDevice>(context.device), context
  );
  auto* const physical_device =
    static_cast<VkPhysicalDevice>(context.physical_device);
  auto timeline = VkPhysicalDeviceTimelineSemaphoreFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
    .pNext = nullptr,
    .timelineSemaphore = VK_FALSE,
  };
  auto features = VkPhysicalDeviceFeatures2{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &timeline,
    .features = {},
  };
  dispatcher.vkGetPhysicalDeviceFeatures2(physical_device, &features);
  if (
    timeline.timelineSemaphore == VK_TRUE &&
    dispatcher.vkGetSemaphoreCounterValue != nullptr
  ) {
    support.producer |= MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE;
    support.consumer |= MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE;
#if defined(__APPLE__)
    // MoltenVK backs a timeline with an MTLSharedEvent, which it exports and
    // imports once the host enables VK_EXT_metal_objects.
    if (
      dispatcher.vkGetDeviceProcAddr(
        static_cast<VkDevice>(context.device), "vkExportMetalObjectsEXT"
      ) != nullptr
    ) {
      support.producer |= MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT;
      support.consumer |= MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT;
    }
#endif
#if defined(_WIN32)
    // A driver may share a timeline as a D3D12 fence, once the host enables
    // VK_KHR_external_semaphore_win32.
    auto type = VkSemaphoreTypeCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .pNext = nullptr,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue = 0,
    };
    const auto fence = VkPhysicalDeviceExternalSemaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
      .pNext = &type,
      .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT,
    };
    auto shared = VkExternalSemaphoreProperties{
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES,
      .pNext = nullptr,
      .exportFromImportedHandleTypes = 0,
      .compatibleHandleTypes = 0,
      .externalSemaphoreFeatures = 0,
    };
    dispatcher.vkGetPhysicalDeviceExternalSemaphoreProperties(
      physical_device, &fence, &shared
    );
    auto* const device = static_cast<VkDevice>(context.device);
    if (
      (shared.externalSemaphoreFeatures &
       VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT) != 0 &&
      dispatcher.vkGetDeviceProcAddr(device, "vkGetSemaphoreWin32HandleKHR") !=
        nullptr
    )
      support.producer |= MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE;
    if (
      (shared.externalSemaphoreFeatures &
       VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT) != 0 &&
      dispatcher.vkGetDeviceProcAddr(
        device, "vkImportSemaphoreWin32HandleKHR"
      ) != nullptr
    )
      support.consumer |= MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE;
#endif
  }
  const auto sync_fd = VkPhysicalDeviceExternalSemaphoreInfo{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
    .pNext = nullptr,
    .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
  };
  auto properties = VkExternalSemaphoreProperties{
    .sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES,
    .pNext = nullptr,
    .exportFromImportedHandleTypes = 0,
    .compatibleHandleTypes = 0,
    .externalSemaphoreFeatures = 0,
  };
  dispatcher.vkGetPhysicalDeviceExternalSemaphoreProperties(
    physical_device, &sync_fd, &properties
  );
  if (
    (properties.externalSemaphoreFeatures &
     VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT) != 0 &&
    dispatcher.vkGetSemaphoreFdKHR != nullptr
  )
    support.producer |= MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
  if (
    (properties.externalSemaphoreFeatures &
     VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT) != 0 &&
    dispatcher.vkImportSemaphoreFdKHR != nullptr
  )
    support.consumer |= MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
  return support;
}

class VulkanTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  VulkanTextureSessionBackend(
    const mln_vulkan_context_descriptor& context, mln::Size size,
    std::shared_ptr<const mln::core::QueueLock> queue_lock
  )
      : backend_(context, size, std::move(queue_lock)) {}

  VulkanTextureSessionBackend(
    const VulkanBorrowedTarget& target, mln::Size size,
    std::shared_ptr<const mln::core::QueueLock> queue_lock
  )
      : backend_(target, size, std::move(queue_lock)) {}

  VulkanTextureSessionBackend(
    const mln_vulkan_context_descriptor& context,
    mln::core::VulkanExportedRing ring, mln::Size size,
    std::shared_ptr<const mln::core::QueueLock> queue_lock
  )
      : backend_(context, std::move(ring), size, std::move(queue_lock)) {}

  auto headless_backend() -> mln::gfx::HeadlessBackend& override {
    return backend_;
  }

  void resize(mln::Size size) override { backend_.set_ring_size(size); }

  void set_producer_sync_kind(mln_gpu_sync_kind kind) {
    backend_.set_producer_sync_kind(kind);
  }

  auto producer_sync() const -> mln_gpu_sync override {
    return backend_.producer_sync();
  }

  auto release_consumer_sync(const mln_gpu_sync& sync) -> mln_status override {
    try {
      auto guard = mln::gfx::BackendScope{backend_};
      if (sync.kind == MLN_GPU_SYNC_SYNC_FD) {
        backend_.wait_sync_fd(
          static_cast<int>(static_cast<int64_t>(sync.handle))
        );
      } else if (sync.kind == MLN_GPU_SYNC_D3D12_FENCE) {
        backend_.wait_d3d12_fence(
          // The C ABI carries the handle as its bit pattern.
          // NOLINTNEXTLINE(performance-no-int-to-ptr)
          reinterpret_cast<void*>(static_cast<uintptr_t>(sync.handle)),
          sync.value
        );
      } else if (sync.kind == MLN_GPU_SYNC_METAL_SHARED_EVENT) {
        backend_.wait_metal_event(
          // The C ABI carries the event as the bit pattern of its pointer.
          // NOLINTNEXTLINE(performance-no-int-to-ptr)
          reinterpret_cast<void*>(static_cast<uintptr_t>(sync.handle)),
          sync.value
        );
      } else {
        backend_.wait_timeline(
          mln::core::vulkan_handle_from_abi<VkSemaphore>(sync.handle),
          sync.value
        );
      }
      return MLN_STATUS_OK;
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("waiting for the consumer semaphore failed");
    }
    return MLN_STATUS_NATIVE_ERROR;
  }

  [[nodiscard]] auto allows_off_thread_teardown() const noexcept
    -> bool override {
    return true;
  }

  auto prepare_off_thread_teardown() noexcept -> bool override {
    return backend_.drain_for_teardown();
  }

  void quarantine() noexcept override {
    backend_.release_queue_access();
    backend_.close_sync_files();
  }

  auto set_vulkan_borrowed_target(const VulkanBorrowedTextures& target)
    -> mln_status override {
    backend_.set_borrowed_target(target);
    return MLN_STATUS_OK;
  }

  auto release_borrowed_target() -> mln_status override {
    backend_.release_borrowed_target();
    return MLN_STATUS_OK;
  }

  void prepare_render_resources() override {
    // Renderer::render creates the Vulkan context before requesting the default
    // renderable, so shared-device resources must be ready first.
    backend_.prepareRenderResources();
  }

  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }

  auto record_frame_metadata(
    const mln::core::RenderFrameMetadata& frame, std::any& out_metadata
  ) -> mln_status override {
    const auto resources = backend_.frame_resources();
    if (resources.image == VK_NULL_HANDLE) {
      throw mln::core::RenderTargetNotReady(
        "rendered Vulkan image is not available"
      );
    }
    if (resources.exported != nullptr) {
      out_metadata = resources.exported->frame_metadata(frame);
      return MLN_STATUS_OK;
    }
    out_metadata = mln_vulkan_texture_frame{
      .size = sizeof(mln_vulkan_texture_frame),
      .width = frame.physical_width,
      .height = frame.physical_height,
      .scale_factor = frame.scale_factor,
      .slot = frame.slot,
      .image = mln::core::vulkan_handle_to_abi(resources.image),
      .image_view = mln::core::vulkan_handle_to_abi(resources.image_view),
      .device = resources.device,
      .format = static_cast<uint32_t>(resources.format),
      .layout = static_cast<uint32_t>(backend_.frame_layout()),
    };
    return MLN_STATUS_OK;
  }

 private:
  mln::core::VulkanTextureBackend backend_;
};

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_VULKAN;
}

auto create_vulkan_offscreen_texture_backend(
  const mln_vulkan_context_descriptor& context, mln::Size size
) -> std::unique_ptr<TextureSessionBackend> {
  return std::make_unique<VulkanTextureSessionBackend>(context, size, nullptr);
}

auto vulkan_borrowed_texture_attach_start(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  size_new_borrowed_session(
    *session, descriptor->logical_size, descriptor->physical_width,
    descriptor->physical_height
  );
  session->target_identity = render_target_identity(
    std::span{descriptor->textures, descriptor->texture_count}
  );
  session->texture.mode = TextureSessionMode::Borrowed;
  session->accepts_queue_lock = true;
  session->producer_sync_kinds |= MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER;
  session->probe_sync_kinds = [context = descriptor->context] {
    return vulkan_sync_support(context);
  };
  // Taken now, because the host may release a device of the library's as
  // soon as this call returns.
  session->initialize_backend = [target = VulkanBorrowedTarget{*descriptor},
                                 device = find_vulkan_device(
                                   descriptor->context.device
                                 )](mln_render_session_object& live) mutable {
    const auto handles_status =
      validate_vulkan_handles(target.descriptor.context);
    if (handles_status != MLN_STATUS_OK) {
      return handles_status;
    }
    auto backend = std::make_unique<VulkanTextureSessionBackend>(
      target, mln::Size{live.physical_width, live.physical_height},
      live.queue_lock
    );
    backend->set_producer_sync_kind(live.producer_sync_kind);
    if (device != nullptr) backend->retain(std::move(device));
    live.texture.backend = std::move(backend);
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = static_cast<uint32_t>(descriptor->texture_count),
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION,
    .producer_sync_kind = {},
    .consumer_sync_kinds =
      MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE | MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_vulkan_borrowed_texture>()
  );
}

auto vulkan_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  const auto replacement_status =
    validate_vulkan_borrowed_texture_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) {
    return replacement_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    replacement->physical_width, replacement->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::BorrowedTexture, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) {
    return submission_status;
  }
  auto target = VulkanBorrowedTextures{*replacement};
  auto identity = render_target_identity(std::span{target.textures});
  return enqueue_borrowed_texture_retarget(
    session, replacement->texture_count, replacement->logical_size,
    target.physical_size, std::move(identity),
    [target = std::move(target)](mln_render_session_object& live, mln::Size) {
      return live.texture.backend->set_vulkan_borrowed_target(target);
    },
    std::move(checked_completion),
    valueless_completion<
      &mln_render_session_set_vulkan_borrowed_texture_target>()
  );
}

#if defined(__ANDROID__)
namespace {

// A slot's image over an Android hardware buffer that the ring allocated.
class HardwareBufferImage final : public VulkanSlotImage {
 public:
  HardwareBufferImage(
    const vk::UniqueDevice& device,
    const mln::vulkan::DispatchLoaderDynamic& dispatcher, vk::Extent2D extent,
    uint64_t usage
  )
      : buffer_(
          platform::android::allocate_render_buffer(
            extent.width, extent.height, usage
          )
        ) {
    auto format = VkAndroidHardwareBufferFormatPropertiesANDROID{};
    format.sType =
      VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID;
    auto properties = VkAndroidHardwareBufferPropertiesANDROID{};
    properties.sType =
      VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;
    properties.pNext = &format;
    const auto result = dispatcher.vkGetAndroidHardwareBufferPropertiesANDROID(
      device.get(), buffer_.get(), &properties
    );
    if (result != VK_SUCCESS || properties.memoryTypeBits == 0) {
      throw std::runtime_error(
        "Vulkan can't import the Android hardware buffer"
      );
    }
    const auto external = vk::ExternalMemoryImageCreateInfo().setHandleTypes(
      vk::ExternalMemoryHandleTypeFlagBits::eAndroidHardwareBufferANDROID
    );
    image_ = device->createImageUnique(
      vk::ImageCreateInfo()
        .setPNext(&external)
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .setExtent({extent.width, extent.height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(
          vk::ImageUsageFlags{vk::ImageUsageFlagBits::eColorAttachment} |
          vk::ImageUsageFlagBits::eSampled
        )
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined),
      nullptr, dispatcher
    );
    // Vulkan imports a hardware buffer only into a dedicated allocation.
    const auto dedicated = vk::MemoryDedicatedAllocateInfo().setImage(*image_);
    const auto imported = vk::ImportAndroidHardwareBufferInfoANDROID()
                            .setPNext(&dedicated)
                            .setBuffer(buffer_.get());
    memory_ = device->allocateMemoryUnique(
      vk::MemoryAllocateInfo()
        .setPNext(&imported)
        .setAllocationSize(properties.allocationSize)
        .setMemoryTypeIndex(
          static_cast<uint32_t>(std::countr_zero(properties.memoryTypeBits))
        ),
      nullptr, dispatcher
    );
    device->bindImageMemory(*image_, *memory_, 0, dispatcher);
    view_ = device->createImageViewUnique(
      vk::ImageViewCreateInfo()
        .setImage(*image_)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .setComponents(vk::ComponentMapping())
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
      nullptr, dispatcher
    );
  }

  [[nodiscard]] auto image() const -> vk::Image override { return *image_; }
  [[nodiscard]] auto view() const -> vk::ImageView override { return *view_; }
  [[nodiscard]] auto frame_metadata(const RenderFrameMetadata& frame) const
    -> std::any override {
    return mln_android_hardware_buffer_frame{
      .size = sizeof(mln_android_hardware_buffer_frame),
      .width = frame.physical_width,
      .height = frame.physical_height,
      .scale_factor = frame.scale_factor,
      .slot = frame.slot,
      .buffer = buffer_.get(),
    };
  }

 private:
  // Declared in reverse of the order they must be destroyed in.
  platform::android::HardwareBuffer buffer_;
  vk::UniqueDeviceMemory memory_;
  vk::UniqueImage image_;
  vk::UniqueImageView view_;
};

}  // namespace

auto configure_android_hardware_buffer_session(
  const AndroidHardwareBufferRing& ring, mln_vulkan_device device,
  mln_render_session_object& session
) -> mln_status {
  auto context = std::shared_ptr<const mln_vulkan_context_descriptor>{};
  const auto device_status = texture_ring_device(device, context);
  if (device_status != MLN_STATUS_OK) return device_status;
  if (context == nullptr) {
    const auto shared_status = shared_presentation_device(context);
    if (shared_status != MLN_STATUS_OK) return shared_status;
  }
  auto dispatcher = vulkan_dispatch_loader(*context);
  vulkan_init_instance_dispatch(dispatcher, *context);
  vulkan_init_device_dispatch(
    dispatcher, static_cast<VkDevice>(context->device), *context
  );
  // A device resolves an extension's commands only once it enables it.
  if (dispatcher.vkGetAndroidHardwareBufferPropertiesANDROID == nullptr) {
    set_thread_error(
      "the Vulkan device can't import Android hardware buffers, because it "
      "lacks VK_ANDROID_external_memory_android_hardware_buffer"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  session.accepts_queue_lock = true;
  // The buffers' readers work outside this device, so a frame hands over as a
  // sync file or as completed work.
  session.probe_sync_kinds = [context] {
    auto support = vulkan_sync_support(*context);
    support.producer &= MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
    support.consumer &= MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
    return support;
  };
  session.initialize_backend = [context,
                                ring](mln_render_session_object& live) {
    // Each frame leaves its image to readers outside Vulkan, such as Android's
    // UI renderer, for which the foreign queue family stands.
    auto exported = VulkanExportedRing{
      .depth = ring.buffer_count,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .final_layout = VK_IMAGE_LAYOUT_GENERAL,
      .owner = MLN_VULKAN_IMAGE_OWNER_FOREIGN,
      .allocate = [usage = ring.usage](
                    const vk::UniqueDevice& device,
                    const mln::vulkan::DispatchLoaderDynamic& dispatcher,
                    vk::Extent2D extent
                  ) -> std::unique_ptr<VulkanSlotImage> {
        return std::make_unique<HardwareBufferImage>(
          device, dispatcher, extent, usage
        );
      },
    };
    auto backend = std::make_unique<VulkanTextureSessionBackend>(
      *context, std::move(exported),
      mln::Size{live.physical_width, live.physical_height}, live.queue_lock
    );
    backend->set_producer_sync_kind(live.producer_sync_kind);
    backend->retain(context);
    live.texture.backend = std::move(backend);
    return MLN_STATUS_OK;
  };
  return MLN_STATUS_OK;
}
#endif

#if defined(MLN_FFI_VULKAN_DMA_BUFS)
namespace {

constexpr auto dma_buf_usage =
  vk::ImageUsageFlags{vk::ImageUsageFlagBits::eColorAttachment} |
  vk::ImageUsageFlagBits::eSampled;

auto dma_buf_vk_format(uint32_t fourcc) -> vk::Format {
  return fourcc == drm_format_argb8888 ? vk::Format::eB8G8R8A8Unorm
                                       : vk::Format::eR8G8B8A8Unorm;
}

// What a ring's images take: a format, the modifiers that the device may pick
// from for each image with each one's count of memory planes, and the
// device's memory types.
struct VulkanDmaBufFormat {
  uint32_t fourcc = 0;
  vk::Format format = vk::Format::eUndefined;
  std::vector<uint64_t> modifiers;
  std::vector<uint32_t> plane_counts;
  std::vector<vk::MemoryPropertyFlags> memory_types;
};

// The memory type of an image that `bits` allows. A linear image goes where
// the CPU can map it, so that a reader can map its dmabuf.
auto dma_buf_memory_type(
  uint32_t bits, const std::vector<vk::MemoryPropertyFlags>& types, bool linear
) -> uint32_t {
  const auto mappable =
    vk::MemoryPropertyFlags{vk::MemoryPropertyFlagBits::eHostVisible};
  const auto local =
    vk::MemoryPropertyFlags{vk::MemoryPropertyFlagBits::eDeviceLocal};
  const auto preferred =
    linear ? std::vector{local | mappable, mappable} : std::vector{local};
  for (const auto flags : preferred) {
    for (auto index = uint32_t{0}; index < types.size(); ++index) {
      if (((bits >> index) & 1U) != 0 && (types[index] & flags) == flags) {
        return index;
      }
    }
  }
  return static_cast<uint32_t>(std::countr_zero(bits));
}

// The modifiers of `format` that the device renders and samples, in at most
// as many memory planes as a dmabuf has.
auto device_modifiers(
  vk::PhysicalDevice physical_device,
  const mln::vulkan::DispatchLoaderDynamic& dispatcher, vk::Format format
) -> std::vector<vk::DrmFormatModifierPropertiesEXT> {
  auto list = vk::DrmFormatModifierPropertiesListEXT{};
  auto properties = vk::FormatProperties2{};
  properties.pNext = &list;
  physical_device.getFormatProperties2(format, &properties, dispatcher);
  auto modifiers = std::vector<vk::DrmFormatModifierPropertiesEXT>(
    list.drmFormatModifierCount
  );
  list.pDrmFormatModifierProperties = modifiers.data();
  physical_device.getFormatProperties2(format, &properties, dispatcher);
  modifiers.resize(list.drmFormatModifierCount);
  const auto features =
    vk::FormatFeatureFlags{vk::FormatFeatureFlagBits::eColorAttachment} |
    vk::FormatFeatureFlagBits::eSampledImage;
  std::erase_if(modifiers, [features](const auto& modifier) {
    return modifier.drmFormatModifierPlaneCount < 1 ||
           modifier.drmFormatModifierPlaneCount > dma_buf_max_planes ||
           (modifier.drmFormatModifierTilingFeatures & features) != features;
  });
  return modifiers;
}

// Whether the device exports a dmabuf of an image in `format` with
// `modifier`.
auto exports_dma_buf(
  vk::PhysicalDevice physical_device,
  const mln::vulkan::DispatchLoaderDynamic& dispatcher, vk::Format format,
  uint64_t modifier
) -> bool {
  const auto modifier_info = vk::PhysicalDeviceImageDrmFormatModifierInfoEXT()
                               .setDrmFormatModifier(modifier)
                               .setSharingMode(vk::SharingMode::eExclusive);
  const auto external_info =
    vk::PhysicalDeviceExternalImageFormatInfo()
      .setPNext(&modifier_info)
      .setHandleType(vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT);
  const auto info = vk::PhysicalDeviceImageFormatInfo2()
                      .setPNext(&external_info)
                      .setFormat(format)
                      .setType(vk::ImageType::e2D)
                      .setTiling(vk::ImageTiling::eDrmFormatModifierEXT)
                      .setUsage(dma_buf_usage);
  auto external = vk::ExternalImageFormatProperties{};
  auto properties = vk::ImageFormatProperties2{};
  properties.pNext = &external;
  return physical_device.getImageFormatProperties2(
           &info, &properties, dispatcher
         ) == vk::Result::eSuccess &&
         (external.externalMemoryProperties.externalMemoryFeatures &
          vk::ExternalMemoryFeatureFlagBits::eExportable);
}

// A slot's image in memory that it exports as a dmabuf.
class DmaBufImage final : public VulkanSlotImage {
 public:
  DmaBufImage(
    const vk::UniqueDevice& device,
    const mln::vulkan::DispatchLoaderDynamic& dispatcher, vk::Extent2D extent,
    const VulkanDmaBufFormat& format
  ) {
    const auto modifiers =
      vk::ImageDrmFormatModifierListCreateInfoEXT().setDrmFormatModifiers(
        format.modifiers
      );
    const auto external =
      vk::ExternalMemoryImageCreateInfo()
        .setPNext(&modifiers)
        .setHandleTypes(vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT);
    image_ = device->createImageUnique(
      vk::ImageCreateInfo()
        .setPNext(&external)
        .setImageType(vk::ImageType::e2D)
        .setFormat(format.format)
        .setExtent({extent.width, extent.height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eDrmFormatModifierEXT)
        .setUsage(dma_buf_usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined),
      nullptr, dispatcher
    );
    auto chosen = vk::ImageDrmFormatModifierPropertiesEXT{};
    if (
      device->getImageDrmFormatModifierPropertiesEXT(
        *image_, &chosen, dispatcher
      ) != vk::Result::eSuccess
    ) {
      throw std::runtime_error("Vulkan can't report the dmabuf's modifier");
    }
    layout_.fourcc = format.fourcc;
    layout_.modifier = chosen.drmFormatModifier;
    const auto modifier = std::ranges::find(format.modifiers, layout_.modifier);
    if (modifier == format.modifiers.end()) {
      throw std::runtime_error("Vulkan chose a modifier that the ring lacks");
    }
    const auto plane_count = format.plane_counts.at(
      static_cast<std::size_t>(modifier - format.modifiers.begin())
    );

    const auto requirements =
      device->getImageMemoryRequirements(*image_, dispatcher);
    // An exported image takes memory of its own, which the dmabuf names. The
    // image isn't disjoint, so its planes all lie in that memory.
    const auto dedicated = vk::MemoryDedicatedAllocateInfo().setImage(*image_);
    const auto exported =
      vk::ExportMemoryAllocateInfo()
        .setPNext(&dedicated)
        .setHandleTypes(vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT);
    memory_ = device->allocateMemoryUnique(
      vk::MemoryAllocateInfo()
        .setPNext(&exported)
        .setAllocationSize(requirements.size)
        .setMemoryTypeIndex(dma_buf_memory_type(
          requirements.memoryTypeBits, format.memory_types,
          layout_.modifier == drm_format_mod_linear
        )),
      nullptr, dispatcher
    );
    device->bindImageMemory(*image_, *memory_, 0, dispatcher);
    fd_ = OwnedFd{device->getMemoryFdKHR(
      vk::MemoryGetFdInfoKHR().setMemory(*memory_).setHandleType(
        vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT
      ),
      dispatcher
    )};
    static constexpr auto aspects = std::array{
      vk::ImageAspectFlagBits::eMemoryPlane0EXT,
      vk::ImageAspectFlagBits::eMemoryPlane1EXT,
      vk::ImageAspectFlagBits::eMemoryPlane2EXT,
      vk::ImageAspectFlagBits::eMemoryPlane3EXT,
    };
    for (auto index = uint32_t{0}; index < plane_count; ++index) {
      const auto plane = device->getImageSubresourceLayout(
        *image_, vk::ImageSubresource().setAspectMask(aspects.at(index)),
        dispatcher
      );
      layout_.planes.push_back(
        mln_dma_buf_plane{
          .fd = fd_.get(),
          .offset = static_cast<uint32_t>(plane.offset),
          .stride = static_cast<uint32_t>(plane.rowPitch),
        }
      );
    }
    view_ = device->createImageViewUnique(
      vk::ImageViewCreateInfo()
        .setImage(*image_)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(format.format)
        .setComponents(vk::ComponentMapping())
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
      nullptr, dispatcher
    );
  }

  [[nodiscard]] auto image() const -> vk::Image override { return *image_; }
  [[nodiscard]] auto view() const -> vk::ImageView override { return *view_; }
  [[nodiscard]] auto frame_metadata(const RenderFrameMetadata& frame) const
    -> std::any override {
    return dma_buf_frame(layout_, frame);
  }

 private:
  // Declared in reverse of the order they must be destroyed in.
  OwnedFd fd_;
  vk::UniqueDeviceMemory memory_;
  vk::UniqueImage image_;
  vk::UniqueImageView view_;
  DmaBufLayout layout_;
};

}  // namespace

auto configure_dma_buf_session(
  const DmaBufRing& ring, mln_vulkan_device device,
  mln_render_session_object& session
) -> mln_status {
  auto context = std::shared_ptr<const mln_vulkan_context_descriptor>{};
  const auto device_status = texture_ring_device(device, context);
  if (device_status != MLN_STATUS_OK) return device_status;
  if (context == nullptr) {
    const auto shared_status = shared_rendering_device(context);
    if (shared_status != MLN_STATUS_OK) return shared_status;
  }
  auto dispatcher = vulkan_dispatch_loader(*context);
  vulkan_init_instance_dispatch(dispatcher, *context);
  vulkan_init_device_dispatch(
    dispatcher, static_cast<VkDevice>(context->device), *context
  );
  // A device resolves an extension's commands only once it enables it, and
  // the library's devices enable VK_EXT_external_memory_dma_buf, which has
  // none, whenever the GPU offers it.
  if (
    dispatcher.vkGetPhysicalDeviceFormatProperties2 == nullptr ||
    dispatcher.vkGetPhysicalDeviceImageFormatProperties2 == nullptr ||
    dispatcher.vkGetMemoryFdKHR == nullptr ||
    dispatcher.vkGetImageDrmFormatModifierPropertiesEXT == nullptr
  ) {
    set_thread_error(
      "the Vulkan device can't export dmabufs, because it lacks "
      "VK_KHR_external_memory_fd or VK_EXT_image_drm_format_modifier"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto physical_device =
    vk::PhysicalDevice(static_cast<VkPhysicalDevice>(context->physical_device));
  auto format = VulkanDmaBufFormat{};
  auto choice = DmaBufChoice{};
  const auto chosen = choose_dma_buf_format(
    ring.formats,
    [&](const mln_dma_buf_format& candidate) {
      const auto vk_format = dma_buf_vk_format(candidate.fourcc);
      const auto modifiers =
        device_modifiers(physical_device, dispatcher, vk_format);
      return std::ranges::any_of(
               modifiers,
               [&candidate](const auto& modifier) {
                 return modifier.drmFormatModifier == candidate.modifier;
               }
             ) &&
             exports_dma_buf(
               physical_device, dispatcher, vk_format, candidate.modifier
             );
    },
    choice
  );
  if (!chosen) {
    set_thread_error(
      "the Vulkan device can export a dmabuf of none of the formats"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  session.accepts_queue_lock = true;
  format.fourcc = choice.fourcc;
  format.format = dma_buf_vk_format(choice.fourcc);
  format.modifiers = std::move(choice.modifiers);
  const auto supported =
    device_modifiers(physical_device, dispatcher, format.format);
  for (const auto modifier : format.modifiers) {
    format.plane_counts.push_back(
      std::ranges::find_if(supported, [modifier](const auto& properties) {
        return properties.drmFormatModifier == modifier;
      })->drmFormatModifierPlaneCount
    );
  }
  const auto memory = physical_device.getMemoryProperties(dispatcher);
  for (auto index = uint32_t{0}; index < memory.memoryTypeCount; ++index) {
    format.memory_types.push_back(memory.memoryTypes.at(index).propertyFlags);
  }
  // The buffers' readers work outside this device, so a frame hands over as a
  // sync file or as completed work.
  session.probe_sync_kinds = [context] {
    auto support = vulkan_sync_support(*context);
    support.producer &= MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
    support.consumer &= MLN_GPU_SYNC_KIND_FLAG_SYNC_FD;
    return support;
  };
  session.initialize_backend =
    [context, depth = ring.buffer_count,
     format = std::move(format)](mln_render_session_object& live) {
      // Each frame leaves its image to readers outside Vulkan, for which the
      // foreign queue family stands.
      auto exported = VulkanExportedRing{
        .depth = depth,
        .format = static_cast<VkFormat>(format.format),
        .final_layout = VK_IMAGE_LAYOUT_GENERAL,
        .owner = MLN_VULKAN_IMAGE_OWNER_FOREIGN,
        .allocate = [format](
                      const vk::UniqueDevice& device,
                      const mln::vulkan::DispatchLoaderDynamic& dispatcher,
                      vk::Extent2D extent
                    ) -> std::unique_ptr<VulkanSlotImage> {
          return std::make_unique<DmaBufImage>(
            device, dispatcher, extent, format
          );
        },
      };
      auto backend = std::make_unique<VulkanTextureSessionBackend>(
        *context, std::move(exported),
        mln::Size{live.physical_width, live.physical_height}, live.queue_lock
      );
      backend->set_producer_sync_kind(live.producer_sync_kind);
      backend->retain(context);
      live.texture.backend = std::move(backend);
      return MLN_STATUS_OK;
    };
  return MLN_STATUS_OK;
}
#endif

#if defined(_WIN32)
namespace {

constexpr auto dxgi_usage =
  vk::ImageUsageFlags{vk::ImageUsageFlagBits::eColorAttachment} |
  vk::ImageUsageFlagBits::eSampled;

// Whether the device imports a D3D11 texture as an image that it renders.
auto imports_d3d11_textures(
  vk::PhysicalDevice physical_device,
  const mln::vulkan::DispatchLoaderDynamic& dispatcher
) -> bool {
  const auto external_info =
    vk::PhysicalDeviceExternalImageFormatInfo().setHandleType(
      vk::ExternalMemoryHandleTypeFlagBits::eD3D11Texture
    );
  const auto info = vk::PhysicalDeviceImageFormatInfo2()
                      .setPNext(&external_info)
                      .setFormat(vk::Format::eB8G8R8A8Unorm)
                      .setType(vk::ImageType::e2D)
                      .setTiling(vk::ImageTiling::eOptimal)
                      .setUsage(dxgi_usage);
  auto external = vk::ExternalImageFormatProperties{};
  auto properties = vk::ImageFormatProperties2{};
  properties.pNext = &external;
  return physical_device.getImageFormatProperties2(
           &info, &properties, dispatcher
         ) == vk::Result::eSuccess &&
         (external.externalMemoryProperties.externalMemoryFeatures &
          vk::ExternalMemoryFeatureFlagBits::eImportable);
}

// A slot's image over the memory of a D3D11 texture that the ring allocated.
class DxgiTextureImage final : public VulkanSlotImage {
 public:
  DxgiTextureImage(
    const vk::UniqueDevice& device,
    const mln::vulkan::DispatchLoaderDynamic& dispatcher, vk::Extent2D extent,
    std::shared_ptr<const DxgiTextureDevice> d3d11,
    const VkPhysicalDeviceMemoryProperties& memory
  )
      : texture_(std::move(d3d11), extent.width, extent.height) {
    const auto external = vk::ExternalMemoryImageCreateInfo().setHandleTypes(
      vk::ExternalMemoryHandleTypeFlagBits::eD3D11Texture
    );
    image_ = device->createImageUnique(
      vk::ImageCreateInfo()
        .setPNext(&external)
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setExtent({extent.width, extent.height, 1})
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(dxgi_usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined),
      nullptr, dispatcher
    );
    texture_.bind(
      device.get(), dispatcher.vkGetDeviceProcAddr, memory, *image_
    );
    view_ = device->createImageViewUnique(
      vk::ImageViewCreateInfo()
        .setImage(*image_)
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setComponents(vk::ComponentMapping())
        .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
      nullptr, dispatcher
    );
  }

  [[nodiscard]] auto image() const -> vk::Image override { return *image_; }
  [[nodiscard]] auto view() const -> vk::ImageView override { return *view_; }
  [[nodiscard]] auto frame_metadata(const RenderFrameMetadata& frame) const
    -> std::any override {
    return mln_dxgi_shared_texture_frame{
      .size = sizeof(mln_dxgi_shared_texture_frame),
      .width = frame.physical_width,
      .height = frame.physical_height,
      .scale_factor = frame.scale_factor,
      .slot = frame.slot,
      .format = dxgi_format_b8g8r8a8_unorm,
      .handle = texture_.handle(),
    };
  }

 private:
  // Declared in reverse of the order they must be destroyed in.
  DxgiSharedTexture texture_;
  vk::UniqueImage image_;
  vk::UniqueImageView view_;
};

}  // namespace

auto configure_dxgi_shared_texture_session(
  const DxgiSharedTextureRing& ring, mln_vulkan_device device,
  mln_render_session_object& session
) -> mln_status {
  auto context = std::shared_ptr<const mln_vulkan_context_descriptor>{};
  const auto device_status = texture_ring_device(device, context);
  if (device_status != MLN_STATUS_OK) return device_status;
  if (context == nullptr) {
    const auto shared_status = shared_rendering_device(context);
    if (shared_status != MLN_STATUS_OK) return shared_status;
  }
  auto dispatcher = vulkan_dispatch_loader(*context);
  vulkan_init_instance_dispatch(dispatcher, *context);
  vulkan_init_device_dispatch(
    dispatcher, static_cast<VkDevice>(context->device), *context
  );
  const auto physical_device =
    vk::PhysicalDevice(static_cast<VkPhysicalDevice>(context->physical_device));
  // A device resolves an extension's commands only once it enables it.
  if (
    dispatcher.vkGetPhysicalDeviceProperties2 == nullptr ||
    dispatcher.vkGetPhysicalDeviceImageFormatProperties2 == nullptr ||
    dispatcher.vkGetDeviceProcAddr(
      static_cast<VkDevice>(context->device),
      "vkGetMemoryWin32HandlePropertiesKHR"
    ) == nullptr
  ) {
    set_thread_error(
      "the Vulkan device can't import D3D11 textures, because it lacks "
      "VK_KHR_external_memory_win32"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  if (!imports_d3d11_textures(physical_device, dispatcher)) {
    set_thread_error("the Vulkan device can't import D3D11 textures");
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto properties = physical_device.getProperties2<
    vk::PhysicalDeviceProperties2, vk::PhysicalDeviceIDProperties>(dispatcher);
  const auto& ids = properties.get<vk::PhysicalDeviceIDProperties>();
  if (ids.deviceLUIDValid != VK_TRUE) {
    set_thread_error(
      "the Vulkan device's GPU has no LUID, so D3D11 can't share its textures"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  auto luid = std::array<uint8_t, VK_LUID_SIZE>{};
  std::ranges::copy(std::span{ids.deviceLUID}, luid.begin());
  auto d3d11 = std::shared_ptr<const DxgiTextureDevice>{};
  const auto d3d11_status = create_dxgi_texture_device(luid, d3d11);
  if (d3d11_status != MLN_STATUS_OK) return d3d11_status;
  const auto memory = static_cast<VkPhysicalDeviceMemoryProperties>(
    physical_device.getMemoryProperties(dispatcher)
  );
  session.accepts_queue_lock = true;
  // The textures' readers work outside this device, so a frame hands over as
  // a D3D12 fence or as completed work.
  session.probe_sync_kinds = [context] {
    auto support = vulkan_sync_support(*context);
    support.producer &= MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE;
    support.consumer &= MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE;
    return support;
  };
  session.initialize_backend = [context, depth = ring.texture_count,
                                d3d11 = std::move(d3d11),
                                memory](mln_render_session_object& live) {
    // Each frame leaves its image to D3D, for which the external queue family
    // stands.
    auto exported = VulkanExportedRing{
      .depth = depth,
      .format = VK_FORMAT_B8G8R8A8_UNORM,
      .final_layout = VK_IMAGE_LAYOUT_GENERAL,
      .owner = MLN_VULKAN_IMAGE_OWNER_EXTERNAL,
      .allocate = [d3d11, memory](
                    const vk::UniqueDevice& device,
                    const mln::vulkan::DispatchLoaderDynamic& dispatcher,
                    vk::Extent2D extent
                  ) -> std::unique_ptr<VulkanSlotImage> {
        return std::make_unique<DxgiTextureImage>(
          device, dispatcher, extent, d3d11, memory
        );
      },
    };
    auto backend = std::make_unique<VulkanTextureSessionBackend>(
      *context, std::move(exported),
      mln::Size{live.physical_width, live.physical_height}, live.queue_lock
    );
    backend->set_producer_sync_kind(live.producer_sync_kind);
    backend->retain(context);
    live.texture.backend = std::move(backend);
    return MLN_STATUS_OK;
  };
  return MLN_STATUS_OK;
}
#endif

}  // namespace mln::core
