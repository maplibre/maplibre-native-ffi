#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

#ifdef __OHOS__
#include "platform/ohos/frame_pacer.hpp"
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
