package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

/** Borrowed JNI environment address for the current Android host thread. */
public class AndroidJniEnvironment private constructor(internal val address: Long) {
  public companion object {
    /**
     * Wraps a `JNIEnv*` address that is valid for the current thread.
     *
     * The value borrows the environment and grants no general memory access.
     */
    public fun ofAddress(address: Long): AndroidJniEnvironment = AndroidJniEnvironment(address)
  }
}

/** Borrowed JNI reference to an Android `android.content.Context`. */
public class AndroidContextReference private constructor(internal val address: Long) {
  public companion object {
    /**
     * Wraps a JNI object-reference address for an Android Context.
     *
     * The reference must remain valid for the initialization call. This value grants no general
     * memory access.
     */
    public fun ofAddress(address: Long): AndroidContextReference = AndroidContextReference(address)
  }
}

/**
 * Initializes Android platform services using JNI handles from the host.
 *
 * Required once during Android host setup, before creating a runtime. APK assets use the provided
 * Context's AssetManager. A zero address in either argument fails with `Status.INVALID_ARGUMENT`.
 */
public fun Maplibre.initialize(
  jniEnvironment: AndroidJniEnvironment,
  context: AndroidContextReference,
) {
  ensureNativeLibrary()
  NativeDiagnostics.check { diagnostic ->
    C.mln_android_init(jniEnvironment.address, context.address, diagnostic)
  }
}
