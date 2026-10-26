/**
 * @file maplibre_native_c/android.h
 * Public C API declarations for Android process integration.
 */

#ifndef MAPLIBRE_NATIVE_C_ANDROID_H
#define MAPLIBRE_NATIVE_C_ANDROID_H

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initializes Android platform services.
 *
 * Required once per Android process, before creating a runtime. Configures
 * locale services, HTTP, and APK asset access.
 *
 * `jni_env` must be a `JNIEnv*` valid for the calling thread. `context` must be
 * an Android `android.content.Context` object. Any Context subtype may be
 * passed. Successful initialization retains that Context's AssetManager for
 * APK asset access. Pass an Activity or the application Context to read the
 * app APK. All pointers are borrowed for the duration of the call.
 *
 * May be called from any thread that is attached to the JVM.
 *
 * Returns:
 * - MLN_STATUS_OK when initialization succeeds or was already completed;
 * - MLN_STATUS_INVALID_ARGUMENT when `jni_env` or `context` is null;
 * - MLN_STATUS_UNSUPPORTED when this library was not built for Android;
 * - MLN_STATUS_NATIVE_ERROR when platform initialization fails.
 */
MLN_BINDING("platforms=android")
MLN_API mln_status mln_android_init(
  void* jni_env, void* context, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_ANDROID_H
