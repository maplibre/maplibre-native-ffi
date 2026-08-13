package org.maplibre.nativeffi

import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.internal.c.mln_android_init
import org.maplibre.nativeffi.internal.status.Status
import platform.android.JNIEnvVar

/** Android-only platform integration entry points for Kotlin/Native. */
@OptIn(ExperimentalForeignApi::class)
public object MaplibreAndroid {
  /**
   * Initializes Android platform services using JNI handles from the host.
   *
   * [jniEnv] must be valid for the current thread. [context] must reference an
   * `android.content.Context`, such as `ANativeActivity.clazz`. Call this before creating a runtime
   * that may issue Android HTTP requests.
   */
  public fun initialize(jniEnv: CPointer<JNIEnvVar>, context: COpaquePointer) {
    Status.check(mln_android_init(jniEnv, null, context))
  }
}
