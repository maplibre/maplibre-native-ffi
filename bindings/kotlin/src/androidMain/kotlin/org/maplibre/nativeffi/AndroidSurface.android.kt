@file:kotlin.jvm.JvmMultifileClass
@file:kotlin.jvm.JvmName("MaplibreNative")

package org.maplibre.nativeffi

import android.view.Surface
import org.maplibre.nativeffi.internal.c.Jni

/**
 * Creates a session that presents the map to [surface]'s window.
 *
 * Works as [MapHandle.attach] with a [RenderTarget.AndroidWindow] of [surface]'s `ANativeWindow`
 * does. The session takes its own reference to the window, so this call keeps none. When the
 * platform destroys [surface], release the session's target or give it another surface without
 * waiting, as [AndroidWindowTarget] says.
 *
 * Errors:
 * - `Status.INVALID_ARGUMENT` when [surface] has no window, such as after the platform destroyed
 *   it.
 * - The errors of [MapHandle.attach].
 */
public fun MapHandle.attachAndroidSurface(
  surface: Surface,
  logicalSize: LogicalSize,
  device: GpuDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  withAndroidWindow(surface) { window ->
    attach(androidWindow(logicalSize, window), device, options)
  }

/**
 * Replaces the session's target with [surface]'s window.
 *
 * Works as [RenderSessionHandle.setTarget] with a [RenderTarget.AndroidWindow] of [surface]'s
 * `ANativeWindow` does.
 *
 * Errors:
 * - `Status.INVALID_ARGUMENT` when [surface] has no window, such as after the platform destroyed
 *   it.
 * - The errors of [RenderSessionHandle.setTarget].
 */
public fun RenderSessionHandle.setAndroidSurfaceTarget(
  surface: Surface,
  logicalSize: LogicalSize,
): Completion<Unit> =
  withAndroidWindow(surface) { window -> setTarget(androidWindow(logicalSize, window)) }

private fun androidWindow(logicalSize: LogicalSize, window: Long): RenderTarget =
  RenderTarget.AndroidWindow(AndroidWindowTarget(logicalSize, NativePointer.ofAddress(window)))

/**
 * Runs [block] with a reference of its own to [surface]'s `ANativeWindow`, which it releases once
 * [block] returns.
 */
private inline fun <T> withAndroidWindow(surface: Surface, block: (Long) -> T): T {
  Jni.ensureLoaded()
  val window = Jni.nativeWindowFromSurface(surface)
  if (window == 0L) {
    throw MaplibreException(Status.INVALID_ARGUMENT, "the Surface has no window")
  }
  try {
    return block(window)
  } finally {
    Jni.nativeWindowRelease(window)
  }
}
