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

}  // namespace mln::core
