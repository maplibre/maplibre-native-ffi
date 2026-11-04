#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "maplibre_native_c/render_target.h"
#include "render/vulkan_device.hpp"

namespace mln::core {

// Creates a Vulkan instance, device, and graphics queue. The device lasts while
// any copy of `out_context` does. An offscreen device names its GPU by
// `adapter_index`, an index into the instance's GPUs, instead of by ID.
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

// The device with presentation that the library's Android and OpenHarmony
// sessions share when the host passes none. It lasts while one of them holds
// it, and the next one creates it again.
auto shared_presentation_device(
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status;

}  // namespace mln::core
