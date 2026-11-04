#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "maplibre_native_c/render_target.h"
#include "render/vulkan_device.hpp"

namespace mln::core {

// Creates a Vulkan instance and a device with one graphics queue, which the
// returned context names. The pointer aliases the device, which lasts while
// any copy of it does. An offscreen device names the GPU by `adapter_index`,
// its index in the instance's list, instead of by ID.
auto create_vulkan_device(
  const VulkanDeviceSelection& selection,
  std::optional<std::uint32_t> adapter_index,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status;

// The context of the device that create_vulkan_device() made with the
// VkDevice `device`, or null for a device that a host made. Holding the result
// keeps the device alive.
auto find_vulkan_device(const void* device)
  -> std::shared_ptr<const mln_vulkan_context_descriptor>;

}  // namespace mln::core
