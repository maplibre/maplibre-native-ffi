package org.maplibre.nativeffi

import android.view.Surface
import org.maplibre.nativeffi.internal.c.Jni
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * Creates a session that presents the map to [surface]'s window, with a graphics context that the
 * library creates.
 *
 * Works as the [attachAndroidSurface] that takes an [AndroidSurfaceDescriptor] does, including when
 * the platform destroys [surface].
 *
 * Errors:
 * - `Status.INVALID_ARGUMENT` when [surface] has no window, such as after the platform destroyed
 *   it.
 * - The errors of the [attachAndroidSurface] that takes an [AndroidSurfaceDescriptor].
 */
public fun MapHandle.attachAndroidSurface(
  surface: Surface,
  logicalSize: LogicalSize,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        Jni.mapAttachAndroidSurface(
          handle,
          writeAndroidSurfaceDescriptor(
            AndroidSurfaceDescriptor(
              logicalSize,
              NativePointer.NULL_POINTER,
              NativePointer.NULL_POINTER,
              NativePointer.NULL_POINTER,
            ),
          ),
          surface,
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachAndroidSurface, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Replaces the window that the session presents to with [surface]'s.
 *
 * Works as the [setAndroidSurfaceTarget] that takes an [AndroidSurfaceReplacement] does.
 *
 * Errors:
 * - `Status.INVALID_ARGUMENT` when [surface] has no window, such as after the platform destroyed
 *   it.
 * - The errors of the [setAndroidSurfaceTarget] that takes an [AndroidSurfaceReplacement].
 */
public fun RenderSessionHandle.setAndroidSurfaceTarget(
  surface: Surface,
  logicalSize: LogicalSize,
): Completion<Unit> =
  nativeUnit(this, binding) {
    check(
      Jni.renderSessionSetAndroidSurfaceTarget(
        handle,
        writeAndroidSurfaceReplacement(
          AndroidSurfaceReplacement(
            logicalSize,
            NativePointer.NULL_POINTER,
            NativePointer.NULL_POINTER,
            NativePointer.NULL_POINTER,
          ),
        ),
        surface,
        completion,
        diagnostic,
      ),
    )
  }
