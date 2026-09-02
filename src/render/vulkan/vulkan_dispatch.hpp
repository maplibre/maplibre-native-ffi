#pragma once

#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#endif
#ifndef VULKAN_HPP_NO_DEFAULT_DISPATCHER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define VULKAN_HPP_NO_DEFAULT_DISPATCHER
#endif

#include <mln/vulkan/renderer_backend.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "maplibre_native_c/render_target.h"

namespace mln::core {

// Resolves vkGetInstanceProcAddr from a Vulkan loader mapped on first use, or
// null when the host has no loader. Resolving at runtime keeps build-host
// loader paths out of the shipped library.
auto vulkan_system_get_instance_proc_addr() noexcept
  -> PFN_vkGetInstanceProcAddr;

inline auto vulkan_get_instance_proc_addr(
  const mln_vulkan_context_descriptor& context
) noexcept -> PFN_vkGetInstanceProcAddr {
  if (context.get_instance_proc_addr != nullptr) {
    // The C ABI carries Vulkan loader callbacks as opaque pointers.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      context.get_instance_proc_addr
    );
  }
  return vulkan_system_get_instance_proc_addr();
}

inline auto vulkan_get_device_proc_addr(
  const mln_vulkan_context_descriptor& context
) noexcept -> PFN_vkGetDeviceProcAddr {
  if (context.get_device_proc_addr != nullptr) {
    // The C ABI carries Vulkan loader callbacks as opaque pointers.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      context.get_device_proc_addr
    );
  }
  return nullptr;
}

inline auto vulkan_dispatch_loader(
  const mln_vulkan_context_descriptor& context
) noexcept -> mln::vulkan::DispatchLoaderDynamic {
  const auto get_instance_proc_addr = vulkan_get_instance_proc_addr(context);
  if (get_instance_proc_addr == nullptr) {
    // Every dispatch entry stays null so callers report an unresolved loader
    // instead of dispatching through a null bootstrap pointer.
    return {};
  }
  return {get_instance_proc_addr};
}

inline auto vulkan_init_instance_dispatch(
  mln::vulkan::DispatchLoaderDynamic& dispatcher,
  const mln_vulkan_context_descriptor& context
) noexcept -> void {
  if (dispatcher.vkGetInstanceProcAddr == nullptr) {
    return;
  }
  dispatcher.init(vk::Instance(static_cast<VkInstance>(context.instance)));
}

inline auto vulkan_init_device_dispatch(
  mln::vulkan::DispatchLoaderDynamic& dispatcher, VkDevice device,
  const mln_vulkan_context_descriptor& context
) noexcept -> void {
  if (const auto get_device_proc_addr = vulkan_get_device_proc_addr(context)) {
    dispatcher.vkGetDeviceProcAddr = get_device_proc_addr;
  }
  dispatcher.init(vk::Device(device));
}

}  // namespace mln::core
