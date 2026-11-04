#pragma once

#include "maplibre_native_c.h"

namespace mln::platform::android {

// Creates a pacer that waits for vsyncs with the calling thread's
// AChoreographer.
auto create_frame_pacer(
  const mln_frame_pacer_options* options, mln_frame_pacer* out_pacer
) -> mln_status;

}  // namespace mln::platform::android
