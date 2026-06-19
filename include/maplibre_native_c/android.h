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
 * Initializes Android platform services that require access to the host app.
 *
 * This function is required before Android HTTP requests can validate TLS using
 * the app's platform trust policy. The host must package the
 * rustls-platform-verifier Android component in the APK or AAB.
 *
 * `jni_env` must be a `JNIEnv*` valid for the calling thread. `context` must be
 * an Android `android.content.Context` object. Any Context subtype may be
 * passed; the implementation resolves and stores the process application
 * context when initialization succeeds. Both pointers are borrowed for the
 * duration of the call.
 *
 * May be called from any thread that is attached to the JVM.
 *
 * Returns:
 * - MLN_STATUS_OK when initialization succeeds or was already completed;
 * - MLN_STATUS_INVALID_ARGUMENT when `jni_env` or `context` is null;
 * - MLN_STATUS_UNSUPPORTED when this library was not built for Android;
 * - MLN_STATUS_NATIVE_ERROR when Android verifier initialization fails.
 */
MLN_API mln_status mln_android_init(void* jni_env, void* context) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_ANDROID_H
