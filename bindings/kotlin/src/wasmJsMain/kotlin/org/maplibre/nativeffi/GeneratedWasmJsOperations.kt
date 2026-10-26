// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Copies WebGPU-native metadata from an acquired frame.
 *
 * See `mln_acquired_frame_get_web_gpu_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun <T> AcquiredFrameHandle.withWebGpuTexture(block: (WebGpuTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(64, 80), 8)
      check(C.mln_acquired_frame_get_web_gpu_texture(handle, out, diagnostic))
      block(readWebGpuTextureFrame(out, scope))
    }
  }

/**
 * Starts attachment of a ring of caller-owned WebGPU textures.
 *
 * See `mln_map_attach_web_gpu_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun MapHandle.attachWebGpuBorrowedTexture(
  descriptor: WebGpuBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
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
      { RenderSessionHandle(it, this@attachWebGpuBorrowedTexture) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a session-owned WebGPU texture ring.
 *
 * See `mln_map_attach_web_gpu_owned_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun MapHandle.attachWebGpuOwnedTexture(
  descriptor: WebGpuOwnedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_web_gpu_owned_texture(
          handle,
          writeWebGpuOwnedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { RenderSessionHandle(it, this@attachWebGpuOwnedTexture) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a WebGPU surface target.
 *
 * See `mln_map_attach_web_gpu_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun MapHandle.attachWebGpuSurface(
  descriptor: WebGpuSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
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
      { RenderSessionHandle(it, this@attachWebGpuSurface) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts an ordered replacement of every texture of a caller-owned WebGPU ring.
 *
 * See `mln_render_session_set_web_gpu_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun RenderSessionHandle.setWebGpuBorrowedTextureTarget(
  descriptor: WebGpuBorrowedTextureDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_web_gpu_borrowed_texture_target(
        handle,
        writeWebGpuBorrowedTextureDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Starts an ordered WebGPU surface replacement.
 *
 * See `mln_render_session_set_web_gpu_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun RenderSessionHandle.setWebGpuSurfaceTarget(
  descriptor: WebGpuSurfaceDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_web_gpu_surface_target(
        handle,
        writeWebGpuSurfaceDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }
