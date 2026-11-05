// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Gets the WebGPU texture of an acquired frame.
 *
 * See `mln_acquired_frame_get_web_gpu_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.TARGET_LOST` when the session failed, or you detached, abandoned, or disposed it.
 */
public fun <T> AcquiredFrameHandle.withWebGpuTexture(block: (WebGpuTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(48, 64, 40), w(8, 8, 4))
      check(C.mln_acquired_frame_get_web_gpu_texture(handle, out, diagnostic))
      block(readWebGpuTextureFrame(out, scope))
    }
  }

/**
 * Creates a session that renders the map into a ring of your WebGPU textures.
 *
 * See `mln_map_attach_web_gpu_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.WEB_GPU`, or
 *   when the target supports none of the `producer_sync_kinds` in `options`.
 */
public fun MapHandle.attachWebGpuBorrowedTexture(
  descriptor: WebGpuBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_web_gpu_borrowed_texture(
          handle,
          writeWebGpuBorrowedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready ->
        RenderSessionHandle.wrap(created, this@attachWebGpuBorrowedTexture, ready)
      },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Creates a session that renders the map to a WebGPU surface.
 *
 * See `mln_map_attach_web_gpu_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 *
 * Errors:
 *
 * - `Status.UNSUPPORTED` when `supportedRenderBackendMask()` lacks `RenderBackendFlag.WEB_GPU`.
 */
public fun MapHandle.attachWebGpuSurface(
  descriptor: WebGpuSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): RenderSessionHandle =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_web_gpu_surface(
          handle,
          writeWebGpuSurfaceDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { created, ready -> RenderSessionHandle.wrap(created, this@attachWebGpuSurface, ready) },
      { it.bindingCallbacks },
      { it.dispose() },
    )
  }

/**
 * Replaces the WebGPU textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * `RenderSessionHandle.releaseTarget()`. Release every acquired frame first. Once the replacement
 * completes, the session no longer uses the old textures.
 *
 * See `mln_render_session_set_web_gpu_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun RenderSessionHandle.setWebGpuBorrowedTextureTarget(
  replacement: WebGpuBorrowedTextureReplacement,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_web_gpu_borrowed_texture_target(
        handle,
        writeWebGpuBorrowedTextureReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Replaces the WebGPU surface that the session presents to.
 *
 * Use it to give the session a surface again after `RenderSessionHandle.releaseTarget()`, to move
 * it to another surface, or to resize it. Once the replacement completes, the session no longer
 * uses the old surface.
 *
 * See `mln_render_session_set_web_gpu_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun RenderSessionHandle.setWebGpuSurfaceTarget(
  replacement: WebGpuSurfaceReplacement,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_web_gpu_surface_target(
        handle,
        writeWebGpuSurfaceReplacement(replacement),
        completion,
        diagnostic,
      ),
    )
  }
