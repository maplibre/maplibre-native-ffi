#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>

#include "handles/handle_table.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// A Vulkan device that the library owns, as its handle holds it. Holding
// `context` keeps the device alive.
struct VulkanDeviceObject {
  std::shared_ptr<const mln_vulkan_context_descriptor> context;
  // Whether the device enabled the extensions that present to a window.
  bool presentation = false;
};

template <>
struct HandleTraits<VulkanDeviceObject> {
  static constexpr auto kind = HandleKind::VulkanDevice;
  static constexpr auto leasable = true;
};

// The GPU that a device selects, and whether it can present.
struct VulkanDeviceSelection {
  std::optional<std::array<std::uint8_t, 16>> uuid;
  std::optional<std::array<std::uint8_t, 8>> luid;
  bool presentation = false;
};

// Each build defines this. A Vulkan build creates the device, and any other
// build reports MLN_STATUS_UNSUPPORTED.
auto create_backend_vulkan_device(
  const VulkanDeviceSelection& selection,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status;

auto vulkan_device_create(
  const mln_vulkan_device_options* options, mln_vulkan_device* out_device
) -> mln_status;
auto vulkan_device_get_context(
  mln_vulkan_device device, mln_vulkan_device_context_result* out_result
) -> mln_status;
auto vulkan_device_release(mln_vulkan_device device) -> void;

// The context of the presenting Vulkan device that `device` names, for a
// platform window's session. A null `device` leaves `out_context` null, for the
// shared device. A build without Vulkan takes only a null `device`.
auto window_presentation_device(
  mln_vulkan_device device,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status;

// The context of the Vulkan device that `device` names, for a session that
// renders into textures that the library allocates. A null `device` leaves
// `out_context` null, for the device that window sessions share. A build
// without Vulkan takes only a null `device`.
auto texture_ring_device(
  mln_vulkan_device device,
  std::shared_ptr<const mln_vulkan_context_descriptor>& out_context
) -> mln_status;

}  // namespace mln::core
