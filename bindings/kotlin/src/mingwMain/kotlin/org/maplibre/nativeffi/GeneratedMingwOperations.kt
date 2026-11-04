// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the DXGI shared texture of an acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the texture. Once you release
 * or discard the frame, the session may render into the texture again. Open the handle with
 * `ID3D11Device1::OpenSharedResource1` or `ID3D12Device::OpenSharedHandle`. A texture that you open
 * keeps its memory, but not its contents.
 *
 * The session owns the handle, so don't close it. To keep the handle after you release the frame,
 * duplicate it with `DuplicateHandle`.
 *
 * See `mln_acquired_frame_get_dxgi_shared_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withDxgiSharedTexture(block: (DxgiSharedTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(40, 40, 32), w(8, 8, 4))
      check(C.mln_acquired_frame_get_dxgi_shared_texture(handle, out, diagnostic))
      block(readDxgiSharedTextureFrame(out, scope))
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
    adopt(out, GeneratedOwnerDisposal::vulkanDevice) { VulkanDeviceHandle(it) }
  }

/**
 * Creates a session that renders the map into a ring of DXGI shared textures that the library
 * allocates.
 *
 * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its texture with
 * `AcquiredFrameHandle.withDxgiSharedTexture()`. `RenderSessionHandle.resize()` gives the ring new
 * textures of the new size.
 *
 * The session renders with the Vulkan device `device`, or, when `device` is null, with one that the
 * library creates. The textures belong to the device's GPU, so open them on a D3D11 or D3D12 device
 * of the same adapter. To pick the GPU, create `device` with `VulkanDeviceOptions.deviceLuid` set
 * to your adapter's LUID. Attach with `RenderDriverKind.CORE_WORKER`.
 *
 * For a reader that waits on no fence, such as Flutter's texture registrar, attach without
 * `GpuSyncKindFlag.D3D12_FENCE` in `RenderSessionAttachOptions.producerSyncKinds`, so that each
 * frame finishes before you acquire it. Release a frame with `GpuSyncKind.CPU_COMPLETE` only once
 * the reader has finished with its texture.
 *
 * See `mln_map_attach_dxgi_shared_texture_ring` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` in an OpenGL build, or when the GPU can't share the textures: D3D11 has no
 *   adapter with the Vulkan device's LUID or can't share textures through NT handles, or the device
 *   lacks VK_KHR_external_memory_win32 or can't import D3D11 textures.
 * - `Status.NATIVE_ERROR` when the library can't create its Vulkan device or a D3D11 device on the
 *   adapter.
 */
public fun MapHandle.attachDxgiSharedTextureRing(
  descriptor: DxgiSharedTextureRingDescriptor,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_dxgi_shared_texture_ring(
          handle,
          writeDxgiSharedTextureRingDescriptor(descriptor),
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle(created, this@attachDxgiSharedTextureRing, ready) },
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
      { created, ready -> RenderSessionHandle(created, this@attachVulkanBorrowedTexture, ready) },
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
      { created, ready -> RenderSessionHandle(created, this@attachVulkanSurface, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that presents the map to a Win32 window, with a graphics context that the
 * library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a device that the library
 * creates. An OpenGL build uses a WGL context, and `device` must be null. Attach with
 * `RenderDriverKind.CORE_WORKER`.
 *
 * An OpenGL build sets the window's pixel format, so don't set one yourself.
 *
 * The session presents without waiting for the display, so demand at most one frame per display
 * refresh. While the window is minimized, frame demands finish with
 * `RenderResult.TARGET_NOT_READY`.
 *
 * Keep the window until a release or replacement of the target, or a detach of the session,
 * completes.
 *
 * See `mln_map_attach_win32_window` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_ARGUMENT` when `window` isn't a window, or, in an OpenGL build, already has
 *   another pixel format.
 * - `Status.INVALID_STATE` when `window` is minimized or has no client area.
 * - `Status.UNSUPPORTED` when you created `device` without `VulkanDeviceOptions.presentation`.
 * - `Status.INVALID_STATE` when `window` was minimized before the attach ran.
 * - `Status.UNSUPPORTED` when the GPU or the Vulkan loader can't present.
 * - `Status.NATIVE_ERROR` when the library can't create the graphics context or present to the
 *   window.
 */
public fun MapHandle.attachWin32Window(
  descriptor: Win32WindowDescriptor,
  device: VulkanDeviceHandle? = null,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_win32_window(
          handle,
          writeWin32WindowDescriptor(descriptor),
          device?.let { argument(it, it.binding) } ?: 0L,
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle(created, this@attachWin32Window, ready) },
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
): Deferred<Unit> =
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
): Deferred<Unit> =
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
 * Replaces the Win32 window that the session presents to.
 *
 * Use it to give the session a window again after `RenderSessionHandle.releaseTarget()`, to move it
 * to another window, or to resize it. Once the replacement completes, the session no longer uses
 * the old window.
 *
 * See `mln_render_session_set_win32_window_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html).
 *
 * Errors:
 *
 * - `Status.INVALID_ARGUMENT` when `window` isn't a window, or, in an OpenGL build, already has
 *   another pixel format.
 * - `Status.INVALID_STATE` when `window` is minimized or has no client area.
 * - `Status.INVALID_STATE` when `window` was minimized before the replacement ran.
 * - `Status.UNSUPPORTED` when, in a Vulkan build, the new window needs a different color format
 *   than the old one. Attach a new session instead.
 * - `Status.NATIVE_ERROR` when the library can't present to the new window.
 */
public fun RenderSessionHandle.setWin32WindowTarget(
  replacement: Win32WindowReplacement,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_win32_window_target(
        handle,
        writeWin32WindowReplacement(replacement),
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
