#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gfx/renderer_backend.hpp>
#include <mbgl/util/size.hpp>
#include <mbgl/vulkan/renderable_resource.hpp>
#include <mbgl/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/surface.h"
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/vulkan/vulkan_dispatch.hpp"

namespace {

auto validate_metal_descriptor(const mln_metal_surface_descriptor* descriptor)
  -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_surface_descriptor)) {
    mln::core::set_thread_error(
      "mln_metal_surface_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    mln::core::validate_metal_context(descriptor->context, false);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->layer == nullptr) {
    mln::core::set_thread_error("Metal surface layer must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vulkan_descriptor(const mln_vulkan_surface_descriptor* descriptor)
  -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_surface_descriptor)) {
    mln::core::set_thread_error(
      "mln_vulkan_surface_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan surface handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == nullptr) {
    mln::core::set_thread_error("Vulkan surface handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vulkan_handles(const mln_vulkan_surface_descriptor& descriptor)
  -> mln_status {
  auto* const instance = static_cast<VkInstance>(descriptor.context.instance);
  auto* const physical_device =
    static_cast<VkPhysicalDevice>(descriptor.context.physical_device);
  auto* const surface = static_cast<VkSurfaceKHR>(descriptor.surface);

  auto dispatcher = mln::core::vulkan_dispatch_loader(descriptor.context);
  mln::core::vulkan_init_instance_dispatch(dispatcher, descriptor.context);
  if (
    dispatcher.vkEnumeratePhysicalDevices == nullptr ||
    dispatcher.vkGetPhysicalDeviceQueueFamilyProperties == nullptr ||
    dispatcher.vkGetPhysicalDeviceSurfaceSupportKHR == nullptr ||
    dispatcher.vkGetPhysicalDeviceSurfaceFormatsKHR == nullptr ||
    dispatcher.vkGetPhysicalDeviceSurfacePresentModesKHR == nullptr
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
  if (descriptor.context.graphics_queue_family_index >= queue_family_count) {
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
    queue_families.at(descriptor.context.graphics_queue_family_index);
  if (
    (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ||
    queue_family.queueCount == 0
  ) {
    mln::core::set_thread_error(
      "Vulkan graphics_queue_family_index must support graphics"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto present_supported = VkBool32{VK_FALSE};
  result = dispatcher.vkGetPhysicalDeviceSurfaceSupportKHR(
    physical_device, descriptor.context.graphics_queue_family_index, surface,
    &present_supported
  );
  if (result != VK_SUCCESS) {
    mln::core::set_thread_error(
      "failed to query Vulkan surface presentation support"
    );
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (present_supported != VK_TRUE) {
    mln::core::set_thread_error(
      "Vulkan graphics_queue_family_index must support presenting to surface"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto surface_format_count = uint32_t{};
  result = dispatcher.vkGetPhysicalDeviceSurfaceFormatsKHR(
    physical_device, surface, &surface_format_count, nullptr
  );
  if (result != VK_SUCCESS) {
    mln::core::set_thread_error("failed to query Vulkan surface formats");
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (surface_format_count == 0) {
    mln::core::set_thread_error(
      "Vulkan surface must expose at least one format"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto present_mode_count = uint32_t{};
  result = dispatcher.vkGetPhysicalDeviceSurfacePresentModesKHR(
    physical_device, surface, &present_mode_count, nullptr
  );
  if (result != VK_SUCCESS) {
    mln::core::set_thread_error("failed to query Vulkan present modes");
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (present_mode_count == 0) {
    mln::core::set_thread_error(
      "Vulkan surface must expose at least one present mode"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  return MLN_STATUS_OK;
}

class VulkanSurfaceBackend final : public mbgl::vulkan::RendererBackend,
                                   public mbgl::vulkan::Renderable {
 private:
  class VulkanSurfaceRenderableResource final
      : public mbgl::vulkan::SurfaceRenderableResource {
   public:
    VulkanSurfaceRenderableResource(
      VulkanSurfaceBackend& backend_, VkSurfaceKHR surface_
    )
        : SurfaceRenderableResource(backend_), borrowed_surface(surface_) {}
    VulkanSurfaceRenderableResource(const VulkanSurfaceRenderableResource&) =
      delete;
    auto operator=(const VulkanSurfaceRenderableResource&)
      -> VulkanSurfaceRenderableResource& = delete;
    VulkanSurfaceRenderableResource(VulkanSurfaceRenderableResource&&) = delete;
    auto operator=(VulkanSurfaceRenderableResource&&)
      -> VulkanSurfaceRenderableResource& = delete;

    // The VkSurfaceKHR is borrowed from the host, so release the unique wrapper
    // before base destruction reaches the Vulkan surface deleter.
    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~VulkanSurfaceRenderableResource() noexcept override {
      static_cast<void>(surface.release());
    }

    void createPlatformSurface() override {
      if (surface) {
        return;
      }
      surface = vk::UniqueSurfaceKHR(
        borrowed_surface,
        vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderDynamic>(
          backend.getInstance().get(), nullptr, backend.getDispatcher()
        )
      );
    }

    auto getDeviceExtensions() -> std::vector<const char*> override {
      return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    }

    void bind() override {}

    void resize(mbgl::Size size) {
      if (!renderPass) {
        return;
      }
      backend.getDevice()->waitIdle(backend.getDispatcher());
      swapchainFramebuffers.clear();
      renderPass.reset();
      swapchainImageViews.clear();
      swapchainImages.clear();
      acquireSemaphores.clear();
      presentSemaphores.clear();
      init(size.width, size.height);
    }

   private:
    VkSurfaceKHR borrowed_surface = nullptr;
  };

 public:
  VulkanSurfaceBackend(
    const mln_vulkan_surface_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::vulkan::RendererBackend(mbgl::gfx::ContextMode::Unique),
        mbgl::vulkan::Renderable(size, nullptr),
        descriptor_(descriptor) {
    initSharedDevice();
    initAllocator();
    initSwapchain();
    initCommandPool();
  }

  VulkanSurfaceBackend(const VulkanSurfaceBackend&) = delete;
  auto operator=(const VulkanSurfaceBackend&) -> VulkanSurfaceBackend& = delete;
  VulkanSurfaceBackend(VulkanSurfaceBackend&&) = delete;
  auto operator=(VulkanSurfaceBackend&&) -> VulkanSurfaceBackend& = delete;

  ~VulkanSurfaceBackend() override {
    auto guard = mbgl::gfx::BackendScope{
      *this, mbgl::gfx::BackendScope::ScopeType::Implicit
    };
    resource.reset();
    getThreadPool().runRenderJobs(true);
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    if (!resource) {
      resource = std::make_unique<VulkanSurfaceRenderableResource>(
        *this, static_cast<VkSurfaceKHR>(descriptor_.surface)
      );
    }
    return *this;
  }

  void resize(mbgl::Size size) {
    mbgl::vulkan::Renderable::setSize(size);
    if (resource) {
      getResource<VulkanSurfaceRenderableResource>().resize(size);
    }
  }

  void activate() override {}
  void deactivate() override {}

 protected:
  void initInstance() override {
    usingSharedContext = true;
    instance = vk::UniqueInstance(
      static_cast<VkInstance>(descriptor_.context.instance),
      vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>(
        nullptr, dispatcher
      )
    );
  }

  void initDebug() override {}

  void initSurface() override {
    getDefaultRenderable()
      .getResource<VulkanSurfaceRenderableResource>()
      .createPlatformSurface();
  }

  void initDevice() override {
    const auto physical_devices =
      instance->enumeratePhysicalDevices(dispatcher);
    auto* const requested_physical_device =
      static_cast<VkPhysicalDevice>(descriptor_.context.physical_device);
    auto found_physical_device = false;
    for (const auto& candidate : physical_devices) {
      if (
        static_cast<VkPhysicalDevice>(candidate) == requested_physical_device
      ) {
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
      vk::ObjectDestroy<vk::NoParent, vk::DispatchLoaderDynamic>(
        nullptr, dispatcher
      )
    );
    mln::core::vulkan_init_device_dispatch(
      dispatcher, device.get(), descriptor_.context
    );
    graphicsQueueIndex =
      static_cast<int32_t>(descriptor_.context.graphics_queue_family_index);
    presentQueueIndex = graphicsQueueIndex;
    graphicsQueue = static_cast<VkQueue>(descriptor_.context.graphics_queue);
    presentQueue = graphicsQueue;
    physicalDeviceFeatures = physicalDevice.getFeatures(dispatcher);
  }

 private:
  void initSharedDevice() {
    dispatcher = mln::core::vulkan_dispatch_loader(descriptor_.context);

    initFrameCapture();
    initInstance();
    mln::core::vulkan_init_instance_dispatch(dispatcher, descriptor_.context);
    initDebug();
    initSurface();
    initDevice();

    physicalDeviceProperties = physicalDevice.getProperties(dispatcher);
  }

  mln_vulkan_surface_descriptor descriptor_;
};

class VulkanSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  VulkanSurfaceSessionBackend(
    const mln_vulkan_surface_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto renderer_backend() -> mbgl::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.resize(mbgl::Size{physical_width, physical_height});
  }

 private:
  VulkanSurfaceBackend backend_;
};

}  // namespace

namespace mln::core {

auto metal_surface_attach(
  mln_map* map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  set_thread_error("Metal surface sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_surface_attach(
  mln_map* map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_vulkan_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto vulkan_status = validate_vulkan_handles(*descriptor);
  if (vulkan_status != MLN_STATUS_OK) {
    return vulkan_status;
  }

  auto session = std::make_unique<mln_render_session>();
  session->map = map;
  session->owner_thread = map_owner_thread(map);
  set_session_extent(*session, descriptor->extent);
  session->surface.backend = std::make_unique<VulkanSurfaceSessionBackend>(
    *descriptor, mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Surface,
    RenderSessionAttachMessages{
      .null_session = "surface session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

}  // namespace mln::core
