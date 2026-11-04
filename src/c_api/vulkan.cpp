#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/vulkan_device.hpp"

auto mln_vulkan_device_create(
  const mln_vulkan_device_options* options, mln_vulkan_device* out_device,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::vulkan_device_create(options, out_device);
  });
}

auto mln_vulkan_device_get_context(
  mln_vulkan_device device, mln_vulkan_device_context_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::vulkan_device_get_context(device, out_result);
  });
}

auto mln_vulkan_device_release(mln_vulkan_device device) noexcept -> void {
  mln::core::vulkan_device_release(device);
}
