#pragma once

#include <bit>

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#ifndef VULKAN_HPP_NO_DEFAULT_DISPATCHER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER
#endif

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "maplibre_native_c/render_target.h"

namespace mln::core {

inline auto vulkan_get_instance_proc_addr(
  const mln_vulkan_context_descriptor& context
) noexcept -> PFN_vkGetInstanceProcAddr {
  if (context.get_instance_proc_addr != nullptr) {
    return std::bit_cast<PFN_vkGetInstanceProcAddr>(
      context.get_instance_proc_addr
    );
  }
  return ::vkGetInstanceProcAddr;
}

inline auto vulkan_get_device_proc_addr(
  const mln_vulkan_context_descriptor& context
) noexcept -> PFN_vkGetDeviceProcAddr {
  if (context.get_device_proc_addr != nullptr) {
    return std::bit_cast<PFN_vkGetDeviceProcAddr>(context.get_device_proc_addr);
  }
  return nullptr;
}

inline auto vulkan_dispatch_loader(
  const mln_vulkan_context_descriptor& context
) noexcept -> vk::DispatchLoaderDynamic {
  return {vulkan_get_instance_proc_addr(context)};
}

inline auto vulkan_init_instance_dispatch(
  vk::DispatchLoaderDynamic& dispatcher,
  const mln_vulkan_context_descriptor& context
) noexcept -> void {
  dispatcher.init(vk::Instance(static_cast<VkInstance>(context.instance)));
}

inline auto vulkan_init_device_dispatch(
  vk::DispatchLoaderDynamic& dispatcher, VkDevice device,
  const mln_vulkan_context_descriptor& context
) noexcept -> void {
  if (const auto get_device_proc_addr = vulkan_get_device_proc_addr(context)) {
    dispatcher.vkGetDeviceProcAddr = get_device_proc_addr;
  }
  dispatcher.init(vk::Device(device));
}

}  // namespace mln::core
