/**
 * @file maplibre_native_c/platform/apple.h
 * Pace the library's frames to the display on Apple platforms.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_APPLE_H
#define MAPLIBRE_NATIVE_C_PLATFORM_APPLE_H

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
 * Creates a frame pacer that follows the display through a `CADisplayLink`.
 *
 * You can call it on any thread. The pacer needs no run loop of yours. On
 * macOS, it follows the main display, the one with the menu bar.
 *
 * With mln_frame_pacer_options.maximum_frames_per_second, the pacer also asks
 * the system for that refresh rate. On iPhone, a rate above 60 needs
 * `CADisableMinimumFrameDurationOnPhone` in the app's `Info.plist`.
 *
 * Returns:
 * - MLN_STATUS_NATIVE_ERROR when the system has no display to follow.
 */
MLN_BINDING("platforms=ios,macos,tvos")
MLN_API mln_status mln_apple_frame_pacer_create(
  const mln_frame_pacer_options* options MLN_BINDING("nullable=true"),
  mln_frame_pacer* out_pacer MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_APPLE_H
