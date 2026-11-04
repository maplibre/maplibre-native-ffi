/**
 * @file maplibre_native_c/android.h
 * Set up the library in an Android app, present to its windows, and pace its
 * frames to the display.
 */

#ifndef MAPLIBRE_NATIVE_C_ANDROID_H
#define MAPLIBRE_NATIVE_C_ANDROID_H

#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "map.h"
#include "render_session.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gives the library the Java VM and the app context that it needs on Android.
 *
 * Call it before creating a runtime. `jni_env` is the calling thread's
 * `JNIEnv*`, and `context` is an Android `Context`, such as the application.
 */
MLN_BINDING("platforms=android")
MLN_API mln_status mln_android_init(
  void* jni_env, void* context, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a frame pacer that follows the display through the calling thread's
 * looper.
 *
 * Call it on a thread with an `ALooper`, such as the app's main thread. The
 * pacer demands frames from that thread at the display refreshes that the
 * looper's `AChoreographer` reports, so keep the thread running its looper
 * while sessions use the pacer.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the calling thread has no `ALooper`.
 */
MLN_BINDING("platforms=android")
MLN_API mln_status mln_android_frame_pacer_create(
  const mln_frame_pacer_options* options MLN_BINDING("nullable=true"),
  mln_frame_pacer* out_pacer MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * An Android window to present to, for mln_map_attach_android_surface().
 *
 * Name the window by `window`, or by `jni_env` and `surface`. The session takes
 * its own reference to the window, so you can release yours once the call
 * returns. It uses the window until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_android_surface_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `ANativeWindow*`, or null to name the window by `surface`. */
  void* window;
  /** The calling thread's `JNIEnv*`, with `surface`. */
  void* jni_env;
  /** An `android.view.Surface`, with `jni_env`. */
  void* surface;
} mln_android_surface_descriptor;

/**
 * A new Android window for mln_render_session_set_android_surface_target().
 *
 * Name the window by `window`, or by `jni_env` and `surface`. The session takes
 * its own reference to the window, so you can release yours once the call
 * returns. It uses the window until a replacement or release of the target
 * completes, a detach or release of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_android_surface_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `ANativeWindow*`, or null to name the window by `surface`. */
  void* window;
  /** The calling thread's `JNIEnv*`, with `surface`. */
  void* jni_env;
  /** An `android.view.Surface`, with `jni_env`. */
  void* surface;
} mln_android_surface_replacement;

/**
 * Creates a session that presents the map to an Android window, with a
 * graphics context that the library creates.
 *
 * The build's render backend decides the context. A Vulkan build presents with
 * `device`, or, when `device` is null, with a device that the library creates
 * with presentation and shares between these sessions. An OpenGL build creates
 * an EGL context on the default display, and `device` must be null. Attach with
 * MLN_RENDER_DRIVER_CORE_WORKER, since the context belongs to the library.
 *
 * Before the platform destroys the window, such as in
 * `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
 * mln_render_session_release_target() and wait for that to complete. The
 * session's reference keeps the window, but not its buffers.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `surface` has no window, such as after
 *   the platform destroyed it.
 * - MLN_STATUS_UNSUPPORTED when `device` can't present, because you created it
 *   without mln_vulkan_device_options.presentation.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the GPU or the Vulkan loader can't present.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the graphics context
 *   or the window's surface.
 */
MLN_BINDING("execution=lifecycle;platforms=android")
MLN_API mln_status mln_map_attach_android_surface(
  mln_map map, const mln_android_surface_descriptor* descriptor,
  mln_vulkan_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Android window that the session presents to.
 *
 * Use it to give the session a window again after
 * mln_render_session_release_target(), to move it to another window, or to
 * resize it. Once the replacement completes, the session no longer uses the
 * old window. A replacement with the window and logical size that the session
 * presents to already changes nothing.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `surface` has no window, such as after
 *   the platform destroyed it.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when, in a Vulkan build, the new window needs a
 *   different color format or pre-rotation support than the old one. Attach a
 *   new session instead.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the window's
 *   surface.
 */
MLN_BINDING("execution=operation;platforms=android")
MLN_API mln_status mln_render_session_set_android_surface_target(
  mln_render_session session,
  const mln_android_surface_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_ANDROID_H
