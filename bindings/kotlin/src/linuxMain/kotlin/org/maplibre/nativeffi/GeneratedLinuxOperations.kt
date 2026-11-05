// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the dmabuf of an acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the buffer. Once you release or
 * discard the frame, the session may render into the buffer again. You can map a buffer with
 * DRM_FORMAT_MOD_LINEAR, which has one plane, with `mmap`, and bracket your reads with
 * DMA_BUF_IOCTL_SYNC.
 *
 * The session owns the planes' file descriptors, so don't close them. To keep the buffer after you
 * release the frame, duplicate each file descriptor with `dup`. A duplicate keeps the buffer's
 * memory, but not its contents.
 *
 * See `mln_acquired_frame_get_dma_buf` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withDmaBuf(block: (DmaBufFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(48, 56, 44), w(8, 8, 4))
      check(C.mln_acquired_frame_get_dma_buf(handle, out, diagnostic))
      block(readDmaBufFrame(out, scope))
    }
  }

/**
 * Gets the Vulkan image of an acquired frame.
 *
 * See `mln_acquired_frame_get_vulkan_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withVulkanTexture(block: (VulkanTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(64, 64, 52), w(8, 8, 4))
      check(C.mln_acquired_frame_get_vulkan_texture(handle, out, diagnostic))
      block(readVulkanTextureFrame(out, scope))
    }
  }

/**
 * Creates a Vulkan instance, device, and graphics queue for render sessions.
 *
 * Attach sessions with the context from `VulkanDeviceHandle.getContext()`, in place of a device of
 * your own. Without `device_uuid` or `device_luid`, it prefers a discrete GPU. The device enables
 * the external memory and semaphore extensions that the GPU offers.
 *
 * See `mln_vulkan_device_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.VULKAN`, or
 *   when `presentation` is true and the Vulkan loader or the GPU can't present.
 * - `Status.NOT_FOUND` when no GPU matches `device_uuid` and `device_luid`.
 * - `Status.NATIVE_ERROR` when there is no Vulkan loader, no GPU has a graphics queue, or Vulkan
 *   can't create the instance or the device.
 */
public fun VulkanDeviceHandle.Companion.create(
  options: VulkanDeviceOptions? = null,
): VulkanDeviceHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(
      C.mln_vulkan_device_create(
        options?.let { writeVulkanDeviceOptions(it) } ?: 0L,
        out,
        diagnostic,
      ),
    )
    adopt(out, GeneratedOwnerDisposal::vulkanDevice) { VulkanDeviceHandle.wrap(it) }
  }

/**
 * Creates a session that renders the map into a ring of dmabufs that the library allocates.
 *
 * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its buffer with
 * `AcquiredFrameHandle.withDmaBuf()`. `RenderSessionHandle.resize()` gives the ring new buffers of
 * the new size.
 *
 * A Vulkan build renders with `device`, or, when `device` is null, with a device that the library
 * creates. An OpenGL build uses an EGL context of its own, and `device` must be null. Attach with
 * `RenderDriverKind.CORE_WORKER`.
 *
 * For a reader that waits on no sync file, such as a Wayland compositor without explicit
 * synchronization, attach without `GpuSyncKindFlag.SYNC_FD` in
 * `RenderSessionAttachOptions.producerSyncKinds`, so that each frame finishes before you acquire
 * it. Keep each frame acquired until its reader is done with the buffer. Give the ring 3 buffers,
 * so the session can render while you hold two frames.
 *
 * See `mln_map_attach_dma_buf_ring` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the graphics context can't render into a dmabuf of any of `formats`.
 * - `Status.NATIVE_ERROR` when the library can't create its Vulkan device or initialize an EGL
 *   display.
 */
public fun MapHandle.attachDmaBufRing(
  descriptor: DmaBufRingDescriptor,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_dma_buf_ring(
          handle,
          writeDmaBufRingDescriptor(descriptor),
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachDmaBufRing, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that renders the map into a ring of your Vulkan images.
 *
 * See `mln_map_attach_vulkan_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.VULKAN`, or
 *   when the target supports none of the `producer_sync_kinds` in `options`.
 */
public fun MapHandle.attachVulkanBorrowedTexture(
  descriptor: VulkanBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_vulkan_borrowed_texture(
          handle,
          writeVulkanBorrowedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready ->
        RenderSessionHandle.wrap(created, this@attachVulkanBorrowedTexture, ready)
      },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that renders the map to a Vulkan surface.
 *
 * See `mln_map_attach_vulkan_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.VULKAN`.
 */
public fun MapHandle.attachVulkanSurface(
  descriptor: VulkanSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_vulkan_surface(
          handle,
          writeVulkanSurfaceDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachVulkanSurface, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that presents the map to a Wayland surface, with a graphics context that the
 * library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a device that the library
 * creates. An OpenGL build uses an EGL context on the display, and `device` must be null. Attach
 * with `RenderDriverKind.CORE_WORKER`.
 *
 * The session presents without waiting for the compositor, so demand at most one frame per display
 * refresh.
 *
 * Keep the surface until a release or replacement of the target, or a detach of the session,
 * completes. Keep the display connected until the session's detach completes. In an OpenGL build,
 * if you also use EGL on the display, initialize it before you attach.
 *
 * See `mln_map_attach_wayland_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when you created `device` without `VulkanDeviceOptions.presentation`.
 * - `Status.UNSUPPORTED` when the GPU or the Vulkan loader can't present to Wayland.
 * - `Status.NATIVE_ERROR` when the library can't create the graphics context or present to the
 *   surface.
 */
public fun MapHandle.attachWaylandSurface(
  descriptor: WaylandSurfaceDescriptor,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_wayland_surface(
          handle,
          writeWaylandSurfaceDescriptor(descriptor),
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachWaylandSurface, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Replaces the Vulkan images that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session images again after
 * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first. Once the replacement
 * completes, the session no longer uses the old images.
 *
 * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun RenderSessionHandle.setVulkanBorrowedTextureTarget(
  replacement: VulkanBorrowedTextureReplacement,
): Completion<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_vulkan_borrowed_texture_target(
        handle,
        writeVulkanBorrowedTextureReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Replaces the Vulkan surface that the session presents to.
 *
 * Use it to give the session a surface again after `RenderSessionHandle.releaseTarget()`, to move
 * it to another surface, or to resize it. Once the replacement completes, the session no longer
 * uses the old surface.
 *
 * See `mln_render_session_set_vulkan_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the new surface needs a different color format or pre-rotation
 *   support than the old one. Attach a new session instead.
 */
public fun RenderSessionHandle.setVulkanSurfaceTarget(
  replacement: VulkanSurfaceReplacement,
): Completion<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_vulkan_surface_target(
        handle,
        writeVulkanSurfaceReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Replaces the Wayland surface that the session presents to.
 *
 * Use it to give the session a surface again after `RenderSessionHandle.releaseTarget()`, to move
 * it to another surface, or to resize it. Once the replacement completes, the session no longer
 * uses the old surface.
 *
 * See `mln_render_session_set_wayland_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when, in a Vulkan build, the new surface needs a different color format
 *   than the old one. Attach a new session instead.
 * - `Status.NATIVE_ERROR` when the library can't present to the new surface.
 */
public fun RenderSessionHandle.setWaylandSurfaceTarget(
  replacement: WaylandSurfaceReplacement,
): Completion<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_wayland_surface_target(
        handle,
        writeWaylandSurfaceReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Gets the context to attach render sessions to the device with.
 *
 * Pass it as the context of `MapHandle.attachVulkanSurface()` or
 * `MapHandle.attachVulkanBorrowedTexture()`. The sessions on the device share its graphics queue.
 * To submit to the queue yourself, set `RenderSessionAttachOptions.queueLock` on each session.
 *
 * See `mln_vulkan_device_get_context` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
 */
public fun VulkanDeviceHandle.getContext(): VulkanDeviceContextResult =
  nativeCall(this, binding) {
    val out = sized(w(32, 64), w(4, 8))
    check(C.mln_vulkan_device_get_context(handle, out, diagnostic))
    readVulkanDeviceContextResult(out)
  }
