// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Creates a frame pacer that follows the display through the calling thread's `AChoreographer`.
 *
 * Call it on a thread with an `ALooper`, such as the app's main thread, and keep that thread
 * running its looper while sessions use the pacer.
 *
 * See `mln_android_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_STATE` when the calling thread has no `ALooper`.
 */
public fun androidFramePacerCreate(options: FramePacerOptions? = null): FramePacerHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(
      C.mln_android_frame_pacer_create(
        options?.let { writeFramePacerOptions(it) } ?: 0L,
        out,
        diagnostic,
      ),
    )
    adopt(out, GeneratedOwnerDisposal::framePacer) { FramePacerHandle(it) }
  }

/**
 * Creates a session that presents the map to an Android window, with a graphics context that the
 * library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a device that the library
 * creates. An OpenGL build uses an EGL context on the default display, and `device` must be null.
 * Attach with `RenderDriverKind.CORE_WORKER`.
 *
 * Before the platform destroys the window, such as in `SurfaceHolder.Callback.surfaceDestroyed`,
 * release the target with `RenderSessionHandle.releaseTarget()` and wait for that to complete.
 *
 * See `mln_map_attach_android_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_ARGUMENT` when `surface` has no window, such as after the platform destroyed
 *   it.
 * - `Status.UNSUPPORTED` when you created `device` without `VulkanDeviceOptions.presentation`.
 * - `Status.UNSUPPORTED` when the GPU or the Vulkan loader can't present.
 * - `Status.NATIVE_ERROR` when the library can't create the graphics context or the window's
 *   surface.
 */
public fun MapHandle.attachAndroidSurface(
  descriptor: AndroidSurfaceDescriptor,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_android_surface(
          handle,
          writeAndroidSurfaceDescriptor(descriptor),
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle(created, this@attachAndroidSurface, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Replaces the Android window that the session presents to.
 *
 * Use it to give the session a window again after `RenderSessionHandle.releaseTarget()`, to move it
 * to another window, or to resize it. Once the replacement completes, the session no longer uses
 * the old window.
 *
 * See `mln_render_session_set_android_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_ARGUMENT` when `surface` has no window, such as after the platform destroyed
 *   it.
 * - `Status.UNSUPPORTED` when, in a Vulkan build, the new window needs a different color format or
 *   pre-rotation support than the old one. Attach a new session instead.
 * - `Status.NATIVE_ERROR` when the library can't create the window's surface.
 */
public fun RenderSessionHandle.setAndroidSurfaceTarget(
  replacement: AndroidSurfaceReplacement,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_android_surface_target(
        handle,
        writeAndroidSurfaceReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }
