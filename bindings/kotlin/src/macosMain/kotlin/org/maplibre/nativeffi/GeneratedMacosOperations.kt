// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Copies Vulkan-native metadata from an acquired frame.
 *
 * See `mln_acquired_frame_get_vulkan_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun <T> AcquiredFrameHandle.withVulkanTexture(block: (VulkanTextureFrame) -> T): T =
  nativeCall(this, binding) {
    borrowView(
      { C.mln_acquired_frame_view_begin(handle, it, diagnostic) },
      { C.mln_acquired_frame_view_end(it) },
    ) { scope ->
      val out = sized(80, 8)
      check(C.mln_acquired_frame_get_vulkan_texture(handle, out, diagnostic))
      block(readVulkanTextureFrame(out, scope))
    }
  }

/**
 * Starts attachment of a ring of caller-owned Vulkan images.
 *
 * See `mln_map_attach_vulkan_borrowed_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun MapHandle.attachVulkanBorrowedTexture(
  descriptor: VulkanBorrowedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
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
      { RenderSessionHandle(it, this@attachVulkanBorrowedTexture) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a session-owned Vulkan texture ring.
 *
 * See `mln_map_attach_vulkan_owned_texture` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun MapHandle.attachVulkanOwnedTexture(
  descriptor: VulkanOwnedTextureDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
  nativeCall(this, binding) {
    attach(
      { out, completion ->
        C.mln_map_attach_vulkan_owned_texture(
          handle,
          writeVulkanOwnedTextureDescriptor(descriptor),
          writeRenderSessionAttachOptions(options),
          out,
          completion,
          diagnostic,
        )
      },
      GeneratedOwnerDisposal::renderSession,
      { RenderSessionHandle(it, this@attachVulkanOwnedTexture) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts attachment of a Vulkan surface target.
 *
 * See `mln_map_attach_vulkan_surface` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun MapHandle.attachVulkanSurface(
  descriptor: VulkanSurfaceDescriptor,
  options: RenderSessionAttachOptions,
): NewRenderSession =
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
      { RenderSessionHandle(it, this@attachVulkanSurface) },
      { it.bindingCallbacks },
      { it.dispose() },
      ::NewRenderSession,
    )
  }

/**
 * Starts an ordered replacement of every image of a caller-owned Vulkan ring.
 *
 * See `mln_render_session_set_vulkan_borrowed_texture_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
public fun RenderSessionHandle.setVulkanBorrowedTextureTarget(
  descriptor: VulkanBorrowedTextureDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_vulkan_borrowed_texture_target(
        handle,
        writeVulkanBorrowedTextureDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }

/**
 * Starts an ordered Vulkan surface replacement.
 *
 * See `mln_render_session_set_vulkan_surface_target` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
 */
public fun RenderSessionHandle.setVulkanSurfaceTarget(
  descriptor: VulkanSurfaceDescriptor,
): Deferred<Unit> =
  nativeUnit(this, binding) {
    check(
      C.mln_render_session_set_vulkan_surface_target(
        handle,
        writeVulkanSurfaceDescriptor(descriptor),
        completion,
        diagnostic,
      ),
    )
  }
