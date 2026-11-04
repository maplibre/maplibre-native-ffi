/**
 * @file maplibre_native_c/platform/ohos.h
 * Present the map to an OpenHarmony app's windows, and pace its frames to the
 * display.
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

/**
 * An OpenHarmony window to present to, for mln_map_attach_ohos_surface().
 *
 * Name the window by `window` or by `surface_id`. The session takes its own
 * reference to the window, so you can release yours once the call returns.
 */
typedef struct mln_ohos_surface_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `OHNativeWindow*`, or null to name the window by `surface_id`. */
  void* window;
  /**
   * The surface ID of a surface in this process, such as the one that
   * `XComponentController.getXComponentSurfaceId()` reports, or 0 to name the
   * window by `window`.
   */
  uint64_t surface_id;
} mln_ohos_surface_descriptor;

/**
 * A new OpenHarmony window for mln_render_session_set_ohos_surface_target().
 *
 * Name the window by `window` or by `surface_id`. The session takes its own
 * reference to the window, so you can release yours once the call returns.
 */
typedef struct mln_ohos_surface_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `OHNativeWindow*`, or null to name the window by `surface_id`. */
  void* window;
  /**
   * The surface ID of a surface in this process, such as the one that
   * `XComponentController.getXComponentSurfaceId()` reports, or 0 to name the
   * window by `window`.
   */
  uint64_t surface_id;
} mln_ohos_surface_replacement;

/**
 * Creates a session that presents the map to an OpenHarmony window, with a
 * graphics context that the library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a
 * device that the library creates. An OpenGL build uses an EGL context on the
 * default display, and `device` must be null. Attach with
 * MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * When the platform destroys the window, such as in an `XComponent`'s
 * `onSurfaceDestroyed` callback, release the target with
 * mln_render_session_release_target(), or give the session a new window with
 * mln_render_session_set_ohos_surface_target(), without waiting for that to
 * complete. The session keeps its own reference to the window and stays
 * usable for the next window. A frame to the destroyed window finishes with
 * MLN_RENDER_RESULT_TARGET_NOT_READY, or with MLN_RENDER_RESULT_RENDERED if the
 * window went away while the frame was presenting.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when no surface in this process has
 *   mln_ohos_surface_descriptor.surface_id, such as after the platform
 *   destroyed it.
 * - MLN_STATUS_UNSUPPORTED when you created `device` without
 *   mln_vulkan_device_options.presentation.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the GPU or the Vulkan loader can't present.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the graphics context
 *   or the window's surface.
 */
MLN_BINDING("execution=lifecycle;platforms=ohos")
MLN_API mln_status mln_map_attach_ohos_surface(
  mln_map map, const mln_ohos_surface_descriptor* descriptor,
  mln_vulkan_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the OpenHarmony window that the session presents to.
 *
 * Use it to give the session a window again after
 * mln_render_session_release_target(), to move it to another window, or to
 * resize it. Once the replacement completes, the session no longer uses the
 * old window.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when no surface in this process has
 *   mln_ohos_surface_replacement.surface_id, such as after the platform
 *   destroyed it.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when, in a Vulkan build, the new window needs a
 *   different color format or pre-rotation support than the old one. Attach a
 *   new session instead.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the window's
 *   surface.
 */
MLN_BINDING("execution=operation;platforms=ohos")
MLN_API mln_status mln_render_session_set_ohos_surface_target(
  mln_render_session session, const mln_ohos_surface_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_OHOS_H
