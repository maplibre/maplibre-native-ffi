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
 * Gets the IOSurface of an acquired frame.
 *
 * Wait for `AcquiredFrameHandle.withProducerSync()` before you read the surface. Once you release
 * or discard the frame, the session may render into the surface again. For an API that takes a
 * `CVPixelBuffer`, wrap the surface with `CVPixelBufferCreateWithIOSurface`. A reference that you
 * take with `CFRetain`, or a pixel buffer that wraps the surface, keeps the surface's memory, but
 * not its contents.
 *
 * See `mln_acquired_frame_get_io_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withIoSurface(block: (IoSurfaceFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(32, 40, 28), w(8, 8, 4))
      check(C.mln_acquired_frame_get_io_surface(handle, out, diagnostic))
      block(readIoSurfaceFrame(out, scope))
    }
  }

/**
 * Gets the Metal texture of an acquired frame.
 *
 * See `mln_acquired_frame_get_metal_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withMetalTexture(block: (MetalTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(48, 56, 40), w(8, 8, 4))
      check(C.mln_acquired_frame_get_metal_texture(handle, out, diagnostic))
      block(readMetalTextureFrame(out, scope))
    }
  }

/**
 * Creates a frame pacer that follows the display through a `CADisplayLink`.
 *
 * You can call it on any thread. The pacer needs no run loop of yours. On macOS, it follows the
 * main display, the one with the menu bar.
 *
 * With `FramePacerOptions.maximumFramesPerSecond`, the pacer also asks the system for that refresh
 * rate. On iPhone, a rate above 60 needs `CADisableMinimumFrameDurationOnPhone` in the app's
 * `Info.plist`.
 *
 * See `mln_apple_frame_pacer_create` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * Errors:
 *
 * - `Status.NATIVE_ERROR` when the system has no display to follow.
 */
public fun appleFramePacerCreate(options: FramePacerOptions? = null): FramePacerHandle =
  nativeCall(null, null) {
    val out = allocate(8, w(8, 8, 4))
    check(
      C.mln_apple_frame_pacer_create(
        options?.let { writeFramePacerOptions(it) } ?: 0L,
        out,
        diagnostic,
      ),
    )
    adopt(out, GeneratedOwnerDisposal::framePacer) { FramePacerHandle.wrap(it) }
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
      { created, ready ->
        RenderSessionHandle.wrap(created, this@attachDxgiSharedTextureRing, ready)
      },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that renders the map into a ring of IOSurfaces that the library allocates.
 *
 * Acquire each frame with `RenderSessionHandle.acquireFrame()`, and get its surface with
 * `AcquiredFrameHandle.withIoSurface()`. `RenderSessionHandle.resize()` gives the ring new surfaces
 * of the new size. Attach with `RenderDriverKind.CORE_WORKER`.
 *
 * To hand frames to a reader that can't wait on an event, such as Flutter's `copyPixelBuffer`,
 * attach without `GpuSyncKindFlag.METAL_SHARED_EVENT` in
 * `RenderSessionAttachOptions.producerSyncKinds`, so that each frame finishes before you acquire
 * it. Release a frame with `GpuSyncKind.CPU_COMPLETE` only once the reader has finished with its
 * surface.
 *
 * See `mln_map_attach_io_surface_ring` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.METAL`.
 * - `Status.NATIVE_ERROR` when `context` names no device and the system has no Metal device.
 */
public fun MapHandle.attachIoSurfaceRing(
  descriptor: IoSurfaceRingDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_io_surface_ring(
          handle,
          writeIoSurfaceRingDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachIoSurfaceRing, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that renders the map into a ring of your Metal textures.
 *
 * See `mln_map_attach_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.METAL`, or
 *   when the target supports none of the `producer_sync_kinds` in `options`.
 */
public fun MapHandle.attachMetalBorrowedTexture(
  descriptor: MetalBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_metal_borrowed_texture(
          handle,
          writeMetalBorrowedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready ->
        RenderSessionHandle.wrap(created, this@attachMetalBorrowedTexture, ready)
      },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that renders the map to a Metal layer.
 *
 * See `mln_map_attach_metal_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.METAL`.
 */
public fun MapHandle.attachMetalSurface(
  descriptor: MetalSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_metal_surface(
          handle,
          writeMetalSurfaceDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachMetalSurface, ready) },
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
      { created, ready -> RenderSessionHandle.wrap(created, this@attachWin32Window, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Replaces the Metal textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first. Once the replacement
 * completes, the session no longer uses the old textures.
 *
 * See `mln_render_session_set_metal_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when the new textures have a different pixel format than the old ones.
 *   Attach a new session to change it.
 */
public fun RenderSessionHandle.setMetalBorrowedTextureTarget(
  replacement: MetalBorrowedTextureReplacement,
): Completion<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_metal_borrowed_texture_target(
        handle,
        writeMetalBorrowedTextureReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Replaces the Metal layer that the session renders to.
 *
 * Use it to give the session a layer again after `RenderSessionHandle.releaseTarget()`, to move it
 * to another layer, or to resize it. Once the replacement completes, the session no longer uses the
 * old layer.
 *
 * See `mln_render_session_set_metal_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun RenderSessionHandle.setMetalSurfaceTarget(
  replacement: MetalSurfaceReplacement,
): Completion<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_metal_surface_target(
        handle,
        writeMetalSurfaceReplacement(replacement),
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
): Completion<Unit> =
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
