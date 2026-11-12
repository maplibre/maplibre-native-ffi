/**
 * @file maplibre_native_c/platform/ohos.h
 * Pace the library's frames to an OpenHarmony device's display.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_OHOS_H
#define MAPLIBRE_NATIVE_C_PLATFORM_OHOS_H

#include <stdint.h>

#include "../base.h"
#include "../completion.h"
#include "../map.h"
#include "../render_session.h"
#include "../render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a frame pacer that follows the display through OpenHarmony's
 * `OH_NativeVSync`.
 *
 * You can call it on any thread. The pacer needs no event loop of yours.
 *
 * Returns:
 * - MLN_STATUS_NATIVE_ERROR when the system can't report the display's
 *   refreshes to the process.
 */
MLN_BINDING("platforms=ohos")
MLN_API mln_status mln_ohos_frame_pacer_create(
  const mln_frame_pacer_options* options MLN_BINDING("nullable=true"),
  mln_frame_pacer* out_pacer MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_OHOS_H
