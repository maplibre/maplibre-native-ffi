@file:kotlin.jvm.JvmMultifileClass
@file:kotlin.jvm.JvmName("MaplibreNative")

package org.maplibre.nativeffi

import android.hardware.HardwareBuffer
import org.maplibre.nativeffi.internal.c.Jni

/**
 * Gets the hardware buffer of an acquired frame as a [HardwareBuffer], such as for
 * `Bitmap.wrapHardwareBuffer`.
 *
 * The [HardwareBuffer] holds its own reference to the buffer, so it stays valid until you close it.
 * Once you release or discard the frame, though, the session may render into the buffer again, so
 * keep the frame acquired while you show the buffer.
 *
 * Errors:
 * - The errors of [withAndroidHardwareBuffer].
 */
public fun AcquiredFrameHandle.getHardwareBuffer(): HardwareBuffer =
  withAndroidHardwareBuffer { frame ->
    checkNotNull(Jni.hardwareBuffer(frame.buffer.address) as HardwareBuffer?) {
      "Android can't make a HardwareBuffer of the frame's buffer"
    }
  }
