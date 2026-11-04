#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/surface_session.hpp"

#if defined(MLN_FFI_WAYLAND)
#include "render/wayland_surface_session.hpp"
#endif

auto mln_map_attach_wayland_surface(
  mln_map map, const mln_wayland_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#if !defined(MLN_FFI_WAYLAND)
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_wayland_surface_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Wayland surfaces are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::wayland_surface_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_render_session_set_wayland_surface_target(
  mln_render_session session,
  const mln_wayland_surface_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#if !defined(MLN_FFI_WAYLAND)
    static_cast<void>(session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_wayland_surface_replacement(replacement);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Wayland surfaces are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::wayland_surface_set_target_start(
      session, replacement, completion
    );
#endif
  });
}
