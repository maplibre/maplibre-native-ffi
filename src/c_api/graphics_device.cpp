#define MLN_BUILDING_C

#include "render/graphics_device.hpp"

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"

auto mln_graphics_device_create(
  const mln_graphics_device_options* options, mln_graphics_device* out_device,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::graphics_device_create(options, out_device);
  });
}

auto mln_graphics_device_release(mln_graphics_device device) noexcept -> void {
  mln::core::graphics_device_release(device);
}

auto mln_map_attach_graphics_device_texture(
  mln_map map, mln_graphics_device device,
  const mln_graphics_device_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::graphics_device_texture_attach_start(
      map, device, descriptor, options, out_session, completion
    );
  });
}
