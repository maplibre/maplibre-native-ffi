// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Copies Metal-native metadata from an acquired frame.
 *
 * See `mln_acquired_frame_get_metal_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun <T> GeneratedAcquiredFrameOperations.withMetalTexture(
  block: (MetalTextureFrame) -> T,
): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(w(64, 72), 8)
      check(C.mln_acquired_frame_get_metal_texture(handle, out, diagnostic))
      block(readMetalTextureFrame(out, scope))
    }
  }

/**
 * Starts attachment of a ring of caller-owned Metal textures.
 *
 * See `mln_map_attach_metal_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun GeneratedMapOperations.attachMetalBorrowedTexture(
  descriptor: MetalBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
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
      { RenderSessionHandle(it, this@attachMetalBorrowedTexture as MapHandle) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a session-owned Metal texture ring.
 *
 * See `mln_map_attach_metal_owned_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun GeneratedMapOperations.attachMetalOwnedTexture(
  descriptor: MetalOwnedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_metal_owned_texture(
          handle,
          writeMetalOwnedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { RenderSessionHandle(it, this@attachMetalOwnedTexture as MapHandle) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a Metal surface target.
 *
 * See `mln_map_attach_metal_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun GeneratedMapOperations.attachMetalSurface(
  descriptor: MetalSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
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
      { RenderSessionHandle(it, this@attachMetalSurface as MapHandle) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts an ordered replacement of every texture of a caller-owned Metal ring.
 *
 * See `mln_render_session_set_metal_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun GeneratedRenderSessionOperations.setMetalBorrowedTextureTarget(
  descriptor: MetalBorrowedTextureDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_metal_borrowed_texture_target(
        handle,
        writeMetalBorrowedTextureDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Starts an ordered Metal surface replacement.
 *
 * See `mln_render_session_set_metal_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun GeneratedRenderSessionOperations.setMetalSurfaceTarget(
  descriptor: MetalSurfaceDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_metal_surface_target(
        handle,
        writeMetalSurfaceDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }
