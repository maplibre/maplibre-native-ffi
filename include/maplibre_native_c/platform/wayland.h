/**
 * @file maplibre_native_c/platform/wayland.h
 * Present the map to a Wayland surface.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_WAYLAND_H
#define MAPLIBRE_NATIVE_C_PLATFORM_WAYLAND_H

#include <stdint.h>

#include "../base.h"
#include "../completion.h"
#include "../map.h"
#include "../render_session.h"
#include "../render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A Wayland surface to present to, for mln_map_attach_wayland_surface(). */
typedef struct mln_wayland_surface_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `wl_display*`. */
  void* display MLN_BINDING("required=true");
  /** Your `wl_surface*`, on `display`. */
  void* surface MLN_BINDING("required=true");
} mln_wayland_surface_descriptor;

/**
 * A new Wayland surface for mln_render_session_set_wayland_surface_target().
 */
typedef struct mln_wayland_surface_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `wl_surface*`, on the display that the session attached with. */
  void* surface MLN_BINDING("required=true");
} mln_wayland_surface_replacement;

/**
 * Creates a session that presents the map to a Wayland surface, with a
 * graphics context that the library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a
 * device that the library creates. An OpenGL build uses an EGL context on the
 * display, and `device` must be null. Attach with
 * MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * The session presents without waiting for the compositor, so demand at most
 * one frame per display refresh.
 *
 * Keep the surface until a release or replacement of the target, or a detach
 * of the session, completes. Keep the display connected until the session's
 * detach completes. In an OpenGL build, if you also use EGL on the display,
 * initialize it before you attach.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when you created `device` without
 *   mln_vulkan_device_options.presentation.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the GPU or the Vulkan loader can't present to
 *   Wayland.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the graphics context
 *   or present to the surface.
 */
MLN_BINDING("execution=lifecycle;platforms=freebsd,linux")
MLN_API mln_status mln_map_attach_wayland_surface(
  mln_map map, const mln_wayland_surface_descriptor* descriptor,
  mln_vulkan_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Wayland surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * mln_render_session_release_target(), to move it to another surface, or to
 * resize it. Once the replacement completes, the session no longer uses the
 * old surface.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when, in a Vulkan build, the new surface needs a
 *   different color format than the old one. Attach a new session instead.
 * - MLN_STATUS_NATIVE_ERROR when the library can't present to the new surface.
 */
MLN_BINDING("execution=operation;platforms=freebsd,linux")
MLN_API mln_status mln_render_session_set_wayland_surface_target(
  mln_render_session session,
  const mln_wayland_surface_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_WAYLAND_H
