#include <algorithm>
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
        mbgl::vulkan::ObjectDestroy<vk::Instance>(
          backend.getInstance().get(), nullptr, backend.getDispatcher()
        )
      );
    }

    auto getDeviceExtensions() -> std::vector<const char*> override {
      return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    }

    void bind() override {}

    // Rebuilds the swapchain and everything sized with it, keeping the render
    // pass. mbgl keys its pipeline cache on the render pass handle, so
    // destroying the pass strands every cached pipeline and lets a recycled
    // handle hit a pipeline built for a different pass. Attachment formats and
    // layouts do not change across a resize; if the surface did report a new
    // format, setColorFormat() drops the pass and initRenderPass() rebuilds it
    // before the framebuffers are created.
    void resize(mbgl::Size size) {
      if (!renderPass) {
        return;
      }
      backend.getDevice()->waitIdle(backend.getDispatcher());
      swapchainFramebuffers.clear();
      swapchainImageViews.clear();
      swapchainImages.clear();
      acquireSemaphores.clear();
      presentSemaphores.clear();
      init(size.width, size.height);
    }

    // Whether a replacement surface can use the render pass and the shaders
    // this session already compiled. The color format decides the render pass,
    // and mbgl compiles a distinct shader variant for surfaces supporting a
    // pre-rotation transform (USE_SURFACE_TRANSFORM). Both are read before
    // anything is torn down, so a mismatch leaves the session as it was.
    [[nodiscard]] auto matches_surface(VkSurfaceKHR candidate) const -> bool {
      const auto& physical_device = backend.getPhysicalDevice();
      const auto& dispatcher = backend.getDispatcher();
      const auto candidate_surface = vk::SurfaceKHR(candidate);

      const auto candidate_capabilities =
        physical_device.getSurfaceCapabilitiesKHR(
          candidate_surface, dispatcher
        );
      const auto candidate_transform =
        candidate_capabilities.supportedTransforms !=
        vk::SurfaceTransformFlagBitsKHR::eIdentity;
      if (candidate_transform != hasSurfaceTransformSupport()) {
        return false;
      }

      // The same choice initSwapchain() makes, so the comparison is against the
      // format the replacement would actually be given.
      const auto formats =
        physical_device.getSurfaceFormatsKHR(candidate_surface, dispatcher);
      const auto found =
        std::find_if(formats.begin(), formats.end(), [](const auto& format) {
          return (format.format == vk::Format::eB8G8R8A8Unorm ||
                  format.format == vk::Format::eR8G8B8A8Unorm) &&
                 format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
      return found != formats.end() && found->format == colorFormat;
    }

    // Presents through a different host surface from here on. The caller has
    // already established that it matches, so the render pass survives.
    void set_surface(VkSurfaceKHR surface_, mbgl::Size size) {
      backend.getDevice()->waitIdle(backend.getDispatcher());
      swapchainFramebuffers.clear();
      swapchainImageViews.clear();
      swapchainImages.clear();
      acquireSemaphores.clear();
      presentSemaphores.clear();
      readTexture.reset();
      // Before the surface it was created from goes away, and not as an
      // oldSwapchain for the new one: a swapchain may only be recycled into
      // another on the same surface.
      swapchain.reset();

      // The outgoing VkSurfaceKHR belongs to the host, so release the wrapper
      // without running the Vulkan deleter.
      static_cast<void>(surface.release());
      borrowed_surface = surface_;
      createPlatformSurface();

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
    auto guard = mbgl::gfx::BackendScope{*this};
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
      adopt_swapchain_extent();
    }
  }

  // Whether a replacement surface can use what this session already compiled.
  [[nodiscard]] auto matches_surface(
    const mln_vulkan_surface_descriptor& descriptor
  ) const -> bool {
    // Nothing has been built yet, so there is nothing to be incompatible with.
    if (!resource) {
      return true;
    }
    return getResource<VulkanSurfaceRenderableResource>().matches_surface(
      static_cast<VkSurfaceKHR>(descriptor.surface)
    );
  }

  void set_surface(const mln_vulkan_surface_descriptor& descriptor) {
    const auto new_size = mbgl::Size{
      mln::core::physical_dimension(
        descriptor.extent.width, descriptor.extent.scale_factor
      ),
      mln::core::physical_dimension(
        descriptor.extent.height, descriptor.extent.scale_factor
      )
    };
    // Nothing has been built yet, so the lazy path already takes the new
    // surface: the wrapper is created against descriptor_ on first use.
    if (!resource) {
      descriptor_.surface = descriptor.surface;
      mbgl::vulkan::Renderable::setSize(new_size);
      return;
    }
    // The resource first, then this backend's own view of the target. Building
    // a swapchain can throw, and recording the surface before that would leave
    // the backend naming a target it does not have.
    getResource<VulkanSurfaceRenderableResource>().set_surface(
      static_cast<VkSurfaceKHR>(descriptor.surface), new_size
    );
    descriptor_.surface = descriptor.surface;
    mbgl::vulkan::Renderable::setSize(new_size);
    adopt_swapchain_extent();
  }

  [[nodiscard]] auto context_descriptor() const
    -> const mln_vulkan_context_descriptor& {
    return descriptor_.context;
  }

  void activate() override {}
  void deactivate() override {}

 private:
  // Takes the extent the swapchain actually got. A surface may report a fixed
  // currentExtent, clamp what was asked for, or swap width and height for a
  // pre-rotated display, and mbgl::Renderer reads its viewport and scissor from
  // the renderable's size.
  void adopt_swapchain_extent() {
    const auto& renderable_resource =
      getResource<VulkanSurfaceRenderableResource>();
    const auto& extent = renderable_resource.getExtent();
    mbgl::vulkan::Renderable::setSize({extent.width, extent.height});
  }

 public:
 protected:
  void initInstance() override {
    usingSharedContext = true;
    instance = vk::UniqueInstance(
      static_cast<VkInstance>(descriptor_.context.instance),
      mbgl::vulkan::ObjectDestroy<vk::detail::NoParent>(nullptr, dispatcher)
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
      mbgl::vulkan::ObjectDestroy<vk::detail::NoParent>(nullptr, dispatcher)
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

  auto set_vulkan_target(const mln_vulkan_surface_descriptor& descriptor)
    -> mln_status override {
    if (!mln::core::vulkan_context_matches(
          backend_.context_descriptor(), descriptor.context
        )) {
      mln::core::set_thread_error(
        "Vulkan surface target must name the instance, physical device, "
        "device, and graphics queue this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    const auto handles_status = validate_vulkan_handles(descriptor);
    if (handles_status != MLN_STATUS_OK) {
      return handles_status;
    }
    auto matches = false;
    try {
      matches = backend_.matches_surface(descriptor);
    } catch (const std::exception& exception) {
      // Reported rather than thrown: nothing has been touched yet, and an
      // escaping exception would take the session through the mid-swap recovery
      // path and cost it a renderer it could have kept.
      mln::core::set_thread_error(exception);
      return MLN_STATUS_NATIVE_ERROR;
    }
    if (!matches) {
      return mln::core::unsupported_retarget(
        "Vulkan surface target must report the color format and surface "
        "transform support this session compiled its render pass and shaders "
        "for; destroy the session and attach again to change them"
      );
    }
    backend_.set_surface(descriptor);
    return MLN_STATUS_OK;
  }

 private:
  VulkanSurfaceBackend backend_;
};

}  // namespace

namespace mln::core {

auto vulkan_surface_attach_start(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_vulkan_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled surface dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  const auto copied = *descriptor;
  session->initialize_backend = [copied](mln_render_session_object& target) {
    const auto handles_status = validate_vulkan_handles(copied);
    if (handles_status != MLN_STATUS_OK) {
      return handles_status;
    }
    target.surface.backend = std::make_unique<VulkanSurfaceSessionBackend>(
      copied, mbgl::Size{target.physical_width, target.physical_height}
    );
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_PRESENTATION
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Surface, options, capabilities,
    out_session, out_operation
  );
}

auto vulkan_surface_set_target_start(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status {
  const auto descriptor_status = validate_vulkan_surface_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto copied = *descriptor;
  return enqueue_driver_operation(
    session, RENDER_OPERATION_MAINTENANCE,
    [copied](mln_render_session_object& target) {
      const auto handles_status = validate_vulkan_handles(copied);
      if (handles_status != MLN_STATUS_OK) {
        return handles_status;
      }
      return surface_session_set_target(
        target.self, copied.extent, [&copied](mln_render_session_object& live) {
          return live.surface.backend->set_vulkan_target(copied);
        }
      );
    },
    out_operation
  );
}

}  // namespace mln::core
