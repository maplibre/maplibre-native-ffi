/**
 * @file maplibre_native_c/android.h
 * Set up the library in an Android app.
 */

#ifndef MAPLIBRE_NATIVE_C_ANDROID_H
#define MAPLIBRE_NATIVE_C_ANDROID_H

#include "base.h"

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

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_ANDROID_H
