/**
 * @file maplibre_native_c/platform/android.h
 * Set up the library in an Android app, and pace its frames to the display.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_ANDROID_H
#define MAPLIBRE_NATIVE_C_PLATFORM_ANDROID_H

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
 * `AChoreographer`.
 *
 * Call it on a thread with an `ALooper`, such as the app's main thread, and
 * keep that thread running its looper while sessions use the pacer.
 *
 * `AChoreographer` can stop while the phone's display is off, even while a
 * Surface on another display, such as an Android Auto projection, still shows
 * the map. The pacer then keeps demanding frames from a timer on the looper,
 * within the frame rate limit, until vsyncs arrive again.
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

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_ANDROID_H
