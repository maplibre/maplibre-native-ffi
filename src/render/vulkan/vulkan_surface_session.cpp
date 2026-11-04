#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#if defined(__ANDROID__) || defined(__OHOS__) || defined(MLN_FFI_WAYLAND) || \
  defined(_WIN32)
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#endif

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/gfx/renderer_backend.hpp>
#include <mln/util/size.hpp>
#include <mln/vulkan/context.hpp>
#include <mln/vulkan/renderable_resource.hpp>
#include <mln/vulkan/renderer_backend.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#if defined(MLN_FFI_WAYLAND)
// The Wayland surface extension names libwayland-client's types without its
// headers, which the build doesn't have.
struct wl_display;
struct wl_surface;
#include <vulkan/vulkan_wayland.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/surface.h"
#if defined(__ANDROID__)
#include "render/android_surface_session.hpp"
#elif defined(__OHOS__)
#include "render/ohos_surface_session.hpp"
#elif defined(MLN_FFI_WAYLAND)
#include "render/wayland_surface_session.hpp"
#elif defined(_WIN32)
#include "render/win32_window_session.hpp"
#endif
#include "render/render_session_common.hpp"
#include "render/surface_session.hpp"
#include "render/vulkan/vulkan_device.hpp"
#include "render/vulkan/vulkan_dispatch.hpp"
#include "render/vulkan/vulkan_handle.hpp"
#include "render/vulkan/vulkan_queue_access.hpp"

namespace {

auto validate_vulkan_handles(const mln_vulkan_surface_descriptor& descriptor)
  -> mln_status {
  auto* const instance = static_cast<VkInstance>(descriptor.context.instance);
  auto* const physical_device =
    static_cast<VkPhysicalDevice>(descriptor.context.physical_device);
  const auto surface =
    mln::core::vulkan_handle_from_abi<VkSurfaceKHR>(descriptor.surface);

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

VKAPI_ATTR void VKAPI_CALL
keep_borrowed_surface(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks*) {}

// Destroys nothing, so a borrowed VkSurfaceKHR outlives every reset of mbgl's
// wrapper, such as the one that mbgl makes to recreate a lost surface.
auto borrowed_surface_dispatcher()
  -> const mln::vulkan::DispatchLoaderDynamic& {
  static const auto dispatcher = [] {
    auto loader = mln::vulkan::DispatchLoaderDynamic{};
    loader.vkDestroySurfaceKHR = keep_borrowed_surface;
    return loader;
  }();
  return dispatcher;
}

// VulkanQueueAccess comes first so that it outlives mbgl's teardown.
class VulkanSurfaceBackend final : private mln::core::VulkanQueueAccess,
                                   public mln::vulkan::RendererBackend,
                                   public mln::vulkan::Renderable {
 private:
  class VulkanSurfaceRenderableResource final
      : public mln::vulkan::SurfaceRenderableResource {
   public:
    VulkanSurfaceRenderableResource(
      VulkanSurfaceBackend& backend_, VkSurfaceKHR surface_,
      vk::PresentModeKHR present_mode
    )
        : SurfaceRenderableResource(backend_, present_mode),
          borrowed_surface(surface_) {}
    VulkanSurfaceRenderableResource(const VulkanSurfaceRenderableResource&) =
      delete;
    auto operator=(const VulkanSurfaceRenderableResource&)
      -> VulkanSurfaceRenderableResource& = delete;
    VulkanSurfaceRenderableResource(VulkanSurfaceRenderableResource&&) = delete;
    auto operator=(VulkanSurfaceRenderableResource&&)
      -> VulkanSurfaceRenderableResource& = delete;

    ~VulkanSurfaceRenderableResource() noexcept override = default;

    // The VkSurfaceKHR belongs to the host or to the window session, so the
    // wrapper never destroys it.
    void createPlatformSurface() override {
      if (surface) {
        return;
      }
      surface = vk::UniqueSurfaceKHR(
        vk::SurfaceKHR(borrowed_surface),
        mln::vulkan::ObjectDestroy<vk::Instance>(
          backend.getInstance().get(), nullptr, borrowed_surface_dispatcher()
        )
      );
    }

    auto getDeviceExtensions() -> std::vector<const char*> override {
      return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    }

#if defined(__ANDROID__)
    // Android's Vulkan loader only asks an app-owned BufferQueue to wait for a
    // free buffer when the acquire timeout is finite. With an unbounded
    // acquire, starvation surfaces as VK_ERROR_SURFACE_LOST_KHR instead.
    // https://github.com/maplibre/maplibre-compose/issues/1370
    auto getAcquireTimeout() const -> uint64_t override { return 16'000'000; }
#endif

    void bind() override {}

    void swap() override {
      if (!mln::core::discard_renderable_present) {
        SurfaceRenderableResource::swap();
        return;
      }

      auto& context = backend.getContext<mln::vulkan::Context>();
      const auto& dispatcher = backend.getDispatcher();
      auto& command_buffer = context.getCommandBuffer();
      command_buffer->end(dispatcher);

      const vk::PipelineStageFlags wait_stage[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
      };
      const auto submit_info = vk::SubmitInfo()
                                 .setCommandBuffers(command_buffer.get())
                                 .setWaitSemaphores(getAcquireSemaphore())
                                 .setWaitDstStageMask(wait_stage);
      backend.getGraphicsQueue().submit(submit_info, nullptr, dispatcher);
      backend.endFrameCapture();
      recreateSwapchain();
    }

    // Rebuilds the swapchain and everything sized with it, keeping the render
    // pass. mbgl keys its pipeline cache on the render pass handle, so
    // destroying the pass strands every cached pipeline and lets a recycled
    // handle hit a pipeline built for a different pass. Attachment formats and
    // layouts do not change across a resize; if the surface did report a new
    // format, setColorFormat() drops the pass and initRenderPass() rebuilds it
    // before the framebuffers are created.
    void resize(mln::Size size) {
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
      // mbgl reports no transform support without a surface, so a released
      // session compares against what it recorded when it let go.
      const auto compiled_transform =
        surface ? hasSurfaceTransformSupport() : released_transform_support;
      if (candidate_transform != compiled_transform) {
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
    void set_surface(VkSurfaceKHR surface_, mln::Size size) {
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

      surface.reset();
      borrowed_surface = surface_;
      createPlatformSurface();

      init(size.width, size.height);
    }

    // Stops presenting, so the host may destroy its surface. The render pass
    // stays for the next surface, which set_surface() takes from here.
    void release_surface() {
      backend.getDevice()->waitIdle(backend.getDispatcher());
      swapchainFramebuffers.clear();
      swapchainImageViews.clear();
      swapchainImages.clear();
      acquireSemaphores.clear();
      presentSemaphores.clear();
      readTexture.reset();
      released_transform_support = hasSurfaceTransformSupport();
      // The swapchain goes before the surface it was created from.
      swapchain.reset();
      surface.reset();
      borrowed_surface = VK_NULL_HANDLE;
    }

   private:
    VkSurfaceKHR borrowed_surface = VK_NULL_HANDLE;
    // Whether the shaders were compiled for a pre-rotated surface, recorded by
    // release_surface().
    bool released_transform_support = false;
  };

 public:
  VulkanSurfaceBackend(
    const mln_vulkan_surface_descriptor& descriptor, mln::Size size,
    std::shared_ptr<const mln::core::QueueLock> queue_lock,
    vk::PresentModeKHR present_mode
  )
      : mln::core::VulkanQueueAccess(std::move(queue_lock)),
        mln::vulkan::RendererBackend(mln::gfx::ContextMode::Unique),
        mln::vulkan::Renderable(size, nullptr),
        descriptor_(descriptor),
        present_mode_(present_mode) {
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
    auto guard = mln::gfx::BackendScope{*this};
    resource.reset();
    getThreadPool().runRenderJobs(true);
  }

  using mln::core::VulkanQueueAccess::drain_for_teardown;
  using mln::core::VulkanQueueAccess::release_queue_access;

  auto getDefaultRenderable() -> mln::gfx::Renderable& override {
    if (!resource) {
      resource = std::make_unique<VulkanSurfaceRenderableResource>(
        *this,
        mln::core::vulkan_handle_from_abi<VkSurfaceKHR>(descriptor_.surface),
        present_mode_
      );
    }
    return *this;
  }

  void resize(mln::Size size) {
    mln::vulkan::Renderable::setSize(size);
    if (resource) {
      getResource<VulkanSurfaceRenderableResource>().resize(size);
      adopt_swapchain_extent();
    }
  }

  [[nodiscard]] auto matches_surface(
    mln_vulkan_non_dispatchable_handle surface
  ) const -> bool {
    // Nothing has been built yet, so there is nothing to be incompatible with.
    if (!resource) {
      return true;
    }
    return getResource<VulkanSurfaceRenderableResource>().matches_surface(
      mln::core::vulkan_handle_from_abi<VkSurfaceKHR>(surface)
    );
  }

  void set_surface(mln_vulkan_non_dispatchable_handle surface, mln::Size size) {
    // Nothing has been built yet, so the lazy path already takes the new
    // surface: the wrapper is created against descriptor_ on first use.
    if (!resource) {
      descriptor_.surface = surface;
      mln::vulkan::Renderable::setSize(size);
      return;
    }
    // The resource first, then this backend's own view of the target. Building
    // a swapchain can throw, and recording the surface before that would leave
    // the backend naming a target it does not have.
    getResource<VulkanSurfaceRenderableResource>().set_surface(
      mln::core::vulkan_handle_from_abi<VkSurfaceKHR>(surface), size
    );
    descriptor_.surface = surface;
    mln::vulkan::Renderable::setSize(size);
    adopt_swapchain_extent();
  }

  void release_surface() {
    getResource<VulkanSurfaceRenderableResource>().release_surface();
    descriptor_.surface = MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  }

  // The attach descriptor with `surface` in place of its own, to check the new
  // surface against the context the session renders with.
  [[nodiscard]] auto descriptor_for(
    mln_vulkan_non_dispatchable_handle surface
  ) const -> mln_vulkan_surface_descriptor {
    auto descriptor = descriptor_;
    descriptor.surface = surface;
    return descriptor;
  }

  void activate() override {}
  void deactivate() override {}

 private:
  // Takes the extent the swapchain actually got. A surface may report a fixed
  // currentExtent, clamp what was asked for, or swap width and height for a
  // pre-rotated display, and mln::Renderer reads its viewport and scissor from
  // the renderable's size.
  void adopt_swapchain_extent() {
    const auto& renderable_resource =
      getResource<VulkanSurfaceRenderableResource>();
    const auto& extent = renderable_resource.getExtent();
    mln::vulkan::Renderable::setSize({extent.width, extent.height});
  }

 public:
 protected:
  void initInstance() override {
    usingSharedContext = true;
    instance = vk::UniqueInstance(
      static_cast<VkInstance>(descriptor_.context.instance),
      mln::vulkan::ObjectDestroy<vk::detail::NoParent>(nullptr, dispatcher)
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
      mln::vulkan::ObjectDestroy<vk::detail::NoParent>(nullptr, dispatcher)
    );
    mln::core::vulkan_init_device_dispatch(
      dispatcher, device.get(), descriptor_.context
    );
    graphicsQueueIndex =
      static_cast<int32_t>(descriptor_.context.graphics_queue_family_index);
    presentQueueIndex = graphicsQueueIndex;
    graphicsQueue = static_cast<VkQueue>(descriptor_.context.graphics_queue);
    presentQueue = graphicsQueue;
    install_queue_access(
      dispatcher, device.get(), static_cast<VkQueue>(graphicsQueue)
    );
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
  vk::PresentModeKHR present_mode_;
};

class VulkanSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  VulkanSurfaceSessionBackend(
    const mln_vulkan_surface_descriptor& descriptor, mln::Size size,
    std::shared_ptr<const mln::core::QueueLock> queue_lock,
    vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo
  )
      : backend_(descriptor, size, std::move(queue_lock), present_mode) {}

  auto renderer_backend() -> mln::gfx::RendererBackend& override {
    return backend_;
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
    backend_.resize(mln::Size{physical_width, physical_height});
  }

  [[nodiscard]] auto allows_off_thread_teardown() const noexcept
    -> bool override {
    return true;
  }

  auto prepare_off_thread_teardown() noexcept -> bool override {
    return backend_.drain_for_teardown();
  }

  void quarantine() noexcept override { backend_.release_queue_access(); }

  auto set_vulkan_target(
    const mln_vulkan_surface_replacement& replacement, mln::Size physical_size
  ) -> mln_status override {
    const auto handles_status =
      validate_vulkan_handles(backend_.descriptor_for(replacement.surface));
    if (handles_status != MLN_STATUS_OK) {
      return handles_status;
    }
    auto matches = false;
    try {
      matches = backend_.matches_surface(replacement.surface);
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
    backend_.set_surface(replacement.surface, physical_size);
    return MLN_STATUS_OK;
  }

  auto release_target() -> mln_status override {
    backend_.release_surface();
    return MLN_STATUS_OK;
  }

 private:
  VulkanSurfaceBackend backend_;
};

#if defined(__ANDROID__) || defined(__OHOS__) || defined(MLN_FFI_WAYLAND) || \
  defined(_WIN32)
#if defined(__ANDROID__)
using PlatformWindow = mln::core::AndroidWindow;
#elif defined(__OHOS__)
using PlatformWindow = mln::core::OhosWindow;
#elif defined(MLN_FFI_WAYLAND)
using PlatformWindow = mln::core::WaylandWindow;
#else
using PlatformWindow = mln::core::Win32Window;
#endif
#if defined(MLN_FFI_WAYLAND)
// The connection that each of the session's Wayland surfaces belongs to.
using PlatformDisplay = wl_display*;
#else
// An Android, OpenHarmony, or Win32 window stands on its own.
using PlatformDisplay = std::nullptr_t;
#endif

// A VkSurfaceKHR that the library made for a platform window.
class OwnedVulkanSurface final {
 public:
  OwnedVulkanSurface() = default;
  OwnedVulkanSurface(
    VkInstance instance, VkSurfaceKHR surface, PFN_vkDestroySurfaceKHR destroy
  )
      : instance_(instance), surface_(surface), destroy_(destroy) {}
  OwnedVulkanSurface(const OwnedVulkanSurface&) = delete;
  auto operator=(const OwnedVulkanSurface&) -> OwnedVulkanSurface& = delete;
  OwnedVulkanSurface(OwnedVulkanSurface&& other) noexcept
      : instance_(other.instance_),
        surface_(std::exchange(other.surface_, VK_NULL_HANDLE)),
        destroy_(other.destroy_) {}
  auto operator=(OwnedVulkanSurface&& other) noexcept -> OwnedVulkanSurface& {
    if (this != &other) {
      reset();
      instance_ = other.instance_;
      surface_ = std::exchange(other.surface_, VK_NULL_HANDLE);
      destroy_ = other.destroy_;
    }
    return *this;
  }
  ~OwnedVulkanSurface() { reset(); }

  void reset() noexcept {
    if (surface_ != VK_NULL_HANDLE) destroy_(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }

  [[nodiscard]] auto handle() const noexcept
    -> mln_vulkan_non_dispatchable_handle {
    return mln::core::vulkan_handle_to_abi(surface_);
  }

  [[nodiscard]] auto get() const noexcept -> VkSurfaceKHR { return surface_; }

 private:
  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  PFN_vkDestroySurfaceKHR destroy_ = nullptr;
};

auto create_window_vulkan_surface(
  const mln_vulkan_context_descriptor& context, PlatformDisplay display,
  PlatformWindow::element_type* window, OwnedVulkanSurface& out_surface
) -> mln_status {
  auto dispatcher = mln::core::vulkan_dispatch_loader(context);
  mln::core::vulkan_init_instance_dispatch(dispatcher, context);
  auto* const instance = static_cast<VkInstance>(context.instance);
#if defined(__ANDROID__)
  static_cast<void>(display);
  const auto create = dispatcher.vkCreateAndroidSurfaceKHR;
  constexpr auto create_name = "vkCreateAndroidSurfaceKHR";
  const auto info = VkAndroidSurfaceCreateInfoKHR{
    .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .window = window,
  };
#elif defined(__OHOS__)
  static_cast<void>(display);
  const auto create = dispatcher.vkCreateSurfaceOHOS;
  constexpr auto create_name = "vkCreateSurfaceOHOS";
  const auto info = VkSurfaceCreateInfoOHOS{
    .sType = VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_OHOS,
    .pNext = nullptr,
    .flags = 0,
    .window = window,
  };
#elif defined(MLN_FFI_WAYLAND)
  // Vulkan-Hpp's dispatcher has this entry only with the Wayland headers.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto create = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
    dispatcher.vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR")
  );
  constexpr auto create_name = "vkCreateWaylandSurfaceKHR";
  const auto info = VkWaylandSurfaceCreateInfoKHR{
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .display = display,
    .surface = window,
  };
#else
  static_cast<void>(display);
  // Vulkan-Hpp's dispatcher has this entry only with the Win32 platform macro.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto create = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
    dispatcher.vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR")
  );
  constexpr auto create_name = "vkCreateWin32SurfaceKHR";
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  auto* module =
    reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(window, GWLP_HINSTANCE));
  if (module == nullptr) module = GetModuleHandleW(nullptr);
  const auto info = VkWin32SurfaceCreateInfoKHR{
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .pNext = nullptr,
    .flags = 0,
    .hinstance = module,
    .hwnd = window,
  };
#endif
  if (create == nullptr || dispatcher.vkDestroySurfaceKHR == nullptr) {
    mln::core::set_thread_error(
      "the Vulkan instance can't present to this platform's windows"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
  auto surface = VkSurfaceKHR{VK_NULL_HANDLE};
  const auto result = create(instance, &info, nullptr, &surface);
  if (result != VK_SUCCESS) {
    const auto message = std::string{create_name} + " failed with VkResult " +
                         std::to_string(result);
    mln::core::set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_surface =
    OwnedVulkanSurface{instance, surface, dispatcher.vkDestroySurfaceKHR};
  return MLN_STATUS_OK;
}

#if defined(MLN_FFI_WAYLAND) || defined(_WIN32)
// The host paces frames itself, so presents don't wait for the display when
// the surface allows it. A Wayland compositor also answers a hidden surface's
// frame callbacks only once it shows again, and FIFO presentation waits for
// them, which would hold a detach queued behind a present.
auto window_present_mode(
  const mln_vulkan_context_descriptor& context, VkSurfaceKHR surface
) -> vk::PresentModeKHR {
  auto dispatcher = mln::core::vulkan_dispatch_loader(context);
  mln::core::vulkan_init_instance_dispatch(dispatcher, context);
  auto* const physical_device =
    static_cast<VkPhysicalDevice>(context.physical_device);
  auto count = uint32_t{};
  auto modes = std::vector<VkPresentModeKHR>{};
  if (
    dispatcher.vkGetPhysicalDeviceSurfacePresentModesKHR(
      physical_device, surface, &count, nullptr
    ) == VK_SUCCESS
  ) {
    modes.resize(count);
    if (
      dispatcher.vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device, surface, &count, modes.data()
      ) != VK_SUCCESS
    ) {
      count = 0;
    }
    modes.resize(count);
  }
  for (const auto mode :
       {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR}) {
    if (std::ranges::find(modes, mode) != modes.end()) {
      return static_cast<vk::PresentModeKHR>(mode);
    }
  }
  return vk::PresentModeKHR::eFifo;
}
#else
// Android and OpenHarmony present in FIFO order.
auto window_present_mode(const mln_vulkan_context_descriptor&, VkSurfaceKHR)
  -> vk::PresentModeKHR {
  return vk::PresentModeKHR::eFifo;
}
#endif

// Presents to an Android, OpenHarmony, Wayland, or Win32 window through a
// VkSurfaceKHR that it creates for each window and destroys once the swapchain
// no longer uses it.
class WindowVulkanSurfaceSessionBackend final
    : public mln::core::SurfaceSessionBackend {
 public:
  WindowVulkanSurfaceSessionBackend(
    PlatformDisplay display, PlatformWindow window, OwnedVulkanSurface surface,
    const mln_vulkan_surface_descriptor& descriptor, mln::Size size,
    std::shared_ptr<const mln::core::QueueLock> queue_lock,
    vk::PresentModeKHR present_mode
  )
      : context_(descriptor.context),
        display_(display),
        window_(std::move(window)),
        surface_(std::move(surface)),
        session_(descriptor, size, std::move(queue_lock), present_mode) {}

  auto renderer_backend() -> mln::gfx::RendererBackend& override {
    return session_.renderer_backend();
  }

  void resize(uint32_t physical_width, uint32_t physical_height) override {
#if defined(_WIN32)
    // A minimized window's surface has no extent, which no swapchain can have,
    // so the swapchain takes the size once the window is restored.
    if (!mln::core::win32_window_ready(window_.get())) {
      pending_size_ = mln::Size{physical_width, physical_height};
      return;
    }
    pending_size_.reset();
#endif
    session_.resize(physical_width, physical_height);
  }

  auto prepare_frame(bool& out_ready) -> mln_status override {
#if defined(_WIN32)
    if (!mln::core::win32_window_ready(window_.get())) {
      out_ready = false;
      return MLN_STATUS_OK;
    }
    if (pending_size_) {
      session_.resize(pending_size_->width, pending_size_->height);
      pending_size_.reset();
    }
#endif
    return session_.prepare_frame(out_ready);
  }

  [[nodiscard]] auto allows_off_thread_teardown() const noexcept
    -> bool override {
    return session_.allows_off_thread_teardown();
  }

  auto prepare_off_thread_teardown() noexcept -> bool override {
    return session_.prepare_off_thread_teardown();
  }

  void quarantine() noexcept override { session_.quarantine(); }

#if defined(__ANDROID__)
  auto set_android_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#elif defined(__OHOS__)
  auto set_ohos_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#elif defined(MLN_FFI_WAYLAND)
  auto set_wayland_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#else
  auto set_win32_target(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status override {
    return set_window(window, physical_size);
  }
#endif

  auto release_target() -> mln_status override {
    const auto status = session_.release_target();
    if (status != MLN_STATUS_OK) return status;
    surface_.reset();
    window_.reset();
    return MLN_STATUS_OK;
  }

 private:
  auto set_window(const PlatformWindow& window, mln::Size physical_size)
    -> mln_status {
    if (window == window_) {
      session_.resize(physical_size.width, physical_size.height);
      return MLN_STATUS_OK;
    }
    auto next = OwnedVulkanSurface{};
    const auto create_status =
      create_window_vulkan_surface(context_, display_, window.get(), next);
    if (create_status != MLN_STATUS_OK) return create_status;
    const auto replacement = mln_vulkan_surface_replacement{
      .size = sizeof(mln_vulkan_surface_replacement),
      .logical_size = {},
      .surface = next.handle(),
    };
    auto status = MLN_STATUS_OK;
    try {
      status = session_.set_vulkan_target(replacement, physical_size);
    } catch (...) {
      // The swapchain may use either surface now, so both last as long as it.
      abandoned_.push_back(std::move(next));
      throw;
    }
    if (status != MLN_STATUS_OK) return status;
    // The swapchain no longer uses the old surface, which goes before its
    // window.
    surface_ = std::move(next);
    window_ = window;
    return MLN_STATUS_OK;
  }

  mln_vulkan_context_descriptor context_;
  PlatformDisplay display_;
  PlatformWindow window_;
  OwnedVulkanSurface surface_;
  // Surfaces that a failed replacement left the swapchain possibly using.
  std::vector<OwnedVulkanSurface> abandoned_;
#if defined(_WIN32)
  // A resize that arrived while the window was minimized.
  std::optional<mln::Size> pending_size_;
#endif
  // Destroyed first, so the swapchain goes before the surfaces and windows.
  VulkanSurfaceSessionBackend session_;
};
#endif

}  // namespace

namespace mln::core {

#if defined(__ANDROID__) || defined(__OHOS__) || defined(MLN_FFI_WAYLAND) || \
  defined(_WIN32)
namespace {

auto create_window_backend(
  PlatformDisplay display, PlatformWindow window,
  std::shared_ptr<const mln_vulkan_context_descriptor> device,
  mln_render_session_object& target
) -> mln_status {
  if (device == nullptr) {
    const auto device_status = mln::core::shared_presentation_device(device);
    if (device_status != MLN_STATUS_OK) return device_status;
  }
  auto surface = OwnedVulkanSurface{};
  const auto surface_status =
    create_window_vulkan_surface(*device, display, window.get(), surface);
  if (surface_status != MLN_STATUS_OK) return surface_status;
  const auto descriptor = mln_vulkan_surface_descriptor{
    .size = sizeof(mln_vulkan_surface_descriptor),
    .logical_size = {.width = target.width, .height = target.height},
    .context = *device,
    .surface = surface.handle(),
  };
  const auto handles_status = validate_vulkan_handles(descriptor);
  if (handles_status != MLN_STATUS_OK) return handles_status;
  const auto present_mode = window_present_mode(*device, surface.get());
  auto backend = std::make_unique<WindowVulkanSurfaceSessionBackend>(
    display, std::move(window), std::move(surface), descriptor,
    mln::Size{target.physical_width, target.physical_height}, target.queue_lock,
    present_mode
  );
  backend->retain(std::move(device));
  target.surface.backend = std::move(backend);
  return MLN_STATUS_OK;
}

}  // namespace

#if defined(__ANDROID__)
auto create_android_surface_backend(
  AndroidSurfaceAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(
    nullptr, std::move(attach.window), std::move(attach.device), target
  );
}
#elif defined(__OHOS__)
auto create_ohos_surface_backend(
  OhosSurfaceAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(
    nullptr, std::move(attach.window), std::move(attach.device), target
  );
}
#elif defined(MLN_FFI_WAYLAND)
auto create_wayland_surface_backend(
  WaylandSurfaceAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(
    attach.display, std::move(attach.window), std::move(attach.device), target
  );
}
#else
auto create_win32_window_backend(
  Win32WindowAttach attach, mln_render_session_object& target
) -> mln_status {
  return create_window_backend(
    nullptr, std::move(attach.window), std::move(attach.device), target
  );
}
#endif
#endif

auto vulkan_surface_attach_start(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
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

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  const auto size_status = size_new_session(*session, descriptor->logical_size);
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  session->target_identity = {descriptor->surface};
  session->accepts_queue_lock = true;
  const auto copied = *descriptor;
  // Taken now, because the host may release a device of the library's as
  // soon as this call returns.
  session->initialize_backend =
    [copied, device = find_vulkan_device(copied.context.device)](
      mln_render_session_object& target
    ) mutable {
      const auto handles_status = validate_vulkan_handles(copied);
      if (handles_status != MLN_STATUS_OK) {
        return handles_status;
      }
      auto backend = std::make_unique<VulkanSurfaceSessionBackend>(
        copied, mln::Size{target.physical_width, target.physical_height},
        target.queue_lock
      );
      if (device != nullptr) backend->retain(std::move(device));
      target.surface.backend = std::move(backend);
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_PRESENTATION,
    .producer_sync_kind = {},
    .consumer_sync_kinds = 0,
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Surface, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_vulkan_surface>()
  );
}

auto vulkan_surface_set_target_start(
  mln_render_session session, const mln_vulkan_surface_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  const auto replacement_status =
    validate_vulkan_surface_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) {
    return replacement_status;
  }
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::Surface, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) {
    return submission_status;
  }
  const auto copied = *replacement;
  return enqueue_surface_retarget(
    session, copied.logical_size, {copied.surface},
    [copied](mln_render_session_object& live, mln::Size physical_size) {
      return live.surface.backend->set_vulkan_target(copied, physical_size);
    },
    std::move(checked_completion),
    valueless_completion<&mln_render_session_set_vulkan_surface_target>()
  );
}

}  // namespace mln::core
