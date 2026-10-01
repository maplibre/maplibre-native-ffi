package org.maplibre.nativeffi.internal.c

import org.maplibre.nativeffi.internal.loader.checkAbiVersion

/**
 * The hand-written natives of the Android JNI shim in `src/androidMain/jni`.
 *
 * Loading the shim registers the natives of [C], [Jni], and
 * [org.maplibre.nativeffi.internal.memory.NativeMemory], so each loads it before its first call.
 */
internal object Jni {
  const val LIBRARY_NAME: String = "maplibre-native-ffi-jni"

  @Volatile private var checked = false

  init {
    System.loadLibrary(LIBRARY_NAME)
  }

  /** Loads the shim, and through it the C library. A class with natives calls this first. */
  fun load() {}

  /** Loads the shim, then checks the C ABI version once. */
  fun ensureLoaded() {
    if (checked) return
    synchronized(this) {
      if (!checked) {
        checkAbiVersion(C.mln_c_version().toUInt().toLong())
        checked = true
      }
    }
  }

  /** The address of the C function that calls the [index]th method of [Upcalls]. */
  @JvmStatic external fun upcallStub(index: Int): Long

  /** Calls `mln_android_init` with this thread's JNIEnv and [context]. */
  @JvmStatic external fun androidInit(context: Any, diagnostic: Long): Int
}
