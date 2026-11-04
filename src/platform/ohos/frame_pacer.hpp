#pragma once

#include "maplibre_native_c.h"

namespace mln::platform::ohos {

// Creates a pacer that waits for vsyncs with an OH_NativeVSync of its own.
auto create_frame_pacer(
  const mln_frame_pacer_options* options, mln_frame_pacer* out_pacer
) -> mln_status;

}  // namespace mln::platform::ohos
