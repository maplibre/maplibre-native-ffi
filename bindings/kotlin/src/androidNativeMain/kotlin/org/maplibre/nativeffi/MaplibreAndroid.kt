package org.maplibre.nativeffi

import kotlinx.cinterop.CPointed
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.toCPointer
import org.maplibre.nativeffi.internal.c.mln_android_init
import org.maplibre.nativeffi.internal.status.Status
import platform.android.JNIEnvVar

/** Borrowed JNI environment address for the current Android host thread. */
public class AndroidJniEnvironment private constructor(internal val address: Long) {
  public companion object {
    /**
     * Wraps a non-null `JNIEnv*` address that is valid for the current thread.
     *
     * The value borrows the environment and grants no general memory access.
     */
    public fun ofAddress(address: Long): AndroidJniEnvironment =
      AndroidJniEnvironment(requireAddress(address, "JNIEnv"))
  }
}

/** Borrowed JNI reference to an Android `android.content.Context`. */
public class AndroidContextReference private constructor(internal val address: Long) {
  public companion object {
    /**
     * Wraps a non-null JNI object-reference address for an Android Context.
     *
     * The reference must remain valid for the initialization call. This value grants no general
     * memory access.
     */
    public fun ofAddress(address: Long): AndroidContextReference =
      AndroidContextReference(requireAddress(address, "Android Context"))
  }
}

/** Android-only platform integration entry points for Kotlin/Native. */
@OptIn(ExperimentalForeignApi::class)
public object MaplibreAndroid {
  /**
   * Initializes Android platform services using JNI handles from the host.
   *
   * Call this before creating a runtime that may issue Android HTTP requests or load `asset://` and
   * `file:///android_asset/` URLs from the APK. `asset://` URLs read the provided Context's
   * AssetManager.
   */
  public fun initialize(jniEnvironment: AndroidJniEnvironment, context: AndroidContextReference) {
    Status.check(
      mln_android_init(
        jniEnvironment.address.toCPointer<JNIEnvVar>(),
        null,
        context.address.toCPointer<CPointed>(),
      )
    )
  }
}

private fun requireAddress(address: Long, name: String): Long {
  require(address != 0L) { "$name address must not be zero" }
  return address
}
