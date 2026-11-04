#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/surface_session.hpp"
#include "render/texture_session.hpp"

#ifdef _WIN32
#include "render/dxgi_shared_texture_session.hpp"
#include "render/win32_window_session.hpp"
#endif

auto mln_map_attach_win32_window(
  mln_map map, const mln_win32_window_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef _WIN32
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status = mln::core::validate_win32_window_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Win32 windows are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::win32_window_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_render_session_set_win32_window_target(
  mln_render_session session, const mln_win32_window_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef _WIN32
    static_cast<void>(session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_win32_window_replacement(replacement);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Win32 windows are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::win32_window_set_target_start(
      session, replacement, completion
    );
#endif
  });
}

auto mln_map_attach_dxgi_shared_texture_ring(
  mln_map map, const mln_dxgi_shared_texture_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef _WIN32
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_dxgi_shared_texture_ring_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("DXGI shared textures are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::dxgi_shared_texture_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_acquired_frame_get_dxgi_shared_texture(
  mln_acquired_frame frame, mln_dxgi_shared_texture_frame* out_frame,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::acquired_frame_get_dxgi_shared_texture(frame, out_frame);
  });
}
