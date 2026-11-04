#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/surface_session.hpp"

#ifdef __OHOS__
#include "platform/ohos/frame_pacer.hpp"
#include "render/ohos_surface_session.hpp"
#endif

auto mln_ohos_frame_pacer_create(
  const mln_frame_pacer_options* options, mln_frame_pacer* out_pacer,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __OHOS__
    static_cast<void>(options);
    static_cast<void>(out_pacer);
    mln::core::set_thread_error("OpenHarmony frame pacers are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::platform::ohos::create_frame_pacer(options, out_pacer);
#endif
  });
}

auto mln_map_attach_ohos_surface(
  mln_map map, const mln_ohos_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __OHOS__
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status = mln::core::validate_ohos_surface_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("OpenHarmony surfaces are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::ohos_surface_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_render_session_set_ohos_surface_target(
  mln_render_session session, const mln_ohos_surface_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __OHOS__
    static_cast<void>(session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_ohos_surface_replacement(replacement);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("OpenHarmony surfaces are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::ohos_surface_set_target_start(
      session, replacement, completion
    );
#endif
  });
}
