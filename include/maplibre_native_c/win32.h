/**
 * @file maplibre_native_c/win32.h
 * Present the map to a Win32 window.
 */

#ifndef MAPLIBRE_NATIVE_C_WIN32_H
#define MAPLIBRE_NATIVE_C_WIN32_H

#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "map.h"
#include "render_session.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A Win32 window to present to, for mln_map_attach_win32_window(). */
typedef struct mln_win32_window_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `HWND`. */
  void* window MLN_BINDING("required=true");
} mln_win32_window_descriptor;

/** A new Win32 window for mln_render_session_set_win32_window_target(). */
typedef struct mln_win32_window_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `HWND`. */
  void* window MLN_BINDING("required=true");
} mln_win32_window_replacement;

/**
 * Creates a session that presents the map to a Win32 window, with a graphics
 * context that the library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a
 * device that the library creates. An OpenGL build uses a WGL context, and
 * `device` must be null. Attach with MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * An OpenGL build sets the window's pixel format, so don't set one yourself.
 *
 * The session presents without waiting for the display, so demand at most one
 * frame per display refresh. While the window is minimized, frame demands
 * finish with MLN_RENDER_RESULT_TARGET_NOT_READY.
 *
 * Keep the window until a release or replacement of the target, or a detach of
 * the session, completes.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `window` isn't a window, or, in an OpenGL
 *   build, already has another pixel format.
 * - MLN_STATUS_INVALID_STATE when `window` is minimized or has no client area.
 * - MLN_STATUS_UNSUPPORTED when you created `device` without
 *   mln_vulkan_device_options.presentation.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when `window` was minimized before the attach ran.
 * - MLN_STATUS_UNSUPPORTED when the GPU or the Vulkan loader can't present.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the graphics context
 *   or present to the window.
 */
MLN_BINDING("execution=lifecycle;platforms=windows")
MLN_API mln_status mln_map_attach_win32_window(
  mln_map map, const mln_win32_window_descriptor* descriptor,
  mln_vulkan_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Win32 window that the session presents to.
 *
 * Use it to give the session a window again after
 * mln_render_session_release_target(), to move it to another window, or to
 * resize it. Once the replacement completes, the session no longer uses the
 * old window.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `window` isn't a window, or, in an OpenGL
 *   build, already has another pixel format.
 * - MLN_STATUS_INVALID_STATE when `window` is minimized or has no client area.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when `window` was minimized before the replacement
 *   ran.
 * - MLN_STATUS_UNSUPPORTED when, in a Vulkan build, the new window needs a
 *   different color format than the old one. Attach a new session instead.
 * - MLN_STATUS_NATIVE_ERROR when the library can't present to the new window.
 */
MLN_BINDING("execution=operation;platforms=windows")
MLN_API mln_status mln_render_session_set_win32_window_target(
  mln_render_session session, const mln_win32_window_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_WIN32_H
