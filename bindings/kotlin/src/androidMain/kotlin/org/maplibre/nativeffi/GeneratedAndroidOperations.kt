// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the Android hardware buffer of an acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the buffer. Once you release or
 * discard the frame, the session may render into the buffer again. A reference that you take with
 * `AHardwareBuffer_acquire` keeps the buffer's memory, but not its contents.
 *
 * See `mln_acquired_frame_get_android_hardware_buffer` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
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
    adopt(out, GeneratedOwnerDisposal::framePacer) { FramePacerHandle.wrap(it) }
  }

/**
 * Creates a session that renders the map into a ring of Android hardware buffers that the library
 * allocates.
 *
 * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its buffer with
 * `AcquiredFrameHandle.withAndroidHardwareBuffer()`. `RenderSessionHandle.resize()` gives the ring
 * new buffers of the new size.
 *
 * A Vulkan build renders with `device`, or, when `device` is null, with a device that the library
 * creates. An OpenGL build uses an EGL context on the default display, and `device` must be null.
 * Attach with `RenderDriverKind.CORE_WORKER`.
 *
 * To show the buffers through Android's UI, such as with `Bitmap.wrapHardwareBuffer`, attach
 * without `GpuSyncKindFlag.SYNC_FD` in `RenderSessionAttachOptions.producerSyncKinds`, so that each
 * frame finishes before you acquire it. Keep each frame acquired until the next one replaces it on
 * screen, then release it with `GpuSyncKind.CPU_COMPLETE`. Give the ring 3 buffers, so the session
 * can render while you hold two frames.
 *
 * See `mln_map_attach_android_hardware_buffer_ring` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when Android is older than 8.0, API level 26, or when the graphics context
 *   can't render into hardware buffers: the Vulkan device lacks
 *   VK_ANDROID_external_memory_android_hardware_buffer, or the EGL display lacks
 *   EGL_ANDROID_get_native_client_buffer or EGL_ANDROID_image_native_buffer.
 * - `Status.NATIVE_ERROR` when the library can't create its Vulkan device or initialize the default
 *   EGL display.
 */
public fun MapHandle.attachAndroidHardwareBufferRing(
  descriptor: AndroidHardwareBufferRingDescriptor,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_android_hardware_buffer_ring(
          handle,
          writeAndroidHardwareBufferRingDescriptor(descriptor),
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready ->
        RenderSessionHandle.wrap(created, this@attachAndroidHardwareBufferRing, ready)
      },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that presents the map to an Android window, with a graphics context that the
 * library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a device that the library
 * creates. An OpenGL build uses an EGL context on the default display, and `device` must be null.
 * Attach with `RenderDriverKind.CORE_WORKER`.
 *
 * When the platform destroys the window, such as in `SurfaceHolder.Callback.surfaceDestroyed`,
 * release the target with `RenderSessionHandle.releaseTarget()`, or give the session a new window
 * with `RenderSessionHandle.setAndroidSurfaceTarget()`, without waiting for that to complete. The
 * session keeps its own reference to the window and stays usable for the next window. A frame to
 * the destroyed window finishes with `RenderResult.TARGET_NOT_READY`, or with
 * `RenderResult.RENDERED` if the window went away while the frame was presenting.
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
      { created, ready -> RenderSessionHandle.wrap(created, this@attachAndroidSurface, ready) },
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
): Completion<Unit> =
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
