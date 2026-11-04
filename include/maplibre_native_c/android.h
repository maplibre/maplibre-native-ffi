/**
 * @file maplibre_native_c/android.h
 * Set up the library in an Android app, and pace its frames to the display.
 */

#ifndef MAPLIBRE_NATIVE_C_ANDROID_H
#define MAPLIBRE_NATIVE_C_ANDROID_H

#include "base.h"
#include "render_session.h"

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

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_ANDROID_H
