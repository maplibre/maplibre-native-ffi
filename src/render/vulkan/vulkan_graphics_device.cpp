#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <mln/util/logging.hpp>

#include <vulkan/vulkan_core.h>

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"
#include "maplibre_native_c.h"
#include "render/graphics_device.hpp"
#include "render/render_session_common.hpp"
#include "render/vulkan/vulkan_dispatch.hpp"
#include "render/vulkan/vulkan_texture_session.hpp"

namespace mln::core {
namespace {

constexpr auto portability_enumeration = "VK_KHR_portability_enumeration";
constexpr auto portability_subset = "VK_KHR_portability_subset";

auto has_extension(
  const std::vector<VkExtensionProperties>& extensions, const char* name
) -> bool {
  return std::ranges::any_of(extensions, [name](const auto& extension) {
    return std::strcmp(extension.extensionName, name) == 0;
  });
}

// Lower ranks are preferred when the host names no adapter.
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

struct Candidate {
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  uint32_t queue_family_index = 0;
  VkPhysicalDeviceProperties properties{};
};

class VulkanGraphicsDevice final : public GraphicsDevice {
 public:
  VulkanGraphicsDevice() = default;
  VulkanGraphicsDevice(const VulkanGraphicsDevice&) = delete;
  auto operator=(const VulkanGraphicsDevice&) -> VulkanGraphicsDevice& = delete;
  VulkanGraphicsDevice(VulkanGraphicsDevice&&) = delete;
  auto operator=(VulkanGraphicsDevice&&) -> VulkanGraphicsDevice& = delete;

  // The loader may already have torn itself down in an exit handler, so a
  // device that outlives the start of process exit stays with the process.
  ~VulkanGraphicsDevice() override {
    if (process_exiting()) {
      return;
    }
    if (device_ != VK_NULL_HANDLE && dispatcher_.vkDestroyDevice != nullptr) {
      dispatcher_.vkDestroyDevice(device_, nullptr);
    }
    if (
      instance_ != VK_NULL_HANDLE && dispatcher_.vkDestroyInstance != nullptr
    ) {
      dispatcher_.vkDestroyInstance(instance_, nullptr);
    }
  }

  auto create(const mln_graphics_device_options& options) -> mln_status {
    auto* const get_instance_proc_addr = vulkan_system_get_instance_proc_addr();
    if (get_instance_proc_addr == nullptr) {
      set_thread_error("the Vulkan loader is not available");
      return MLN_STATUS_NATIVE_ERROR;
    }
    dispatcher_.init(get_instance_proc_addr);
    const auto instance_status = create_instance();
    if (instance_status != MLN_STATUS_OK) {
      return instance_status;
    }
    auto candidate = std::optional<Candidate>{};
    const auto select_status = select_physical_device(options, candidate);
    if (select_status != MLN_STATUS_OK) {
      return select_status;
    }
    const auto device_status = create_device(*candidate);
    if (device_status != MLN_STATUS_OK) {
      return device_status;
    }
    mln::Log::Info(
      mln::Event::Render,
      std::string{"graphics device uses Vulkan physical device "} +
        static_cast<const char*>(candidate->properties.deviceName)
    );
    context_ = mln_vulkan_context_descriptor{
      .instance = instance_,
      .physical_device = candidate->physical_device,
      .device = device_,
      .graphics_queue = queue_,
      .graphics_queue_family_index = candidate->queue_family_index,
      // The C ABI carries Vulkan loader callbacks as opaque pointers.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      .get_instance_proc_addr = reinterpret_cast<void*>(get_instance_proc_addr),
      .get_device_proc_addr = nullptr,
    };
    return MLN_STATUS_OK;
  }

  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    const auto descriptor = mln_vulkan_owned_texture_descriptor{
      .size = sizeof(mln_vulkan_owned_texture_descriptor),
      .extent = {},
      .context = context_,
    };
    out_backend = create_vulkan_owned_texture_backend(descriptor, size, 1);
    return MLN_STATUS_OK;
  }

 private:
  auto create_instance() -> mln_status {
    auto count = uint32_t{};
    if (
      dispatcher_.vkEnumerateInstanceExtensionProperties == nullptr ||
      dispatcher_.vkCreateInstance == nullptr ||
      dispatcher_.vkEnumerateInstanceExtensionProperties(
        nullptr, &count, nullptr
      ) != VK_SUCCESS
    ) {
      set_thread_error("the Vulkan loader cannot list instance extensions");
      return MLN_STATUS_NATIVE_ERROR;
    }
    auto available = std::vector<VkExtensionProperties>(count);
    dispatcher_.vkEnumerateInstanceExtensionProperties(
      nullptr, &count, available.data()
    );
    available.resize(count);
    // MoltenVK lists its devices only to an instance that asks for portability
    // drivers.
    const auto portability = has_extension(available, portability_enumeration);
    auto enabled = std::vector<const char*>{};
    if (portability) {
      enabled.push_back(portability_enumeration);
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
      const auto message =
        "vkCreateInstance failed with VkResult " + std::to_string(result);
      set_thread_error(message.c_str());
      return MLN_STATUS_NATIVE_ERROR;
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

  auto select_physical_device(
    const mln_graphics_device_options& options,
    std::optional<Candidate>& out_candidate
  ) -> mln_status {
    auto count = uint32_t{};
    if (
      dispatcher_.vkEnumeratePhysicalDevices(instance_, &count, nullptr) !=
        VK_SUCCESS ||
      count == 0
    ) {
      set_thread_error("the Vulkan instance has no physical device");
      return MLN_STATUS_NATIVE_ERROR;
    }
    auto devices = std::vector<VkPhysicalDevice>(count);
    dispatcher_.vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    devices.resize(count);

    const auto named =
      (options.fields & MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX) != 0U;
    if (named && options.adapter_index >= devices.size()) {
      const auto message = "adapter_index " +
                           std::to_string(options.adapter_index) +
                           " is past the " + std::to_string(devices.size()) +
                           " Vulkan physical devices";
      set_thread_error(message.c_str());
      return MLN_STATUS_NATIVE_ERROR;
    }
    for (auto index = std::size_t{}; index < devices.size(); ++index) {
      if (named && index != options.adapter_index) {
        continue;
      }
      const auto family = graphics_queue_family(devices[index]);
      if (!family) {
        continue;
      }
      auto candidate = Candidate{
        .physical_device = devices[index],
        .queue_family_index = *family,
        .properties = {},
      };
      dispatcher_.vkGetPhysicalDeviceProperties(
        candidate.physical_device, &candidate.properties
      );
      if (
        !out_candidate ||
        device_type_rank(candidate.properties.deviceType) <
          device_type_rank(out_candidate->properties.deviceType)
      ) {
        out_candidate = candidate;
      }
    }
    if (!out_candidate) {
      set_thread_error(
        named ? "the Vulkan physical device at adapter_index has no graphics "
                "queue"
              : "no Vulkan physical device has a graphics queue"
      );
      return MLN_STATUS_NATIVE_ERROR;
    }
    return MLN_STATUS_OK;
  }

  auto create_device(const Candidate& candidate) -> mln_status {
    auto count = uint32_t{};
    dispatcher_.vkEnumerateDeviceExtensionProperties(
      candidate.physical_device, nullptr, &count, nullptr
    );
    auto available = std::vector<VkExtensionProperties>(count);
    dispatcher_.vkEnumerateDeviceExtensionProperties(
      candidate.physical_device, nullptr, &count, available.data()
    );
    available.resize(count);
    // A device that has this extension requires a device that enables it.
    auto enabled = std::vector<const char*>{};
    if (has_extension(available, portability_subset)) {
      enabled.push_back(portability_subset);
    }
    // The renderer reads the supported features of a device it borrows, so the
    // device enables each one that it uses and the adapter supports.
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
      .pNext = nullptr,
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
      const auto message =
        "vkCreateDevice failed with VkResult " + std::to_string(result);
      set_thread_error(message.c_str());
      return MLN_STATUS_NATIVE_ERROR;
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

auto create_backend_graphics_device(
  const mln_graphics_device_options& options,
  std::shared_ptr<GraphicsDevice>& out_device
) -> mln_status {
  auto device = std::make_shared<VulkanGraphicsDevice>();
  const auto status = device->create(options);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  out_device = std::move(device);
  return MLN_STATUS_OK;
}

}  // namespace mln::core
