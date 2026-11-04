// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

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
    adopt(out, GeneratedOwnerDisposal::framePacer) { FramePacerHandle(it) }
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
      { created, ready -> RenderSessionHandle(created, this@attachIoSurfaceRing, ready) },
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
      { created, ready -> RenderSessionHandle(created, this@attachMetalBorrowedTexture, ready) },
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
      { created, ready -> RenderSessionHandle(created, this@attachMetalSurface, ready) },
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
): Deferred<Unit> =
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
): Deferred<Unit> =
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
