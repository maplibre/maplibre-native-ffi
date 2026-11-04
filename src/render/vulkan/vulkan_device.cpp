// The dispatch header sets the vulkan.hpp configuration this translation unit
// has to agree on, so it stays the first include.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mln/util/logging.hpp>

#include <vulkan/vulkan_core.h>

#include "render/vulkan/vulkan_device.hpp"

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "maplibre_native_c.h"
#include "render/vulkan/vulkan_dispatch.hpp"
#include "render/vulkan_device.hpp"

namespace mln::core {
namespace {

constexpr auto portability_enumeration = "VK_KHR_portability_enumeration";
constexpr auto portability_subset = "VK_KHR_portability_subset";
constexpr auto surface_extension = "VK_KHR_surface";
constexpr auto swapchain_extension = "VK_KHR_swapchain";
constexpr auto timeline_semaphore_extension = "VK_KHR_timeline_semaphore";

// The surface extensions of this platform's windows. A device that presents
// enables each one that the loader offers.
constexpr auto platform_surface_extensions = std::to_array<const char*>({
#if defined(__ANDROID__)
  "VK_KHR_android_surface",
#elif defined(__OHOS__)
  "VK_OHOS_surface",
#elif defined(_WIN32)
  "VK_KHR_win32_surface",
#elif defined(__APPLE__)
  "VK_EXT_metal_surface",
#else
  "VK_KHR_wayland_surface",
  "VK_KHR_xcb_surface",
  "VK_KHR_xlib_surface",
#endif
});

// A device extension for sharing images and semaphores with other APIs, which
// a device enables when the GPU offers it and the extension it requires.
struct SharingExtension {
  const char* name;
  const char* requires_extension;
};

// Each one builds on extensions that Vulkan 1.1 made core, so a device enables
// them only at that version.
constexpr auto sharing_extensions = std::to_array<SharingExtension>({
  {.name = "VK_KHR_external_memory", .requires_extension = nullptr},
  {.name = "VK_KHR_external_semaphore", .requires_extension = nullptr},
  {.name = "VK_EXT_queue_family_foreign", .requires_extension = nullptr},
#if defined(_WIN32)
  {.name = "VK_KHR_external_memory_win32", .requires_extension = nullptr},
  {.name = "VK_KHR_external_semaphore_win32", .requires_extension = nullptr},
#elif defined(__APPLE__)
  {.name = "VK_EXT_metal_objects", .requires_extension = nullptr},
#else
  {.name = "VK_KHR_external_memory_fd", .requires_extension = nullptr},
  {.name = "VK_KHR_external_semaphore_fd", .requires_extension = nullptr},
  {.name = "VK_EXT_external_memory_dma_buf",
   .requires_extension = "VK_KHR_external_memory_fd"},
#endif
#if defined(__ANDROID__)
  {.name = "VK_ANDROID_external_memory_android_hardware_buffer",
   .requires_extension = "VK_EXT_queue_family_foreign"},
#endif
});

auto has_extension(
  const std::vector<VkExtensionProperties>& extensions, const char* name
) -> bool {
  return std::ranges::any_of(extensions, [name](const auto& extension) {
    return std::strcmp(extension.extensionName, name) == 0;
  });
}

// Lower ranks are preferred when the host names no GPU.
auto device_type_rank(VkPhysicalDeviceType type) -> int {
  switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return 0;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return 1;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      return 2;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return 3;
    default:
      return 4;
  }
}

auto native_error(const std::string& message) -> mln_status {
  set_thread_error(message.c_str());
  return MLN_STATUS_NATIVE_ERROR;
}

struct Candidate {
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  uint32_t queue_family_index = 0;
  VkPhysicalDeviceProperties properties{};
  std::vector<VkExtensionProperties> extensions;
};

class VulkanDevice;

// Every device that create_vulkan_device() made and that still lives, by its
// VkDevice.
struct Registry {
  std::mutex mutex;
  std::unordered_map<const void*, std::weak_ptr<const VulkanDevice>> devices;
};

auto registry() -> Registry& {
  // Leaked, so a device that a session destroys while the process exits never
  // finds the registry destroyed.
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  static auto* instance = new Registry();
  return *instance;
}

class VulkanDevice final {
 public:
  VulkanDevice() = default;
  VulkanDevice(const VulkanDevice&) = delete;
  auto operator=(const VulkanDevice&) -> VulkanDevice& = delete;
  VulkanDevice(VulkanDevice&&) = delete;
  auto operator=(VulkanDevice&&) -> VulkanDevice& = delete;

  // The loader may already have torn itself down in an exit handler, so a
  // device that outlives the start of process exit stays with the process.
  ~VulkanDevice() {
    if (device_ != VK_NULL_HANDLE) {
      auto& state = registry();
      const auto lock = std::scoped_lock{state.mutex};
      const auto found = state.devices.find(device_);
      if (found != state.devices.end() && found->second.expired()) {
        state.devices.erase(found);
      }
    }
    if (process_exiting()) {
      return;
    }
    if (device_ != VK_NULL_HANDLE && dispatcher_.vkDestroyDevice != nullptr) {
      // Vulkan requires the queue's work to finish before the device goes.
      // Nothing else uses the device by now, so the wait needs no queue lock.
      if (dispatcher_.vkDeviceWaitIdle != nullptr) {
        static_cast<void>(dispatcher_.vkDeviceWaitIdle(device_));
      }
      dispatcher_.vkDestroyDevice(device_, nullptr);
    }
    if (
      instance_ != VK_NULL_HANDLE && dispatcher_.vkDestroyInstance != nullptr
    ) {
      dispatcher_.vkDestroyInstance(instance_, nullptr);
    }
  }

  auto create(
    const VulkanDeviceSelection& selection,
    std::optional<std::uint32_t> adapter_index
  ) -> mln_status {
    auto* const get_instance_proc_addr = vulkan_system_get_instance_proc_addr();
    if (get_instance_proc_addr == nullptr) {
      return native_error("the Vulkan loader is not available");
    }
    dispatcher_.init(get_instance_proc_addr);
    const auto instance_status = create_instance(selection.presentation);
    if (instance_status != MLN_STATUS_OK) {
      return instance_status;
    }
    auto candidate = std::optional<Candidate>{};
    const auto select_status =
      select_physical_device(selection, adapter_index, candidate);
    if (select_status != MLN_STATUS_OK) {
      return select_status;
    }
    const auto device_status =
      create_device(*candidate, selection.presentation);
    if (device_status != MLN_STATUS_OK) {
      return device_status;
    }
    mln::Log::Info(
      mln::Event::Render,
      std::string{"Vulkan device uses physical device "} +
        static_cast<const char*>(candidate->properties.deviceName)
    );
    context_ = mln_vulkan_context_descriptor{
      .instance = instance_,
      .physical_device = candidate->physical_device,
      .device = device_,
      .graphics_queue = queue_,
      .graphics_queue_family_index = candidate->queue_family_index,
      // The C ABI carries Vulkan loader callbacks as opaque pointers.
      // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
      .get_instance_proc_addr = reinterpret_cast<void*>(get_instance_proc_addr),
      .get_device_proc_addr =
        reinterpret_cast<void*>(dispatcher_.vkGetDeviceProcAddr),
      // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    };
    return MLN_STATUS_OK;
  }

  [[nodiscard]] auto context() const noexcept
    -> const mln_vulkan_context_descriptor& {
    return context_;
  }

 private:
  [[nodiscard]] auto instance_extensions() const
    -> std::optional<std::vector<VkExtensionProperties>> {
    auto count = uint32_t{};
    if (
      dispatcher_.vkEnumerateInstanceExtensionProperties == nullptr ||
      dispatcher_.vkEnumerateInstanceExtensionProperties(
        nullptr, &count, nullptr
      ) != VK_SUCCESS
    ) {
      return std::nullopt;
    }
    auto available = std::vector<VkExtensionProperties>(count);
    if (
      dispatcher_.vkEnumerateInstanceExtensionProperties(
        nullptr, &count, available.data()
      ) != VK_SUCCESS
    ) {
      return std::nullopt;
    }
    available.resize(count);
    return available;
  }

  auto create_instance(bool presentation) -> mln_status {
    const auto available = instance_extensions();
    if (!available || dispatcher_.vkCreateInstance == nullptr) {
      return native_error("the Vulkan loader cannot list instance extensions");
    }
    auto enabled = std::vector<const char*>{};
    // MoltenVK lists its devices only to an instance that asks for portability
    // drivers.
    const auto portability = has_extension(*available, portability_enumeration);
    if (portability) {
      enabled.push_back(portability_enumeration);
    }
    if (presentation) {
      for (const auto* const name : platform_surface_extensions) {
        if (has_extension(*available, name)) {
          enabled.push_back(name);
        }
      }
      const auto platform_surfaces = enabled.size() - (portability ? 1 : 0);
      if (
        !has_extension(*available, surface_extension) || platform_surfaces == 0
      ) {
        set_thread_error(
          "the Vulkan loader offers no surface extension for this platform's "
          "windows"
        );
        return MLN_STATUS_UNSUPPORTED;
      }
      enabled.push_back(surface_extension);
    }
    const auto application = VkApplicationInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "maplibre-native-ffi",
      .applicationVersion = 0,
      .pEngineName = "maplibre-native",
      .engineVersion = 0,
      .apiVersion = VK_API_VERSION_1_1,
    };
    const auto info = VkInstanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
                           : VkInstanceCreateFlags{},
      .pApplicationInfo = &application,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(enabled.size()),
      .ppEnabledExtensionNames = enabled.data(),
    };
    const auto result =
      dispatcher_.vkCreateInstance(&info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
      instance_ = VK_NULL_HANDLE;
      return native_error(
        "vkCreateInstance failed with VkResult " + std::to_string(result)
      );
    }
    dispatcher_.init(vk::Instance(instance_));
    return MLN_STATUS_OK;
  }

  [[nodiscard]] auto graphics_queue_family(
    VkPhysicalDevice physical_device
  ) const -> std::optional<uint32_t> {
    auto count = uint32_t{};
    dispatcher_.vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &count, nullptr
    );
    auto families = std::vector<VkQueueFamilyProperties>(count);
    dispatcher_.vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &count, families.data()
    );
    for (auto index = uint32_t{}; index < count; ++index) {
      if (
        (families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
        families[index].queueCount > 0
      ) {
        return index;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] auto device_extensions(VkPhysicalDevice physical_device) const
    -> std::vector<VkExtensionProperties> {
    auto count = uint32_t{};
    if (
      dispatcher_.vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &count, nullptr
      ) != VK_SUCCESS
    ) {
      return {};
    }
    auto available = std::vector<VkExtensionProperties>(count);
    if (
      dispatcher_.vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &count, available.data()
      ) != VK_SUCCESS
    ) {
      return {};
    }
    available.resize(count);
    return available;
  }

  [[nodiscard]] auto matches_ids(
    VkPhysicalDevice physical_device, const VulkanDeviceSelection& selection
  ) const -> bool {
    if (!selection.uuid && !selection.luid) {
      return true;
    }
    if (dispatcher_.vkGetPhysicalDeviceProperties2 == nullptr) {
      return false;
    }
    auto ids = VkPhysicalDeviceIDProperties{};
    ids.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
    auto properties = VkPhysicalDeviceProperties2{};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &ids;
    dispatcher_.vkGetPhysicalDeviceProperties2(physical_device, &properties);
    if (
      selection.uuid &&
      !std::ranges::equal(*selection.uuid, std::span{ids.deviceUUID})
    ) {
      return false;
    }
    return !selection.luid ||
           (ids.deviceLUIDValid == VK_TRUE &&
            std::ranges::equal(*selection.luid, std::span{ids.deviceLUID}));
  }

  auto select_physical_device(
    const VulkanDeviceSelection& selection,
    std::optional<std::uint32_t> adapter_index,
    std::optional<Candidate>& out_candidate
  ) -> mln_status {
    auto count = uint32_t{};
    if (
      dispatcher_.vkEnumeratePhysicalDevices(instance_, &count, nullptr) !=
        VK_SUCCESS ||
      count == 0
    ) {
      return native_error("the Vulkan instance has no physical device");
    }
    auto devices = std::vector<VkPhysicalDevice>(count);
    dispatcher_.vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    devices.resize(count);

    if (adapter_index && *adapter_index >= devices.size()) {
      return native_error(
        "adapter_index " + std::to_string(*adapter_index) + " is past the " +
        std::to_string(devices.size()) + " Vulkan physical devices"
      );
    }
    auto matched = false;
    auto graphics = false;
    for (auto index = std::size_t{}; index < devices.size(); ++index) {
      if (adapter_index && index != *adapter_index) {
        continue;
      }
      if (!matches_ids(devices[index], selection)) {
        continue;
      }
      matched = true;
      const auto family = graphics_queue_family(devices[index]);
      if (!family) {
        continue;
      }
      graphics = true;
      auto candidate = Candidate{
        .physical_device = devices[index],
        .queue_family_index = *family,
        .properties = {},
        .extensions = device_extensions(devices[index]),
      };
      if (
        selection.presentation &&
        !has_extension(candidate.extensions, swapchain_extension)
      ) {
        continue;
      }
      dispatcher_.vkGetPhysicalDeviceProperties(
        candidate.physical_device, &candidate.properties
      );
      if (
        !out_candidate ||
        device_type_rank(candidate.properties.deviceType) <
          device_type_rank(out_candidate->properties.deviceType)
      ) {
        out_candidate = std::move(candidate);
      }
    }
    if (out_candidate) {
      return MLN_STATUS_OK;
    }
    if (!matched) {
      set_thread_error(
        "no Vulkan physical device matches device_uuid and device_luid"
      );
      return MLN_STATUS_NOT_FOUND;
    }
    if (graphics) {
      set_thread_error(
        "no Vulkan physical device with a graphics queue offers "
        "VK_KHR_swapchain"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
    return native_error(
      adapter_index
        ? "the Vulkan physical device at adapter_index has no graphics queue"
        : "no Vulkan physical device has a graphics queue"
    );
  }

  [[nodiscard]] auto supports_timeline(const Candidate& candidate) const
    -> bool {
    if (
      dispatcher_.vkGetPhysicalDeviceFeatures2 == nullptr ||
      !has_extension(candidate.extensions, timeline_semaphore_extension)
    ) {
      return false;
    }
    auto timeline = VkPhysicalDeviceTimelineSemaphoreFeatures{};
    timeline.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    auto features = VkPhysicalDeviceFeatures2{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &timeline;
    dispatcher_.vkGetPhysicalDeviceFeatures2(
      candidate.physical_device, &features
    );
    return timeline.timelineSemaphore == VK_TRUE;
  }

  auto create_device(const Candidate& candidate, bool presentation)
    -> mln_status {
    const auto& available = candidate.extensions;
    auto enabled = std::vector<const char*>{};
    // Vulkan requires a device that offers this extension to enable it.
    if (has_extension(available, portability_subset)) {
      enabled.push_back(portability_subset);
    }
    if (presentation) {
      enabled.push_back(swapchain_extension);
    }
    // Sessions find these by resolving their commands, so nothing records
    // them.
    if (candidate.properties.apiVersion >= VK_API_VERSION_1_1) {
      for (const auto& extension : sharing_extensions) {
        if (
          has_extension(available, extension.name) &&
          (extension.requires_extension == nullptr ||
           has_extension(available, extension.requires_extension))
        ) {
          enabled.push_back(extension.name);
        }
      }
    }
    auto timeline = VkPhysicalDeviceTimelineSemaphoreFeatures{};
    timeline.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    const auto uses_timeline =
      candidate.properties.apiVersion >= VK_API_VERSION_1_1 &&
      supports_timeline(candidate);
    if (uses_timeline) {
      enabled.push_back(timeline_semaphore_extension);
      timeline.timelineSemaphore = VK_TRUE;
    }
    // The renderer reads the supported features of a device it borrows, so the
    // device enables each one that it uses and the GPU supports.
    auto supported = VkPhysicalDeviceFeatures{};
    dispatcher_.vkGetPhysicalDeviceFeatures(
      candidate.physical_device, &supported
    );
    auto features = VkPhysicalDeviceFeatures{};
    features.samplerAnisotropy = supported.samplerAnisotropy;
    features.wideLines = supported.wideLines;
    const auto priority = 1.0F;
    const auto queue = VkDeviceQueueCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .queueFamilyIndex = candidate.queue_family_index,
      .queueCount = 1,
      .pQueuePriorities = &priority,
    };
    const auto info = VkDeviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = uses_timeline ? &timeline : nullptr,
      .flags = 0,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(enabled.size()),
      .ppEnabledExtensionNames = enabled.data(),
      .pEnabledFeatures = &features,
    };
    const auto result = dispatcher_.vkCreateDevice(
      candidate.physical_device, &info, nullptr, &device_
    );
    if (result != VK_SUCCESS) {
      device_ = VK_NULL_HANDLE;
      return native_error(
        "vkCreateDevice failed with VkResult " + std::to_string(result)
      );
    }
    dispatcher_.init(vk::Device(device_));
    dispatcher_.vkGetDeviceQueue(
      device_, candidate.queue_family_index, 0, &queue_
    );
    return MLN_STATUS_OK;
  }

  mln::vulkan::DispatchLoaderDynamic dispatcher_;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue queue_ = VK_NULL_HANDLE;
  mln_vulkan_context_descriptor context_{};
};

}  // namespace

auto create_vulkan_device(
  const VulkanDeviceSelection& selection,
  std::optional<std::uint32_t> adapter_index,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status {
  auto device = std::make_shared<VulkanDevice>();
  const auto status = device->create(selection, adapter_index);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  {
    auto& state = registry();
    const auto lock = std::scoped_lock{state.mutex};
    state.devices[device->context().device] = device;
  }
  out_context = std::shared_ptr<const mln_vulkan_context_descriptor>(
    device, &device->context()
  );
  return MLN_STATUS_OK;
}

auto find_vulkan_device(const void* device)
  -> std::shared_ptr<const mln_vulkan_context_descriptor> {
  auto& state = registry();
  const auto lock = std::scoped_lock{state.mutex};
  const auto found = state.devices.find(device);
  if (found == state.devices.end()) {
    return nullptr;
  }
  const auto owner = found->second.lock();
  if (owner == nullptr) {
    return nullptr;
  }
  return {owner, &owner->context()};
}

auto create_backend_vulkan_device(
  const VulkanDeviceSelection& selection,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status {
  return create_vulkan_device(selection, std::nullopt, out_context);
}

}  // namespace mln::core
