#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"

auto mln_offscreen_device_create(
  const mln_offscreen_device_options* options, mln_offscreen_device* out_device,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offscreen_device_create(options, out_device);
  });
}

auto mln_offscreen_device_release(mln_offscreen_device device) noexcept
  -> void {
  mln::core::offscreen_device_release(device);
}

auto mln_map_attach_offscreen(
  mln_map map, const mln_offscreen_descriptor* descriptor,
  const mln_render_session_attach_options* options, mln_offscreen_device device,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offscreen_attach_start(
      map, descriptor, options, device, out_session, completion
    );
  });
}
