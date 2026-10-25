// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Copies WebGPU-native metadata from an acquired frame.
 *
 * See `mln_acquired_frame_get_webgpu_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun <T> GeneratedAcquiredFrameOperations.withWebgpuTexture(
  block: (WebgpuTextureFrame) -> T,
): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(64, 80), 8)
      check(C.mln_acquired_frame_get_webgpu_texture(handle, out, diagnostic))
      block(readWebgpuTextureFrame(out, scope))
    }
  }

/**
 * Starts attachment of a ring of caller-owned WebGPU textures.
 *
 * See `mln_map_attach_webgpu_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun GeneratedMapOperations.attachWebgpuBorrowedTexture(
  descriptor: WebgpuBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_webgpu_borrowed_texture(
          handle,
          writeWebgpuBorrowedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { RenderSessionHandle(it, this@attachWebgpuBorrowedTexture as MapHandle) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a session-owned WebGPU texture ring.
 *
 * See `mln_map_attach_webgpu_owned_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun GeneratedMapOperations.attachWebgpuOwnedTexture(
  descriptor: WebgpuOwnedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_webgpu_owned_texture(
          handle,
          writeWebgpuOwnedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { RenderSessionHandle(it, this@attachWebgpuOwnedTexture as MapHandle) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a WebGPU surface target.
 *
 * See `mln_map_attach_webgpu_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun GeneratedMapOperations.attachWebgpuSurface(
  descriptor: WebgpuSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_webgpu_surface(
          handle,
          writeWebgpuSurfaceDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { RenderSessionHandle(it, this@attachWebgpuSurface as MapHandle) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts an ordered replacement of every texture of a caller-owned WebGPU ring.
 *
 * See `mln_render_session_set_webgpu_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun GeneratedRenderSessionOperations.setWebgpuBorrowedTextureTarget(
  descriptor: WebgpuBorrowedTextureDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_webgpu_borrowed_texture_target(
        handle,
        writeWebgpuBorrowedTextureDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Starts an ordered WebGPU surface replacement.
 *
 * See `mln_render_session_set_webgpu_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun GeneratedRenderSessionOperations.setWebgpuSurfaceTarget(
  descriptor: WebgpuSurfaceDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_webgpu_surface_target(
        handle,
        writeWebgpuSurfaceDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }
