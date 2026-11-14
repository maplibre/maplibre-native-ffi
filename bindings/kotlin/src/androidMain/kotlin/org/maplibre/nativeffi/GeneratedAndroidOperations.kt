// Generated from the C headers by tools/bindgen. Do not edit.
@file:kotlin.jvm.JvmMultifileClass
@file:kotlin.jvm.JvmName("MaplibreNative")

package org.maplibre.nativeffi

import kotlin.jvm.JvmName
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the Android hardware buffer of an exported ring's acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the buffer. Once you release or
 * discard the frame, the session may render into the buffer again. A reference that you take with
 * `AHardwareBuffer_acquire` keeps the buffer's memory, but not its contents.
 *
 * See `mln_acquired_frame_get_android_hardware_buffer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the session's ring shares its textures in another way.
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withAndroidHardwareBuffer(
  block: (AndroidHardwareBufferFrame) -> T,
): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(32, 40, 28), w(8, 8, 4))
      check(C.mln_acquired_frame_get_android_hardware_buffer(handle, out, diagnostic))
      block(readAndroidHardwareBufferFrame(out, scope))
    }
  }
