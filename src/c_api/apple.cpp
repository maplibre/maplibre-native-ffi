#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/texture_session.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>

#include "platform/apple/frame_pacer.hpp"
#endif

#ifdef MLN_RENDER_BACKEND_METAL
#include "render/metal/metal_texture_session.hpp"
#endif

auto mln_apple_frame_pacer_create(
  const mln_frame_pacer_options* options, mln_frame_pacer* out_pacer,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#if !defined(__APPLE__) || TARGET_OS_VISION
    static_cast<void>(options);
    static_cast<void>(out_pacer);
    mln::core::set_thread_error("Apple frame pacers are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::platform::apple::create_frame_pacer(options, out_pacer);
#endif
  });
}

auto mln_map_attach_io_surface_ring(
  mln_map map, const mln_io_surface_ring_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef MLN_RENDER_BACKEND_METAL
    static_cast<void>(map);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_io_surface_ring_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error(
      "IOSurface rings are not supported by this build"
    );
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::io_surface_ring_attach_start(
      map, descriptor, options, out_session, completion
    );
#endif
  });
}

auto mln_acquired_frame_get_io_surface(
  mln_acquired_frame frame, mln_io_surface_frame* out_frame,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::acquired_frame_get_io_surface(frame, out_frame);
  });
}
