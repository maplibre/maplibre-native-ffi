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
 * locale services, TLS verification, and APK asset access. The host must
 * package the rustls-platform-verifier Android component in the APK or AAB.
 *
 * `jni_env` must be a `JNIEnv*` valid for the calling thread. `jni_class` is
 * accepted for static JNI binding adapters and ignored by the implementation;
 * direct C callers may pass null. `context` must be an Android
 * `android.content.Context` object. Any Context subtype may be passed. The
 * implementation retains that Context's AssetManager when initialization
 * succeeds, so pass an Activity or the application Context to read the app
 * APK. TLS verification still resolves the process application context. All
 * pointers are borrowed for the duration of the call.
 *
 * May be called from any thread that is attached to the JVM.
 *
 * Returns:
 * - MLN_STATUS_OK when initialization succeeds or was already completed;
 * - MLN_STATUS_INVALID_ARGUMENT when `jni_env` or `context` is null;
 * - MLN_STATUS_UNSUPPORTED when this library was not built for Android;
 * - MLN_STATUS_NATIVE_ERROR when platform initialization fails.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_android_init(
  void* jni_env MLN_BINDING("kind=native_pointer;ownership=borrowed"),
  void* jni_class MLN_BINDING("kind=native_pointer;ownership=borrowed"),
  void* context MLN_BINDING("kind=native_pointer;ownership=borrowed"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_ANDROID_H
